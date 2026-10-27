// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#include "windows.h"
#include "sddl.h"

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_pal/srw_lock_ll.h"
#include "c_pal/string_utils.h"
#include "c_pal/threadapi.h"
#include "c_pal/timer.h"

#include "c_pal/robust_semaphore.h"

MU_DEFINE_ENUM_STRINGS(ROBUST_SEMAPHORE_ACQUIRE_RESULT, ROBUST_SEMAPHORE_ACQUIRE_RESULT_VALUES);

#define ACQUISITION_STATE_VALUES \
    ACQUISITION_STATE_NONE, \
    ACQUISITION_STATE_REQUESTED, \
    ACQUISITION_STATE_WAITING, \
    ACQUISITION_STATE_HOLDING

MU_DEFINE_ENUM(ACQUISITION_STATE, ACQUISITION_STATE_VALUES);
MU_DEFINE_ENUM_STRINGS(ACQUISITION_STATE, ACQUISITION_STATE_VALUES);

typedef struct ROBUST_SEMAPHORE_TAG
{
    uint32_t max_count;
    HANDLE mutexes[ROBUST_SEMAPHORE_MAX_COUNT];

    HANDLE cancel_event;
    HANDLE release_event;

    volatile_atomic int32_t worker_thread_id;

    /* guards all the fields below */
    SRW_LOCK_LL lock;

    /* the last worker thread started, NULL if none was started */
    THREAD_HANDLE worker_thread;
    /* false once the worker thread found no new acquisition and decided to exit */
    bool worker_running;
    bool destroying;
    bool worker_frees_object;

    ACQUISITION_STATE acquisition_state;
    bool acquisition_cancelled;
    bool acquisition_released;
    uint32_t timeout_ms;
    uint32_t max_hold_ms;
    ROBUST_SEMAPHORE_ON_ACQUIRE_COMPLETE on_acquire_complete;
    void* on_acquire_complete_context;
} ROBUST_SEMAPHORE;

static void internal_close_handle(HANDLE handle, const char* handle_name)
{
    if (!CloseHandle(handle))
    {
        LogLastError("failure in CloseHandle(%s=%p)", handle_name, handle);
    }
    else
    {
        /* do nothing, closed */
    }
}

static void internal_set_event(HANDLE event, const char* event_name)
{
    if (!SetEvent(event))
    {
        LogLastError("failure in SetEvent(%s=%p)", event_name, event);
    }
    else
    {
        /* do nothing, signaled */
    }
}

static void internal_reset_event(HANDLE event, const char* event_name)
{
    if (!ResetEvent(event))
    {
        LogLastError("failure in ResetEvent(%s=%p)", event_name, event);
    }
    else
    {
        /* do nothing, reset */
    }
}

static void internal_free_robust_semaphore(ROBUST_SEMAPHORE_HANDLE robust_semaphore)
{
    /*Codes_SRS_ROBUST_SEMAPHORE_01_019: [ robust_semaphore_destroy shall close the mutexes and the events by calling CloseHandle. ]*/
    for (uint32_t i = 0; i < robust_semaphore->max_count; i++)
    {
        internal_close_handle(robust_semaphore->mutexes[i], "mutex");
    }

    internal_close_handle(robust_semaphore->release_event, "release_event");
    internal_close_handle(robust_semaphore->cancel_event, "cancel_event");

    /*Codes_SRS_ROBUST_SEMAPHORE_01_058: [ robust_semaphore_destroy shall deinitialize the lock by calling srw_lock_ll_deinit. ]*/
    srw_lock_ll_deinit(&robust_semaphore->lock);

    /*Codes_SRS_ROBUST_SEMAPHORE_01_059: [ robust_semaphore_destroy shall free the robust semaphore object. ]*/
    free(robust_semaphore);
}

static void internal_release_under_lock(ROBUST_SEMAPHORE_HANDLE robust_semaphore)
{
    if (
        (robust_semaphore->acquisition_state == ACQUISITION_STATE_NONE) ||
        robust_semaphore->acquisition_released
        )
    {
        /* do nothing, there is no acquisition or it was already released */
    }
    else
    {
        /*Codes_SRS_ROBUST_SEMAPHORE_01_034: [ If there is a current acquisition that was not released, robust_semaphore_release shall mark it as released and signal the cancel event and the release event by calling SetEvent. ]*/
        robust_semaphore->acquisition_released = true;
        internal_set_event(robust_semaphore->cancel_event, "cancel_event");
        internal_set_event(robust_semaphore->release_event, "release_event");

        if (robust_semaphore->acquisition_state != ACQUISITION_STATE_HOLDING)
        {
            /* do nothing, a waiting acquisition ends when its on_acquire_complete is called with ROBUST_SEMAPHORE_ACQUIRE_CANCELLED */
        }
        else
        {
            /*Codes_SRS_ROBUST_SEMAPHORE_01_051: [ If the current acquisition holds a unit, robust_semaphore_release shall end it. ]*/
            robust_semaphore->acquisition_state = ACQUISITION_STATE_NONE;
        }
    }
}

