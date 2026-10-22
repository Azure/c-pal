// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>

#include "windows.h"

#include "testrunnerswitcher.h"

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_pal/srw_lock_ll.h"
#include "c_pal/threadapi.h"
#include "c_pal/timer.h"

#include "c_pal/robust_semaphore.h"

/* Chaos tests for robust_semaphore: for a while, owner threads acquire, hold, release, destroy and recreate robust semaphore objects
that share the same units, while canceller threads cancel their acquisitions at random. The tests check that:
- no more than max_count units are held at the same time and no unit is held twice,
- every acquisition gets exactly one on_acquire_complete with a valid result and unit index,
- robust_semaphore_destroy calls the pending on_acquire_complete before it returns,
- nothing crashes or hangs and all the units are free at the end. */

#define CHAOS_TEST_DURATION_MS 10000
#define CHAOS_OWNER_COUNT 8
#define CHAOS_CANCELLER_COUNT 2
#define CHAOS_MAX_COUNT 3
#define CHAOS_WAIT_TIMEOUT_MS 10000
#define CHAOS_UNIT_PROBE_INTERVAL_MS 20

#define CHAOS_VIOLATION_VALUES \
    CHAOS_VIOLATION_NONE, \
    CHAOS_VIOLATION_MORE_HOLDERS_THAN_MAX_COUNT, \
    CHAOS_VIOLATION_UNIT_HELD_TWICE, \
    CHAOS_VIOLATION_UNIT_NOT_HELD, \
    CHAOS_VIOLATION_INVALID_RESULT, \
    CHAOS_VIOLATION_INVALID_UNIT_INDEX, \
    CHAOS_VIOLATION_CALLBACK_NOT_CALLED_BY_DESTROY, \
    CHAOS_VIOLATION_STILL_HOLDING_AFTER_DESTROY, \
    CHAOS_VIOLATION_CREATE_FAILED

MU_DEFINE_ENUM(CHAOS_VIOLATION, CHAOS_VIOLATION_VALUES);
MU_DEFINE_ENUM_STRINGS(CHAOS_VIOLATION, CHAOS_VIOLATION_VALUES);

TEST_DEFINE_ENUM_TYPE(CHAOS_VIOLATION, CHAOS_VIOLATION_VALUES);

typedef struct CHAOS_TEST_TAG CHAOS_TEST;

/* a robust semaphore object: its owner thread acquires, releases, destroys and recreates it, the canceller threads cancel its acquisitions */
typedef struct CHAOS_SLOT_TAG
{
    CHAOS_TEST* test;
    uint32_t index;

    /* exclusive: the owner destroys and recreates the robust semaphore, shared: a canceller cancels its acquisition */
    SRW_LOCK_LL lock;
    ROBUST_SEMAPHORE_HANDLE robust_semaphore;

    volatile_atomic int32_t started;            /* successful robust_semaphore_acquire_async calls (incremented before the call) */
    volatile_atomic int32_t completed;          /* on_acquire_complete calls, incremented last in on_acquire_complete */
    volatile_atomic int32_t last_result;        /* ROBUST_SEMAPHORE_ACQUIRE_RESULT of the last on_acquire_complete */
    volatile_atomic int32_t held_unit;
    volatile_atomic int32_t holding;            /* 1 while this slot is counted as the holder of held_unit */
    volatile_atomic int32_t release_requested;  /* set by the owner before it releases or destroys the current acquisition */
} CHAOS_SLOT;

struct CHAOS_TEST_TAG
{
    char name[96];
    uint32_t max_count;
    /* when max_hold_ms is used the worker thread releases units without telling the user, so the holders cannot be counted */
    bool use_max_hold;

    volatile_atomic int32_t stop;
    volatile_atomic int32_t violation_count;
    volatile_atomic int32_t first_violation;

    /* a holder is counted between on_acquire_complete(OK) and the owner releasing it, which is inside the time its
    mutex is owned, so the counted holders can never be more than max_count */
    volatile_atomic int32_t holders;
    volatile_atomic int32_t max_holders;
    volatile_atomic int32_t unit_holders[ROBUST_SEMAPHORE_MAX_COUNT];

    volatile_atomic int32_t ok_count;
    volatile_atomic int32_t timeout_count;
    volatile_atomic int32_t cancelled_count;
    volatile_atomic int32_t error_count;
    volatile_atomic int32_t refused_count;
    volatile_atomic int32_t release_count;
    volatile_atomic int32_t cancel_count;
    volatile_atomic int32_t destroy_count;

    CHAOS_SLOT slots[CHAOS_OWNER_COUNT];
};