static void internal_end_acquisition_without_unit(ROBUST_SEMAPHORE_HANDLE robust_semaphore, ROBUST_SEMAPHORE_ACQUIRE_RESULT acquire_result, ROBUST_SEMAPHORE_ON_ACQUIRE_COMPLETE on_acquire_complete, void* on_acquire_complete_context)
{
    /*Codes_SRS_ROBUST_SEMAPHORE_01_044: [ robust_semaphore_worker_thread shall end an acquisition that did not get a unit before calling its on_acquire_complete, so that on_acquire_complete can start a new acquisition. ]*/
    srw_lock_ll_acquire_exclusive(&robust_semaphore->lock);
    robust_semaphore->acquisition_state = ACQUISITION_STATE_NONE;
    srw_lock_ll_release_exclusive(&robust_semaphore->lock);

    on_acquire_complete(on_acquire_complete_context, acquire_result, ROBUST_SEMAPHORE_UNIT_INDEX_NONE);
}

static void internal_hold_unit(ROBUST_SEMAPHORE_HANDLE robust_semaphore, uint32_t unit_index, uint32_t max_hold_ms, ROBUST_SEMAPHORE_ON_ACQUIRE_COMPLETE on_acquire_complete, void* on_acquire_complete_context)
{
    /*Codes_SRS_ROBUST_SEMAPHORE_01_041: [ If WaitForMultipleObjects returns WAIT_OBJECT_0 + 1 + i or WAIT_ABANDONED_0 + 1 + i then robust_semaphore_worker_thread shall record the time the unit was acquired by calling timer_global_get_elapsed_ms, mark the acquisition as holding unit i and call on_acquire_complete with ROBUST_SEMAPHORE_ACQUIRE_OK and i. ]*/
    double acquired_ms = timer_global_get_elapsed_ms();

    srw_lock_ll_acquire_exclusive(&robust_semaphore->lock);
    robust_semaphore->acquisition_state = ACQUISITION_STATE_HOLDING;
    srw_lock_ll_release_exclusive(&robust_semaphore->lock);

    on_acquire_complete(on_acquire_complete_context, ROBUST_SEMAPHORE_ACQUIRE_OK, unit_index);

    /*Codes_SRS_ROBUST_SEMAPHORE_01_045: [ If a unit was acquired, after on_acquire_complete returns, robust_semaphore_worker_thread shall wait for the release event by calling WaitForSingleObject with what is left of max_hold_ms since the unit was acquired (measured by calling timer_global_get_elapsed_ms, 0 if max_hold_ms already passed, INFINITE if max_hold_ms is ROBUST_SEMAPHORE_NO_TIMEOUT). ]*/
    DWORD hold_timeout_ms;
    if (max_hold_ms == ROBUST_SEMAPHORE_NO_TIMEOUT)
    {
        hold_timeout_ms = INFINITE;
    }
    else
    {
        double held_ms = timer_global_get_elapsed_ms() - acquired_ms;
        hold_timeout_ms = (held_ms >= (double)max_hold_ms) ? 0 : (DWORD)((double)max_hold_ms - held_ms);
    }

    DWORD hold_wait_result = WaitForSingleObject(robust_semaphore->release_event, hold_timeout_ms);
    if (hold_wait_result == WAIT_TIMEOUT)
    {
        /*Codes_SRS_ROBUST_SEMAPHORE_01_046: [ If waiting for the release event times out then robust_semaphore_worker_thread shall release the mutex early by calling ReleaseMutex. ]*/
        LogError("Unit %" PRIu32 " was held for more than max_hold_ms=%" PRIu32 " ms (including the time spent in on_acquire_complete), releasing it early", unit_index, max_hold_ms);
    }
    else if (hold_wait_result != WAIT_OBJECT_0)
    {
        LogLastError("failure in WaitForSingleObject(release_event=%p, %lu) returned %lu, releasing unit %" PRIu32 "", robust_semaphore->release_event, hold_timeout_ms, hold_wait_result, unit_index);
    }
    else
    {
        /* do nothing, released */
    }

    /*Codes_SRS_ROBUST_SEMAPHORE_01_047: [ If a unit was acquired, robust_semaphore_worker_thread shall release the mutex by calling ReleaseMutex and, if the acquisition still holds the unit (it was not released), end it. ]*/
    if (!ReleaseMutex(robust_semaphore->mutexes[unit_index]))
    {
        LogLastError("failure in ReleaseMutex(mutex=%p) for unit %" PRIu32 "", robust_semaphore->mutexes[unit_index], unit_index);
    }
    else
    {
        /* do nothing, the unit is free */
    }

    srw_lock_ll_acquire_exclusive(&robust_semaphore->lock);
    if (robust_semaphore->acquisition_state != ACQUISITION_STATE_HOLDING)
    {
        /* do nothing, robust_semaphore_release already ended the acquisition */
    }
    else
    {
        robust_semaphore->acquisition_state = ACQUISITION_STATE_NONE;
    }
    srw_lock_ll_release_exclusive(&robust_semaphore->lock);
}

static void internal_run_acquisition(ROBUST_SEMAPHORE_HANDLE robust_semaphore, uint32_t timeout_ms, uint32_t max_hold_ms, ROBUST_SEMAPHORE_ON_ACQUIRE_COMPLETE on_acquire_complete, void* on_acquire_complete_context)
{
    HANDLE wait_handles[1 + ROBUST_SEMAPHORE_MAX_COUNT];
    wait_handles[0] = robust_semaphore->cancel_event;
    for (uint32_t i = 0; i < robust_semaphore->max_count; i++)
    {
        wait_handles[1 + i] = robust_semaphore->mutexes[i];
    }

    /*Codes_SRS_ROBUST_SEMAPHORE_01_039: [ Otherwise robust_semaphore_worker_thread shall wait for the cancel event or any of the mutexes by calling WaitForMultipleObjects with the cancel event followed by the max_count mutexes, bWaitAll FALSE and timeout_ms as timeout (INFINITE if timeout_ms is ROBUST_SEMAPHORE_NO_TIMEOUT). ]*/
    DWORD wait_result = WaitForMultipleObjects(1 + robust_semaphore->max_count, wait_handles, FALSE, (timeout_ms == ROBUST_SEMAPHORE_NO_TIMEOUT) ? INFINITE : timeout_ms);

    if (wait_result == WAIT_OBJECT_0)
    {
        /*Codes_SRS_ROBUST_SEMAPHORE_01_040: [ If WaitForMultipleObjects returns WAIT_OBJECT_0 then robust_semaphore_worker_thread shall end the acquisition and call on_acquire_complete with ROBUST_SEMAPHORE_ACQUIRE_CANCELLED and ROBUST_SEMAPHORE_UNIT_INDEX_NONE. ]*/
        internal_end_acquisition_without_unit(robust_semaphore, ROBUST_SEMAPHORE_ACQUIRE_CANCELLED, on_acquire_complete, on_acquire_complete_context);
    }
    else if (
        (wait_result >= WAIT_OBJECT_0 + 1) &&
        (wait_result < WAIT_OBJECT_0 + 1 + robust_semaphore->max_count)
        )
    {
        internal_hold_unit(robust_semaphore, wait_result - (WAIT_OBJECT_0 + 1), max_hold_ms, on_acquire_complete, on_acquire_complete_context);
    }
    else if (
        (wait_result >= WAIT_ABANDONED_0 + 1) &&
        (wait_result < WAIT_ABANDONED_0 + 1 + robust_semaphore->max_count)
        )
    {
        uint32_t unit_index = wait_result - (WAIT_ABANDONED_0 + 1);
        LogWarning("Unit %" PRIu32 " was abandoned by its previous owner (its thread or process ended without releasing it), acquiring it", unit_index);
        internal_hold_unit(robust_semaphore, unit_index, max_hold_ms, on_acquire_complete, on_acquire_complete_context);
    }
    else if (wait_result == WAIT_TIMEOUT)
    {
        /*Codes_SRS_ROBUST_SEMAPHORE_01_042: [ If WaitForMultipleObjects returns WAIT_TIMEOUT then robust_semaphore_worker_thread shall end the acquisition and call on_acquire_complete with ROBUST_SEMAPHORE_ACQUIRE_TIMEOUT and ROBUST_SEMAPHORE_UNIT_INDEX_NONE. ]*/
        LogWarning("Could not acquire one of the %" PRIu32 " units within %" PRIu32 " ms", robust_semaphore->max_count, timeout_ms);
        internal_end_acquisition_without_unit(robust_semaphore, ROBUST_SEMAPHORE_ACQUIRE_TIMEOUT, on_acquire_complete, on_acquire_complete_context);
    }
    else
    {
        /*Codes_SRS_ROBUST_SEMAPHORE_01_043: [ If WaitForMultipleObjects returns any other value then robust_semaphore_worker_thread shall end the acquisition and call on_acquire_complete with ROBUST_SEMAPHORE_ACQUIRE_ERROR and ROBUST_SEMAPHORE_UNIT_INDEX_NONE. ]*/
        LogLastError("failure in WaitForMultipleObjects(%" PRIu32 ", %p, FALSE, %" PRIu32 ") returned %lu", 1 + robust_semaphore->max_count, wait_handles, timeout_ms, wait_result);
        internal_end_acquisition_without_unit(robust_semaphore, ROBUST_SEMAPHORE_ACQUIRE_ERROR, on_acquire_complete, on_acquire_complete_context);
    }
}