static void record_violation(CHAOS_TEST* test, CHAOS_VIOLATION violation)
{
    (void)interlocked_increment(&test->violation_count);
    (void)interlocked_compare_exchange(&test->first_violation, (int32_t)violation, (int32_t)CHAOS_VIOLATION_NONE);
}

static void count_holder(CHAOS_SLOT* slot, uint32_t unit_index)
{
    CHAOS_TEST* test = slot->test;

    if (interlocked_compare_exchange(&test->unit_holders[unit_index], 1, 0) != 0)
    {
        record_violation(test, CHAOS_VIOLATION_UNIT_HELD_TWICE);
    }
    else
    {
        /* do nothing, nobody else holds this unit */
    }

    int32_t holders = interlocked_increment(&test->holders);
    if (holders > (int32_t)test->max_count)
    {
        record_violation(test, CHAOS_VIOLATION_MORE_HOLDERS_THAN_MAX_COUNT);
    }
    else
    {
        /* do nothing, within the limit */
    }

    int32_t current_max;
    while ((current_max = interlocked_add(&test->max_holders, 0)) < holders)
    {
        (void)interlocked_compare_exchange(&test->max_holders, holders, current_max);
    }
}

static void uncount_holder(CHAOS_SLOT* slot)
{
    CHAOS_TEST* test = slot->test;
    uint32_t unit_index = (uint32_t)interlocked_add(&slot->held_unit, 0);

    if (interlocked_compare_exchange(&test->unit_holders[unit_index], 0, 1) != 1)
    {
        record_violation(test, CHAOS_VIOLATION_UNIT_NOT_HELD);
    }
    else
    {
        /* do nothing, the unit was held by this slot */
    }

    (void)interlocked_decrement(&test->holders);
}

static void on_chaos_acquire_complete(void* context, ROBUST_SEMAPHORE_ACQUIRE_RESULT result, uint32_t unit_index)
{
    CHAOS_SLOT* slot = context;
    CHAOS_TEST* test = slot->test;

    switch (result)
    {
        case ROBUST_SEMAPHORE_ACQUIRE_OK:
            (void)interlocked_increment(&test->ok_count);
            if (unit_index >= test->max_count)
            {
                record_violation(test, CHAOS_VIOLATION_INVALID_UNIT_INDEX);
            }
            else if (test->use_max_hold)
            {
                /* a slow callback can run past max_hold_ms */
                if (rand() % 4 == 0)
                {
                    Sleep((DWORD)(rand() % 5));
                }
                else
                {
                    /* do nothing, return right away */
                }
            }
            else
            {
                (void)interlocked_exchange(&slot->held_unit, (int32_t)unit_index);
                count_holder(slot, unit_index);
                (void)interlocked_exchange(&slot->holding, 1);

                if (
                    (interlocked_add(&slot->release_requested, 0) != 0) &&
                    (interlocked_exchange(&slot->holding, 0) == 1)
                    )
                {
                    /* the owner released the acquisition while it was waiting, the worker thread gives the unit back when this returns */
                    uncount_holder(slot);
                }
                else
                {
                    /* do nothing, the owner releases the unit later */
                }
            }
            break;
        case ROBUST_SEMAPHORE_ACQUIRE_TIMEOUT:
            (void)interlocked_increment(&test->timeout_count);
            break;
        case ROBUST_SEMAPHORE_ACQUIRE_CANCELLED:
            (void)interlocked_increment(&test->cancelled_count);
            break;
        case ROBUST_SEMAPHORE_ACQUIRE_ERROR:
            (void)interlocked_increment(&test->error_count);
            break;
        default:
            record_violation(test, CHAOS_VIOLATION_INVALID_RESULT);
            break;
    }

    if (
        (result != ROBUST_SEMAPHORE_ACQUIRE_OK) &&
        (unit_index != ROBUST_SEMAPHORE_UNIT_INDEX_NONE)
        )
    {
        record_violation(test, CHAOS_VIOLATION_INVALID_UNIT_INDEX);
    }
    else
    {
        /* do nothing, the unit index matches the result */
    }

    (void)interlocked_exchange(&slot->last_result, (int32_t)result);
    (void)interlocked_increment(&slot->completed);
}

static uint32_t pick_timeout_ms(void)
{
    uint32_t result;
    int r = rand() % 10;
    if (r < 3)
    {
        result = ROBUST_SEMAPHORE_NO_TIMEOUT;
    }
    else if (r < 5)
    {
        result = 0;
    }
    else
    {
        result = 1 + (uint32_t)(rand() % 20);
    }
    return result;
}