static int robust_semaphore_worker_thread(void* context)
{
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = context;

    /*Codes_SRS_ROBUST_SEMAPHORE_01_036: [ robust_semaphore_worker_thread shall record the id of the worker thread by calling GetCurrentThreadId. ]*/
    (void)interlocked_exchange(&robust_semaphore->worker_thread_id, (int32_t)GetCurrentThreadId());

    bool worker_frees_object = false;
    THREAD_HANDLE worker_thread = NULL;
    bool exit_thread = false;
    while (!exit_thread)
    {
        srw_lock_ll_acquire_exclusive(&robust_semaphore->lock);
        if (robust_semaphore->acquisition_state == ACQUISITION_STATE_REQUESTED)
        {
            /*Codes_SRS_ROBUST_SEMAPHORE_01_037: [ If there is a new acquisition, robust_semaphore_worker_thread shall take it and reset the cancel event and the release event by calling ResetEvent. ]*/
            internal_reset_event(robust_semaphore->cancel_event, "cancel_event");
            internal_reset_event(robust_semaphore->release_event, "release_event");

            uint32_t timeout_ms = robust_semaphore->timeout_ms;
            uint32_t max_hold_ms = robust_semaphore->max_hold_ms;
            ROBUST_SEMAPHORE_ON_ACQUIRE_COMPLETE on_acquire_complete = robust_semaphore->on_acquire_complete;
            void* on_acquire_complete_context = robust_semaphore->on_acquire_complete_context;

            if (
                robust_semaphore->acquisition_cancelled ||
                robust_semaphore->acquisition_released
                )
            {
                /*Codes_SRS_ROBUST_SEMAPHORE_01_038: [ If the acquisition was already cancelled or released, robust_semaphore_worker_thread shall end it and call on_acquire_complete with ROBUST_SEMAPHORE_ACQUIRE_CANCELLED and ROBUST_SEMAPHORE_UNIT_INDEX_NONE without waiting for a unit. ]*/
                robust_semaphore->acquisition_state = ACQUISITION_STATE_NONE;
                srw_lock_ll_release_exclusive(&robust_semaphore->lock);

                on_acquire_complete(on_acquire_complete_context, ROBUST_SEMAPHORE_ACQUIRE_CANCELLED, ROBUST_SEMAPHORE_UNIT_INDEX_NONE);
            }
            else
            {
                robust_semaphore->acquisition_state = ACQUISITION_STATE_WAITING;
                srw_lock_ll_release_exclusive(&robust_semaphore->lock);

                internal_run_acquisition(robust_semaphore, timeout_ms, max_hold_ms, on_acquire_complete, on_acquire_complete_context);
            }
        }
        else
        {
            /*Codes_SRS_ROBUST_SEMAPHORE_01_070: [ If there is no new acquisition, robust_semaphore_worker_thread shall, while still holding the lock, mark the worker thread as not running. ]*/
            /* thread ids are reused: a stale id would make robust_semaphore_destroy called on an unrelated thread look like it is called on the worker thread */
            robust_semaphore->worker_running = false;
            (void)interlocked_exchange(&robust_semaphore->worker_thread_id, 0);
            worker_frees_object = robust_semaphore->worker_frees_object;
            worker_thread = robust_semaphore->worker_thread;

            /*Codes_SRS_ROBUST_SEMAPHORE_01_071: [ robust_semaphore_worker_thread shall then release the lock and return 0 without taking the lock again. ]*/
            /* robust_semaphore_acquire_async and robust_semaphore_destroy may wait for this thread to end while holding the lock */
            srw_lock_ll_release_exclusive(&robust_semaphore->lock);
            exit_thread = true;
        }
    }

    if (!worker_frees_object)
    {
        /* do nothing, robust_semaphore_acquire_async or robust_semaphore_destroy waits for this thread */
    }
    else
    {
        /*Codes_SRS_ROBUST_SEMAPHORE_01_049: [ If robust_semaphore_destroy was called on the worker thread, before returning robust_semaphore_worker_thread shall close its own thread handle by calling CloseHandle. ]*/
        /* nobody joins this thread, on Windows THREAD_HANDLE is the thread HANDLE */
        internal_close_handle((HANDLE)worker_thread, "worker_thread");

        /*Codes_SRS_ROBUST_SEMAPHORE_01_060: [ If robust_semaphore_destroy was called on the worker thread, before returning robust_semaphore_worker_thread shall free the robust semaphore object as robust_semaphore_destroy does. ]*/
        internal_free_robust_semaphore(robust_semaphore);
    }

    return 0;
}