static uint32_t pick_max_hold_ms(CHAOS_TEST* test)
{
    uint32_t result;
    if (
        !test->use_max_hold ||
        (rand() % 3 == 0)
        )
    {
        result = ROBUST_SEMAPHORE_NO_TIMEOUT;
    }
    else
    {
        result = (uint32_t)(rand() % 10);
    }
    return result;
}

/* returns true if the acquisition was started */
static bool chaos_acquire(CHAOS_SLOT* slot)
{
    bool result;
    CHAOS_TEST* test = slot->test;

    (void)interlocked_exchange(&slot->release_requested, 0);
    /* on_acquire_complete can run before robust_semaphore_acquire_async returns */
    (void)interlocked_increment(&slot->started);
    if (robust_semaphore_acquire_async(slot->robust_semaphore, pick_timeout_ms(), pick_max_hold_ms(test), on_chaos_acquire_complete, slot) != 0)
    {
        /* a unit acquired while its acquisition was being released is given back only after on_acquire_complete returns, until then a new acquisition is refused */
        (void)interlocked_decrement(&slot->started);
        (void)interlocked_increment(&test->refused_count);
        result = false;
    }
    else
    {
        result = true;
    }

    return result;
}

static void stop_counting_the_current_acquisition(CHAOS_SLOT* slot)
{
    (void)interlocked_exchange(&slot->release_requested, 1);
    if (interlocked_exchange(&slot->holding, 0) == 1)
    {
        uncount_holder(slot);
    }
    else
    {
        /* do nothing, the slot is not counted as a holder */
    }
}

static void chaos_release(CHAOS_SLOT* slot)
{
    stop_counting_the_current_acquisition(slot);
    robust_semaphore_release(slot->robust_semaphore);
    (void)interlocked_increment(&slot->test->release_count);
}

/* returns false if a new robust semaphore could not be created */
static bool chaos_destroy(CHAOS_SLOT* slot, bool recreate)
{
    bool result;
    CHAOS_TEST* test = slot->test;

    stop_counting_the_current_acquisition(slot);

    srw_lock_ll_acquire_exclusive(&slot->lock);

    robust_semaphore_destroy(slot->robust_semaphore);
    slot->robust_semaphore = NULL;
    (void)interlocked_increment(&test->destroy_count);

    if (interlocked_add(&slot->completed, 0) != interlocked_add(&slot->started, 0))
    {
        record_violation(test, CHAOS_VIOLATION_CALLBACK_NOT_CALLED_BY_DESTROY);
    }
    else
    {
        /* do nothing, every acquisition got its on_acquire_complete */
    }

    if (interlocked_add(&slot->holding, 0) != 0)
    {
        record_violation(test, CHAOS_VIOLATION_STILL_HOLDING_AFTER_DESTROY);
    }
    else
    {
        /* do nothing, the destroyed object holds no unit */
    }

    if (!recreate)
    {
        result = true;
    }
    else
    {
        slot->robust_semaphore = robust_semaphore_create(test->name, test->max_count, NULL);
        if (slot->robust_semaphore == NULL)
        {
            record_violation(test, CHAOS_VIOLATION_CREATE_FAILED);
            result = false;
        }
        else
        {
            result = true;
        }
    }

    srw_lock_ll_release_exclusive(&slot->lock);

    return result;
}

static int chaos_owner_thread(void* context)
{
    CHAOS_SLOT* slot = context;
    CHAOS_TEST* test = slot->test;
    /* the owner's view of its acquisition: false once it was released, or if there is none */
    bool acquisition_active = false;

    srand((unsigned int)time(NULL) ^ (slot->index * 7919));

    while (interlocked_add(&test->stop, 0) == 0)
    {
        int r = rand() % 100;
        bool callback_pending = (interlocked_add(&slot->completed, 0) != interlocked_add(&slot->started, 0));

        if (r < 2)
        {
            /* destroy while idle, waiting or holding */
            acquisition_active = false;
            if (!chaos_destroy(slot, true))
            {
                break;
            }
            else
            {
                /* do nothing, a new robust semaphore object is ready */
            }
        }
        else if (callback_pending)
        {
            /* waiting for a unit */
            if (
                acquisition_active &&
                (r < 5)
                )
            {
                acquisition_active = false;
                chaos_release(slot);
            }
            else
            {
                Sleep((DWORD)(rand() % 2));
            }
        }
        else if (
            acquisition_active &&
            (interlocked_add(&slot->last_result, 0) == ROBUST_SEMAPHORE_ACQUIRE_OK)
            )
        {
            /* holding a unit */
            Sleep((DWORD)(rand() % 3));
            acquisition_active = false;
            chaos_release(slot);
        }
        else if (r < 90)
        {
            acquisition_active = chaos_acquire(slot);
        }
        else
        {
            Sleep((DWORD)(rand() % 2));
        }
    }

    if (slot->robust_semaphore == NULL)
    {
        /* do nothing, a new robust semaphore could not be created */
    }
    else
    {
        (void)chaos_destroy(slot, false);
    }

    return 0;
}