static int internal_start_worker_thread_under_lock(ROBUST_SEMAPHORE_HANDLE robust_semaphore)
{
    int result;
    int thread_result;

    if (
        (robust_semaphore->worker_thread != NULL) &&
        /*Codes_SRS_ROBUST_SEMAPHORE_01_067: [ if a worker thread was started before, wait for it to end by calling ThreadAPI_Join. ]*/
        /* it is not running: it found no new acquisition and does not take the lock again, so it can be waited for while holding the lock */
        (ThreadAPI_Join(robust_semaphore->worker_thread, &thread_result) != THREADAPI_OK)
        )
    {
        /*Codes_SRS_ROBUST_SEMAPHORE_01_069: [ If ThreadAPI_Join or ThreadAPI_Create fails, robust_semaphore_acquire_async shall forget the new acquisition and the worker thread, release the lock and return a non-zero value. ]*/
        /* ThreadAPI_Join closes the thread handle even when it fails, so it cannot be waited for again */
        LogError("failure in ThreadAPI_Join(worker_thread=%p) waiting for the previous worker thread of robust_semaphore=%p", robust_semaphore->worker_thread, robust_semaphore);
        robust_semaphore->worker_thread = NULL;
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_ROBUST_SEMAPHORE_01_068: [ start a new worker thread by calling ThreadAPI_Create with robust_semaphore_worker_thread and mark it as running. ]*/
        if (ThreadAPI_Create(&robust_semaphore->worker_thread, robust_semaphore_worker_thread, robust_semaphore) != THREADAPI_OK)
        {
            /*Codes_SRS_ROBUST_SEMAPHORE_01_069: [ If ThreadAPI_Join or ThreadAPI_Create fails, robust_semaphore_acquire_async shall forget the new acquisition and the worker thread, release the lock and return a non-zero value. ]*/
            LogError("failure in ThreadAPI_Create(&robust_semaphore->worker_thread=%p, robust_semaphore_worker_thread, robust_semaphore=%p)", &robust_semaphore->worker_thread, robust_semaphore);
            robust_semaphore->worker_thread = NULL;
            result = MU_FAILURE;
        }
        else
        {
            robust_semaphore->worker_running = true;
            result = 0;
        }
    }

    return result;
}

ROBUST_SEMAPHORE_HANDLE robust_semaphore_create(const char* name, uint32_t max_count, const char* security_descriptor)
{
    ROBUST_SEMAPHORE_HANDLE result;

    if (
        /*Codes_SRS_ROBUST_SEMAPHORE_01_001: [ If name is NULL then robust_semaphore_create shall fail and return NULL. ]*/
        (name == NULL) ||
        /*Codes_SRS_ROBUST_SEMAPHORE_01_002: [ If max_count is 0 then robust_semaphore_create shall fail and return NULL. ]*/
        (max_count == 0) ||
        /*Codes_SRS_ROBUST_SEMAPHORE_01_003: [ If max_count is greater than ROBUST_SEMAPHORE_MAX_COUNT then robust_semaphore_create shall fail and return NULL. ]*/
        (max_count > ROBUST_SEMAPHORE_MAX_COUNT)
        )
    {
        LogError("Invalid arguments: const char* name=%s, uint32_t max_count=%" PRIu32 ", const char* security_descriptor=%s",
            MU_P_OR_NULL(name), max_count, MU_P_OR_NULL(security_descriptor));
        result = NULL;
    }
    else
    {
        /*Codes_SRS_ROBUST_SEMAPHORE_01_004: [ robust_semaphore_create shall allocate memory for the robust semaphore object. ]*/
        result = malloc(sizeof(ROBUST_SEMAPHORE));
        if (result == NULL)
        {
            /*Codes_SRS_ROBUST_SEMAPHORE_01_013: [ If there are any failures then robust_semaphore_create shall close the handles it created by calling CloseHandle, free the memory it allocated and return NULL. ]*/
            LogError("failure in malloc(sizeof(ROBUST_SEMAPHORE)=%zu)", sizeof(ROBUST_SEMAPHORE));
        }
        else
        {
            /*Codes_SRS_ROBUST_SEMAPHORE_01_005: [ robust_semaphore_create shall initialize a lock by calling srw_lock_ll_init. ]*/
            if (srw_lock_ll_init(&result->lock) != 0)
            {
                /*Codes_SRS_ROBUST_SEMAPHORE_01_013: [ If there are any failures then robust_semaphore_create shall close the handles it created by calling CloseHandle, free the memory it allocated and return NULL. ]*/
                LogError("failure in srw_lock_ll_init(&result->lock=%p)", &result->lock);
            }
            else
            {
                PSECURITY_DESCRIPTOR converted_security_descriptor = NULL;

                if (
                    (security_descriptor != NULL) &&
                    /*Codes_SRS_ROBUST_SEMAPHORE_01_006: [ If security_descriptor is not NULL, robust_semaphore_create shall convert it by calling ConvertStringSecurityDescriptorToSecurityDescriptorA. ]*/
                    !ConvertStringSecurityDescriptorToSecurityDescriptorA(security_descriptor, SDDL_REVISION_1, &converted_security_descriptor, NULL)
                    )
                {
                    /*Codes_SRS_ROBUST_SEMAPHORE_01_013: [ If there are any failures then robust_semaphore_create shall close the handles it created by calling CloseHandle, free the memory it allocated and return NULL. ]*/
                    LogLastError("failure in ConvertStringSecurityDescriptorToSecurityDescriptorA(%s)", security_descriptor);
                }
                else
                {
                    SECURITY_ATTRIBUTES security_attributes;
                    security_attributes.nLength = sizeof(security_attributes);
                    security_attributes.lpSecurityDescriptor = converted_security_descriptor;
                    security_attributes.bInheritHandle = FALSE;

                    uint32_t i;
                    /*Codes_SRS_ROBUST_SEMAPHORE_01_054: [ For each unit i, robust_semaphore_create shall: ]*/
                    for (i = 0; i < max_count; i++)
                    {
                        /*Codes_SRS_ROBUST_SEMAPHORE_01_007: [ build the mutex name <name>_<i> by calling sprintf_char. ]*/
                        char* mutex_name = sprintf_char("%s_%" PRIu32 "", name, i);
                        if (mutex_name == NULL)
                        {
                            /*Codes_SRS_ROBUST_SEMAPHORE_01_013: [ If there are any failures then robust_semaphore_create shall close the handles it created by calling CloseHandle, free the memory it allocated and return NULL. ]*/
                            LogError("failure in sprintf_char building the name of mutex %" PRIu32 " of %s", i, name);
                            break;
                        }
                        else
                        {
                            /*Codes_SRS_ROBUST_SEMAPHORE_01_008: [ create or open the mutex by calling CreateMutexExA with the converted security descriptor, the mutex name and SYNCHRONIZE | MUTEX_MODIFY_STATE access. ]*/
                            result->mutexes[i] = CreateMutexExA(&security_attributes, mutex_name, 0, SYNCHRONIZE | MUTEX_MODIFY_STATE);
                            if (result->mutexes[i] == NULL)
                            {
                                /*Codes_SRS_ROBUST_SEMAPHORE_01_013: [ If there are any failures then robust_semaphore_create shall close the handles it created by calling CloseHandle, free the memory it allocated and return NULL. ]*/
                                LogLastError("failure in CreateMutexExA(%s)", mutex_name);
                                /*Codes_SRS_ROBUST_SEMAPHORE_01_055: [ free the mutex name. ]*/
                                free(mutex_name);
                                break;
                            }
                            else
                            {
                                /*Codes_SRS_ROBUST_SEMAPHORE_01_055: [ free the mutex name. ]*/
                                free(mutex_name);
                            }
                        }
                    }

                    if (converted_security_descriptor == NULL)
                    {
                        /* do nothing, the mutexes have the default security descriptor */
                    }
                    else
                    {
                        /*Codes_SRS_ROBUST_SEMAPHORE_01_009: [ robust_semaphore_create shall free the converted security descriptor by calling LocalFree. ]*/
                        if (LocalFree(converted_security_descriptor) != NULL)
                        {
                            LogLastError("failure in LocalFree(converted_security_descriptor=%p)", converted_security_descriptor);
                        }
                        else
                        {
                            /* do nothing, freed */
                        }
                    }

                    if (i < max_count)
                    {
                        /* do nothing, close the mutexes that were created below */
                    }
                    else
                    {
                        /*Codes_SRS_ROBUST_SEMAPHORE_01_056: [ robust_semaphore_create shall create a manual reset cancel event by calling CreateEventA. ]*/
                        result->cancel_event = CreateEventA(NULL, TRUE, FALSE, NULL);
                        if (result->cancel_event == NULL)
                        {
                            /*Codes_SRS_ROBUST_SEMAPHORE_01_013: [ If there are any failures then robust_semaphore_create shall close the handles it created by calling CloseHandle, free the memory it allocated and return NULL. ]*/
                            LogLastError("failure in CreateEventA(NULL, TRUE, FALSE, NULL) for the cancel event");
                        }
                        else
                        {
                            /*Codes_SRS_ROBUST_SEMAPHORE_01_057: [ robust_semaphore_create shall create a manual reset release event by calling CreateEventA. ]*/
                            result->release_event = CreateEventA(NULL, TRUE, FALSE, NULL);
                            if (result->release_event == NULL)
                            {
                                /*Codes_SRS_ROBUST_SEMAPHORE_01_013: [ If there are any failures then robust_semaphore_create shall close the handles it created by calling CloseHandle, free the memory it allocated and return NULL. ]*/
                                LogLastError("failure in CreateEventA(NULL, TRUE, FALSE, NULL) for the release event");
                            }
                            else
                            {
                                result->max_count = max_count;
                                (void)interlocked_exchange(&result->worker_thread_id, 0);
                                result->worker_thread = NULL;
                                result->worker_running = false;
                                result->destroying = false;
                                result->worker_frees_object = false;
                                result->acquisition_state = ACQUISITION_STATE_NONE;
                                result->acquisition_cancelled = false;
                                result->acquisition_released = false;

                                /*Codes_SRS_ROBUST_SEMAPHORE_01_012: [ robust_semaphore_create shall succeed and return the robust semaphore object. ]*/
                                goto all_ok;
                            }

                            internal_close_handle(result->cancel_event, "cancel_event");
                        }
                    }

                    for (uint32_t j = 0; j < i; j++)
                    {
                        internal_close_handle(result->mutexes[j], "mutex");
                    }
                }

                srw_lock_ll_deinit(&result->lock);
            }

            free(result);
        }

        result = NULL;
    }

all_ok:
    return result;
}