static int chaos_canceller_thread(void* context)
{
    CHAOS_TEST* test = context;

    srand((unsigned int)time(NULL) ^ (unsigned int)GetCurrentThreadId());

    while (interlocked_add(&test->stop, 0) == 0)
    {
        CHAOS_SLOT* slot = &test->slots[rand() % CHAOS_OWNER_COUNT];

        srw_lock_ll_acquire_shared(&slot->lock);
        if (slot->robust_semaphore == NULL)
        {
            /* do nothing, the owner is replacing the robust semaphore */
        }
        else
        {
            robust_semaphore_cancel(slot->robust_semaphore);
            (void)interlocked_increment(&test->cancel_count);
        }
        srw_lock_ll_release_shared(&slot->lock);

        Sleep((DWORD)(rand() % 2));
    }

    return 0;
}

/* looks at the mutexes from outside of the module: a unit is held if another thread owns its mutex */
static uint32_t get_held_unit_count(const char* name, uint32_t max_count)
{
    uint32_t held_count = 0;

    for (uint32_t i = 0; i < max_count; i++)
    {
        char mutex_name[128];
        ASSERT_IS_TRUE(snprintf(mutex_name, sizeof(mutex_name), "%s_%" PRIu32 "", name, i) > 0);

        HANDLE mutex = OpenMutexA(SYNCHRONIZE | MUTEX_MODIFY_STATE, FALSE, mutex_name);
        DWORD open_error = GetLastError();
        if (mutex == NULL)
        {
            /* nobody has the mutex open, so nobody holds it */
            ASSERT_ARE_EQUAL(uint32_t, ERROR_FILE_NOT_FOUND, (uint32_t)open_error, "OpenMutexA(%s) failed", mutex_name);
        }
        else
        {
            DWORD wait_result = WaitForSingleObject(mutex, 0);
            if (
                (wait_result == WAIT_OBJECT_0) ||
                (wait_result == WAIT_ABANDONED)
                )
            {
                ASSERT_IS_TRUE(ReleaseMutex(mutex));
            }
            else
            {
                ASSERT_ARE_EQUAL(uint32_t, WAIT_TIMEOUT, (uint32_t)wait_result, "WaitForSingleObject(%s) failed", mutex_name);
                held_count++;
            }
            (void)CloseHandle(mutex);
        }
    }

    return held_count;
}