void robust_semaphore_destroy(ROBUST_SEMAPHORE_HANDLE robust_semaphore)
{
    if (robust_semaphore == NULL)
    {
        /*Codes_SRS_ROBUST_SEMAPHORE_01_014: [ If robust_semaphore is NULL then robust_semaphore_destroy shall return. ]*/
        LogError("Invalid arguments: ROBUST_SEMAPHORE_HANDLE robust_semaphore=%p", robust_semaphore);
    }
    else
    {
        /*Codes_SRS_ROBUST_SEMAPHORE_01_015: [ robust_semaphore_destroy shall acquire the lock in exclusive mode. ]*/
        srw_lock_ll_acquire_exclusive(&robust_semaphore->lock);

        /*Codes_SRS_ROBUST_SEMAPHORE_01_061: [ robust_semaphore_destroy shall mark the object as being destroyed and release the current acquisition as robust_semaphore_release does. ]*/
        robust_semaphore->destroying = true;
        internal_release_under_lock(robust_semaphore);

        if (
            robust_semaphore->worker_running &&
            ((DWORD)interlocked_add(&robust_semaphore->worker_thread_id, 0) == GetCurrentThreadId())
            )
        {
            /*Codes_SRS_ROBUST_SEMAPHORE_01_017: [ If robust_semaphore_destroy is called on the worker thread, it shall mark the worker thread as the one that frees the object, release the lock and return. ]*/
            LogError("robust_semaphore_destroy(robust_semaphore=%p) was called from on_acquire_complete, the worker thread frees the object when on_acquire_complete returns", robust_semaphore);
            robust_semaphore->worker_frees_object = true;
            srw_lock_ll_release_exclusive(&robust_semaphore->lock);
        }
        else
        {
            /*Codes_SRS_ROBUST_SEMAPHORE_01_062: [ Otherwise robust_semaphore_destroy shall release the lock and, if a worker thread was started, wait for it to end by calling ThreadAPI_Join. ]*/
            /* a running worker thread ends once it completed the released acquisition, because it finds no new acquisition */
            THREAD_HANDLE worker_thread = robust_semaphore->worker_thread;
            srw_lock_ll_release_exclusive(&robust_semaphore->lock);

            int thread_result;
            if (
                (worker_thread != NULL) &&
                (ThreadAPI_Join(worker_thread, &thread_result) != THREADAPI_OK)
                )
            {
                /*Codes_SRS_ROBUST_SEMAPHORE_01_053: [ If ThreadAPI_Join fails, robust_semaphore_destroy shall return without freeing the object, because the worker thread may still use it. ]*/
                LogError("failure in ThreadAPI_Join(worker_thread=%p), not freeing robust_semaphore=%p because its worker thread may still use it", worker_thread, robust_semaphore);
            }
            else
            {
                /* no worker thread was started, or it ended: the mutexes and the events are closed and the object is freed by internal_free_robust_semaphore */
                internal_free_robust_semaphore(robust_semaphore);
            }
        }
    }
}

int robust_semaphore_acquire_async(ROBUST_SEMAPHORE_HANDLE robust_semaphore, uint32_t timeout_ms, uint32_t max_hold_ms, ROBUST_SEMAPHORE_ON_ACQUIRE_COMPLETE on_acquire_complete, void* on_acquire_complete_context)
{
    int result;

    if (
        /*Codes_SRS_ROBUST_SEMAPHORE_01_020: [ If robust_semaphore is NULL then robust_semaphore_acquire_async shall fail and return a non-zero value. ]*/
        (robust_semaphore == NULL) ||
        /*Codes_SRS_ROBUST_SEMAPHORE_01_021: [ If on_acquire_complete is NULL then robust_semaphore_acquire_async shall fail and return a non-zero value. ]*/
        (on_acquire_complete == NULL)
        )
    {
        LogError("Invalid arguments: ROBUST_SEMAPHORE_HANDLE robust_semaphore=%p, uint32_t timeout_ms=%" PRIu32 ", uint32_t max_hold_ms=%" PRIu32 ", ROBUST_SEMAPHORE_ON_ACQUIRE_COMPLETE on_acquire_complete=%p, void* on_acquire_complete_context=%p",
            robust_semaphore, timeout_ms, max_hold_ms, on_acquire_complete, on_acquire_complete_context);
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_ROBUST_SEMAPHORE_01_022: [ robust_semaphore_acquire_async shall acquire the lock in exclusive mode. ]*/
        srw_lock_ll_acquire_exclusive(&robust_semaphore->lock);

        if (robust_semaphore->destroying)
        {
            /*Codes_SRS_ROBUST_SEMAPHORE_01_063: [ If the object is being destroyed, robust_semaphore_acquire_async shall fail and return a non-zero value. ]*/
            LogError("Cannot acquire robust_semaphore=%p, it is being destroyed", robust_semaphore);
            result = MU_FAILURE;
        }
        else if (robust_semaphore->acquisition_state != ACQUISITION_STATE_NONE)
        {
            /*Codes_SRS_ROBUST_SEMAPHORE_01_024: [ If there is a current acquisition that was not released or ended, robust_semaphore_acquire_async shall fail and return a non-zero value. ]*/
            LogError("Cannot acquire robust_semaphore=%p, the current acquisition (state %" PRI_MU_ENUM ") did not end", robust_semaphore, MU_ENUM_VALUE(ACQUISITION_STATE, robust_semaphore->acquisition_state));
            result = MU_FAILURE;
        }
        else
        {
            /*Codes_SRS_ROBUST_SEMAPHORE_01_064: [ robust_semaphore_acquire_async shall store timeout_ms, max_hold_ms, on_acquire_complete and on_acquire_complete_context as the new current acquisition. ]*/
            robust_semaphore->timeout_ms = timeout_ms;
            robust_semaphore->max_hold_ms = max_hold_ms;
            robust_semaphore->on_acquire_complete = on_acquire_complete;
            robust_semaphore->on_acquire_complete_context = on_acquire_complete_context;
            robust_semaphore->acquisition_cancelled = false;
            robust_semaphore->acquisition_released = false;
            robust_semaphore->acquisition_state = ACQUISITION_STATE_REQUESTED;

            if (robust_semaphore->worker_running)
            {
                /*Codes_SRS_ROBUST_SEMAPHORE_01_065: [ If the worker thread is running, robust_semaphore_acquire_async shall leave the new acquisition to it. ]*/
                /* the worker thread takes it once its current acquisition ended, for example when robust_semaphore_acquire_async is called from on_acquire_complete */
                /*Codes_SRS_ROBUST_SEMAPHORE_01_026: [ robust_semaphore_acquire_async shall release the lock and succeed and return 0. ]*/
                result = 0;
            }
            /*Codes_SRS_ROBUST_SEMAPHORE_01_066: [ Otherwise robust_semaphore_acquire_async shall: ]*/
            else if (internal_start_worker_thread_under_lock(robust_semaphore) != 0)
            {
                /*Codes_SRS_ROBUST_SEMAPHORE_01_069: [ If ThreadAPI_Join or ThreadAPI_Create fails, robust_semaphore_acquire_async shall forget the new acquisition and the worker thread, release the lock and return a non-zero value. ]*/
                robust_semaphore->acquisition_state = ACQUISITION_STATE_NONE;
                result = MU_FAILURE;
            }
            else
            {
                /*Codes_SRS_ROBUST_SEMAPHORE_01_026: [ robust_semaphore_acquire_async shall release the lock and succeed and return 0. ]*/
                result = 0;
            }
        }

        srw_lock_ll_release_exclusive(&robust_semaphore->lock);
    }

    return result;
}

void robust_semaphore_cancel(ROBUST_SEMAPHORE_HANDLE robust_semaphore)
{
    if (robust_semaphore == NULL)
    {
        /*Codes_SRS_ROBUST_SEMAPHORE_01_028: [ If robust_semaphore is NULL then robust_semaphore_cancel shall return. ]*/
        LogError("Invalid arguments: ROBUST_SEMAPHORE_HANDLE robust_semaphore=%p", robust_semaphore);
    }
    else
    {
        /*Codes_SRS_ROBUST_SEMAPHORE_01_029: [ robust_semaphore_cancel shall acquire the lock in exclusive mode. ]*/
        srw_lock_ll_acquire_exclusive(&robust_semaphore->lock);

        if (
            (robust_semaphore->acquisition_state != ACQUISITION_STATE_REQUESTED) &&
            (robust_semaphore->acquisition_state != ACQUISITION_STATE_WAITING)
            )
        {
            /* do nothing, no acquisition is waiting for a unit */
        }
        else
        {
            /*Codes_SRS_ROBUST_SEMAPHORE_01_030: [ If the current acquisition was not released and did not get a unit, robust_semaphore_cancel shall mark it as cancelled and signal the cancel event by calling SetEvent. ]*/
            robust_semaphore->acquisition_cancelled = true;
            internal_set_event(robust_semaphore->cancel_event, "cancel_event");
        }

        /*Codes_SRS_ROBUST_SEMAPHORE_01_031: [ robust_semaphore_cancel shall release the lock. ]*/
        srw_lock_ll_release_exclusive(&robust_semaphore->lock);
    }
}

void robust_semaphore_release(ROBUST_SEMAPHORE_HANDLE robust_semaphore)
{
    if (robust_semaphore == NULL)
    {
        /*Codes_SRS_ROBUST_SEMAPHORE_01_032: [ If robust_semaphore is NULL then robust_semaphore_release shall return. ]*/
        LogError("Invalid arguments: ROBUST_SEMAPHORE_HANDLE robust_semaphore=%p", robust_semaphore);
    }
    else
    {
        /*Codes_SRS_ROBUST_SEMAPHORE_01_033: [ robust_semaphore_release shall acquire the lock in exclusive mode. ]*/
        srw_lock_ll_acquire_exclusive(&robust_semaphore->lock);

        internal_release_under_lock(robust_semaphore);

        /*Codes_SRS_ROBUST_SEMAPHORE_01_035: [ robust_semaphore_release shall release the lock. ]*/
        srw_lock_ll_release_exclusive(&robust_semaphore->lock);
    }
}