static void run_chaos_test(bool use_max_hold)
{
    static volatile_atomic int32_t test_index;
    CHAOS_TEST* test = malloc(sizeof(CHAOS_TEST));
    ASSERT_IS_NOT_NULL(test);
    (void)memset(test, 0, sizeof(CHAOS_TEST));
    ASSERT_IS_TRUE(snprintf(test->name, sizeof(test->name), "Local\\robust_semaphore_win32_chaos_int_%lu_%" PRId32 "", GetCurrentProcessId(), interlocked_increment(&test_index)) > 0);
    test->max_count = CHAOS_MAX_COUNT;
    test->use_max_hold = use_max_hold;
    (void)interlocked_exchange(&test->first_violation, (int32_t)CHAOS_VIOLATION_NONE);

    for (uint32_t i = 0; i < CHAOS_OWNER_COUNT; i++)
    {
        CHAOS_SLOT* slot = &test->slots[i];
        slot->test = test;
        slot->index = i;
        ASSERT_ARE_EQUAL(int, 0, srw_lock_ll_init(&slot->lock));
        slot->robust_semaphore = robust_semaphore_create(test->name, test->max_count, NULL);
        ASSERT_IS_NOT_NULL(slot->robust_semaphore);
        (void)interlocked_exchange(&slot->last_result, -1);
    }

    /* the robust semaphore logs every timeout and every early release, which would flood the output */
    LOGGER_CONFIG old_config = logger_get_config();
    logger_set_config((LOGGER_CONFIG) { .log_sinks = NULL, .log_sink_count = 0 });

    THREAD_HANDLE owner_threads[CHAOS_OWNER_COUNT];
    THREAD_HANDLE canceller_threads[CHAOS_CANCELLER_COUNT];
    for (uint32_t i = 0; i < CHAOS_OWNER_COUNT; i++)
    {
        ASSERT_ARE_EQUAL(int, THREADAPI_OK, ThreadAPI_Create(&owner_threads[i], chaos_owner_thread, &test->slots[i]));
    }
    for (uint32_t i = 0; i < CHAOS_CANCELLER_COUNT; i++)
    {
        ASSERT_ARE_EQUAL(int, THREADAPI_OK, ThreadAPI_Create(&canceller_threads[i], chaos_canceller_thread, test));
    }

    ThreadAPI_Sleep(CHAOS_TEST_DURATION_MS);
    (void)interlocked_exchange(&test->stop, 1);

    for (uint32_t i = 0; i < CHAOS_CANCELLER_COUNT; i++)
    {
        int thread_result;
        ASSERT_ARE_EQUAL(int, THREADAPI_OK, ThreadAPI_Join(canceller_threads[i], &thread_result));
    }
    for (uint32_t i = 0; i < CHAOS_OWNER_COUNT; i++)
    {
        int thread_result;
        ASSERT_ARE_EQUAL(int, THREADAPI_OK, ThreadAPI_Join(owner_threads[i], &thread_result));
    }

    logger_set_config(old_config);

    LogInfo("chaos with%s max_hold_ms: ok=%" PRId32 ", timeout=%" PRId32 ", cancelled=%" PRId32 ", error=%" PRId32 ", refused=%" PRId32 ", release=%" PRId32 ", cancel=%" PRId32 ", destroy=%" PRId32 ", max_holders=%" PRId32 ", violations=%" PRId32 " (first %" PRI_MU_ENUM ")",
        use_max_hold ? "" : "out",
        interlocked_add(&test->ok_count, 0), interlocked_add(&test->timeout_count, 0), interlocked_add(&test->cancelled_count, 0), interlocked_add(&test->error_count, 0),
        interlocked_add(&test->refused_count, 0), interlocked_add(&test->release_count, 0), interlocked_add(&test->cancel_count, 0), interlocked_add(&test->destroy_count, 0),
        interlocked_add(&test->max_holders, 0), interlocked_add(&test->violation_count, 0), MU_ENUM_VALUE(CHAOS_VIOLATION, (CHAOS_VIOLATION)interlocked_add(&test->first_violation, 0)));

    ASSERT_ARE_EQUAL(CHAOS_VIOLATION, CHAOS_VIOLATION_NONE, (CHAOS_VIOLATION)interlocked_add(&test->first_violation, 0));
    ASSERT_ARE_EQUAL(int32_t, 0, interlocked_add(&test->violation_count, 0));
    ASSERT_ARE_EQUAL(int32_t, 0, interlocked_add(&test->error_count, 0));
    ASSERT_IS_TRUE(interlocked_add(&test->ok_count, 0) > 0, "no unit was ever acquired");
    ASSERT_IS_TRUE(interlocked_add(&test->max_holders, 0) <= (int32_t)test->max_count);
    ASSERT_ARE_EQUAL(int32_t, 0, interlocked_add(&test->holders, 0));
    for (uint32_t i = 0; i < CHAOS_OWNER_COUNT; i++)
    {
        ASSERT_ARE_EQUAL(int32_t, interlocked_add(&test->slots[i].started, 0), interlocked_add(&test->slots[i].completed, 0), "slot %" PRIu32 "", i);
    }

    /* every robust semaphore object was destroyed, so every unit is free */
    double start = timer_global_get_elapsed_ms();
    uint32_t held_count;
    while ((held_count = get_held_unit_count(test->name, test->max_count)) != 0)
    {
        ASSERT_IS_TRUE(timer_global_get_elapsed_ms() - start < CHAOS_WAIT_TIMEOUT_MS, "%" PRIu32 " units of %s are still held", held_count, test->name);
        Sleep(CHAOS_UNIT_PROBE_INTERVAL_MS);
    }

    for (uint32_t i = 0; i < CHAOS_OWNER_COUNT; i++)
    {
        srw_lock_ll_deinit(&test->slots[i].lock);
    }
    free(test);
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    gballoc_hl_deinit();
}

TEST_FUNCTION(robust_semaphore_chaos_never_lets_more_than_max_count_holders_hold_a_unit)
{
    // arrange
    bool use_max_hold = false;

    // act
    run_chaos_test(use_max_hold);

    // assert
    /* run_chaos_test checks the invariants after the chaos */
}

TEST_FUNCTION(robust_semaphore_chaos_with_max_hold_ms_and_slow_callbacks_does_not_crash_or_leak_units)
{
    // arrange
    bool use_max_hold = true;

    // act
    run_chaos_test(use_max_hold);

    // assert
    /* run_chaos_test checks the invariants after the chaos */
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
