// Copyright (C) Microsoft Corporation. All rights reserved.
#include <stddef.h>
#include <stdbool.h>                      // for true
#include <stdlib.h>                       // for free, malloc
#include <inttypes.h>
#include <unistd.h>
#include <errno.h>

#include <sys/epoll.h>

#ifdef USE_VALGRIND
#include "valgrind/helgrind.h"
#endif

#include "macro_utils/macro_utils.h"

#include "c_logging/xlogging.h"

#include "c_pal/containing_record.h"
#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/refcount.h"
#include "c_pal/socket_handle.h"
#include "c_pal/s_list.h"
#include "c_pal/sync.h"
#include "c_pal/threadapi.h"

#include "c_pal/completion_port_linux.h"

#define MAX_EVENTS_NUM      64
#define EVENTS_TIMEOUT_MS   2*1000

typedef struct EPOLL_THREAD_DATA_TAG
{
    S_LIST_ENTRY link;
    ON_COMPLETION_PORT_EVENT_COMPLETE event_callback;
    void* event_callback_ctx;
    volatile_atomic int32_t event_callback_called;
    SOCKET_HANDLE socket;
} EPOLL_THREAD_DATA;

typedef struct COMPLETION_PORT_TAG
{
    int epoll;
    THREAD_HANDLE thread_handle;
    volatile_atomic int32_t worker_thread_continue;
    S_LIST_ENTRY alloc_data_list;
    volatile_atomic int32_t recv_data_access;
    volatile_atomic int32_t pending_calls;
} COMPLETION_PORT;

#define COMPLETION_PORT_CALLBACK_STATE_VALUES \
    COMPLETION_PORT_CALLBACK_INIT, \
    COMPLETION_PORT_CALLBACK_EXECUTING, \
    COMPLETION_PORT_CALLBACK_EXECUTED

MU_DEFINE_ENUM(COMPLETION_PORT_CALLBACK_STATE, COMPLETION_PORT_CALLBACK_STATE_VALUES)
MU_DEFINE_ENUM_STRINGS(COMPLETION_PORT_CALLBACK_STATE, COMPLETION_PORT_CALLBACK_STATE_VALUES)

MU_DEFINE_ENUM_STRINGS(COMPLETION_PORT_EPOLL_ACTION, COMPLETION_PORT_EPOLL_ACTION_VALUES)

DEFINE_REFCOUNT_TYPE(COMPLETION_PORT);

static void enter_crit_section(COMPLETION_PORT* completion_port)
{
    do
    {
        int32_t current_val = interlocked_compare_exchange(&completion_port->recv_data_access, 1, 0);
        if (current_val == 0)
        {
            break;
        }
        else
        {
            // Do Nothing wait for address
        }
        (void)wait_on_address(&completion_port->recv_data_access, current_val, UINT32_MAX);
    } while (true);
}

static void leave_crit_section(COMPLETION_PORT* completion_port)
{
    (void)interlocked_exchange(&completion_port->recv_data_access, 0);
    wake_by_address_single(&completion_port->recv_data_access);
}

static int add_thread_data_to_list(COMPLETION_PORT* completion_port, EPOLL_THREAD_DATA* epoll_data)
{
    int result;
    enter_crit_section(completion_port);
    {
        if (s_list_add(&completion_port->alloc_data_list, &epoll_data->link) != 0)
        {
            LogError("failure adding receive data to list");
            result = MU_FAILURE;
        }
        else
        {
            result = 0;
        }
    }
    leave_crit_section(completion_port);
    return result;
}

static int remove_thread_data_from_list(COMPLETION_PORT* completion_port, EPOLL_THREAD_DATA* epoll_data)
{
    int result;
    enter_crit_section(completion_port);
    {
        if (s_list_remove(&completion_port->alloc_data_list, &epoll_data->link) != 0)
        {
            LogError("failure removing receive data");
            result = MU_FAILURE;
        }
        else
        {
            result = 0;
        }
    }
    leave_crit_section(completion_port);

    return result;
}

static int epoll_worker_func(void* parameter)
{
    COMPLETION_PORT_HANDLE completion_port = (COMPLETION_PORT_HANDLE)parameter;
    // Codes_SRS_COMPLETION_PORT_LINUX_11_031: [ If parameter is NULL, epoll_worker_func shall do nothing. ]
    if (completion_port == NULL)
    {
        LogCritical("Invalid arguement epoll_worker_function COMPLETION_PORT_HANDLE completion_port=%p", completion_port);
    }
    else
    {
        int log_only_once = 0;
        // Loop while true
        do
        {
            struct epoll_event events[MAX_EVENTS_NUM];
            // Codes_SRS_COMPLETION_PORT_LINUX_11_032: [ epoll_worker_func shall call epoll_wait to wait for an epoll event to become signaled with a timeout of 2 Seconds. ]
            int num_ready = epoll_wait(completion_port->epoll, events, MAX_EVENTS_NUM, EVENTS_TIMEOUT_MS);
            if (num_ready == -1)
            {
                if (log_only_once < 1)
                {
                    LogErrorNo("Failure epoll_wait, MAX_EVENTS_NUM: %d, EVENTS_TIMEOUT_MS: %d event: %p", MAX_EVENTS_NUM, EVENTS_TIMEOUT_MS, events);
                    log_only_once++;
                }
            }

            // Codes_SRS_COMPLETION_PORT_LINUX_11_034: [ epoll_worker_func shall loop through the num of descriptors that was returned. ]
            for (int index = 0; index < num_ready; index++)
            {
                COMPLETION_PORT_EPOLL_ACTION epoll_action;
                EPOLL_THREAD_DATA* epoll_data = events[index].data.ptr;
                if (events[index].events & EPOLLRDHUP)
                {
                    epoll_action = COMPLETION_PORT_EPOLL_EPOLLRDHUP;
                }
                else if (events[index].events & EPOLLIN)
                {
                    epoll_action = COMPLETION_PORT_EPOLL_EPOLLIN;
                }
                else if (events[index].events & EPOLLOUT)
                {
                    epoll_action = COMPLETION_PORT_EPOLL_EPOLLOUT;
                }
                else
                {
                    LogWarning("Unexpected epoll event %d", events[index].events);
                    continue;
                }
                #ifdef USE_VALGRIND
                    ANNOTATE_HAPPENS_AFTER(epoll_data);
                #endif
                // If we haven't called into event_callback yet
                if (interlocked_compare_exchange(&epoll_data->event_callback_called, COMPLETION_PORT_CALLBACK_EXECUTING, COMPLETION_PORT_CALLBACK_INIT) == COMPLETION_PORT_CALLBACK_INIT)
                {
                    // Codes_SRS_COMPLETION_PORT_LINUX_11_035: [ epoll_worker_func shall call the event_callback with the specified COMPLETION_PORT_EPOLL_ACTION that was returned. ]
                    epoll_data->event_callback(epoll_data->event_callback_ctx, epoll_action);

                    (void)interlocked_exchange(&epoll_data->event_callback_called, COMPLETION_PORT_CALLBACK_EXECUTED);
                    wake_by_address_single(&epoll_data->event_callback_called);
                }
                // Codes_SRS_COMPLETION_PORT_LINUX_11_036: [ Then epoll_worker_func shall remove the EPOLL_THREAD_DATA from the list and free the object. ]
                if (remove_thread_data_from_list(completion_port, epoll_data) != 0)
                {
                    LogWarning("failure removing receive data from list %p, action: %" PRI_MU_ENUM "", epoll_data, MU_ENUM_VALUE(COMPLETION_PORT_EPOLL_ACTION, epoll_action));
                }
                free(epoll_data);
            }
            // Codes_SRS_COMPLETION_PORT_LINUX_11_033: [ On a epoll_wait timeout epoll_worker_func shall ensure it should not exit and issue another epoll_wait. ]
        } while (interlocked_add(&completion_port->worker_thread_continue, 0) == 0);
    }
    return 0;
}

COMPLETION_PORT_HANDLE completion_port_create(void)
{
    COMPLETION_PORT_HANDLE result;

    // Codes_SRS_COMPLETION_PORT_LINUX_11_001: [ completion_port_create shall allocate memory for a completion port object. ]
    result = REFCOUNT_TYPE_CREATE(COMPLETION_PORT);
    if (result == NULL)
    {
        LogError("failure REFCOUNT_TYPE_CREATE completion port");
    }
    else
    {
        (void)interlocked_exchange(&result->worker_thread_continue, 0);
        (void)interlocked_exchange(&result->recv_data_access, 0);
        (void)interlocked_exchange(&result->pending_calls, 0);

        if (s_list_initialize(&result->alloc_data_list) != 0)
        {
            LogError("failure initializing recv list");
        }
        else
        {
            // Codes_SRS_COMPLETION_PORT_LINUX_11_002: [ completion_port_create shall create the epoll instance by calling epoll_create. ]
            result->epoll = epoll_create(MAX_EVENTS_NUM);
            if (result->epoll == -1)
            {
                LogErrorNo("failure epoll_create MAX_EVENTS_NUM: %d", MAX_EVENTS_NUM);
            }
            else
            {
                // Codes_SRS_COMPLETION_PORT_LINUX_11_003: [ completion_port_create shall create a thread that runs epoll_worker_func to handle the epoll events. ]
                if (ThreadAPI_Create(&result->thread_handle, epoll_worker_func, result) != THREADAPI_OK)
                {
                    LogCritical("Failure creating thread");
                }
                else
                {
                    // Codes_SRS_COMPLETION_PORT_LINUX_11_004: [ On success completion_port_create shall return the allocated COMPLETION_PORT_HANDLE. ]
                    goto all_ok;
                }
            }
            (void)close(result->epoll);
        }
        // Codes_SRS_COMPLETION_PORT_LINUX_11_005: [ If there are any errors then completion_port_create shall fail and return NULL. ]
        REFCOUNT_TYPE_DESTROY(COMPLETION_PORT, result);
        result = NULL;
    }
all_ok:
    return result;
}

void completion_port_inc_ref(COMPLETION_PORT_HANDLE completion_port)
{
    // Codes_SRS_COMPLETION_PORT_LINUX_11_006: [ If completion_port is NULL, completion_port_inc_ref shall return. ]
    if (completion_port == NULL)
    {
        LogError("Invalid arguments: COMPLETION_PORT_HANDLE completion_port=%p", completion_port);
    }
    else
    {
        // Codes_SRS_COMPLETION_PORT_LINUX_11_007: [ Otherwise completion_port_inc_ref shall increment the internally maintained reference count. ]
        INC_REF(COMPLETION_PORT, completion_port);
    }
}

void completion_port_dec_ref(COMPLETION_PORT_HANDLE completion_port)
{
    // Codes_SRS_COMPLETION_PORT_LINUX_11_008: [ If completion_port is NULL, completion_port_dec_ref shall return. ]
    if (completion_port == NULL)
    {
        LogError("Invalid arguments: COMPLETION_PORT_HANDLE completion_port=%p", completion_port);
    }
    else
    {
        // Codes_SRS_COMPLETION_PORT_LINUX_11_009: [ completion_port_dec_ref shall decrement the reference count for completion_port. ]
        if (DEC_REF(COMPLETION_PORT, completion_port) == 0)
        {
            // Codes_SRS_COMPLETION_PORT_LINUX_11_012: [ - increment the flag signaling that the threads can complete. ]
            (void)interlocked_increment(&completion_port->worker_thread_continue);

            // Codes_SRS_COMPLETION_PORT_LINUX_11_010: [ If the reference count reaches 0, completion_port_dec_ref shall do the following: ]
            int32_t value;
            // Codes_SRS_COMPLETION_PORT_LINUX_11_011: [ wait for the ongoing call count to reach zero. ]
            while ((value = interlocked_add(&completion_port->pending_calls, 0)) != 0)
            {
                (void)wait_on_address(&completion_port->pending_calls, value, UINT32_MAX);
            }

            // Codes_SRS_COMPLETION_PORT_LINUX_11_013: [ close the epoll object. ]
            if (close(completion_port->epoll) != 0)
            {
                LogErrorNo("failure closing epoll");
            }

            int dont_care;
            // Codes_SRS_COMPLETION_PORT_LINUX_11_014: [ close the thread by calling ThreadAPI_Join. ]
            if (ThreadAPI_Join(completion_port->thread_handle, &dont_care) != THREADAPI_OK)
            {
                LogError("Failure joining thread");
            }

            // Codes_SRS_COMPLETION_PORT_LINUX_11_015: [ then the memory associated with completion_port shall be freed. ]
            PS_LIST_ENTRY entry;
            while ((entry = s_list_remove_head(&completion_port->alloc_data_list)) != &completion_port->alloc_data_list)
            {
                EPOLL_THREAD_DATA* epoll_data = CONTAINING_RECORD(entry, EPOLL_THREAD_DATA, link);
                free(epoll_data);
            }

            REFCOUNT_TYPE_DESTROY(COMPLETION_PORT, completion_port);
        }
        else
        {
            // do nothing
        }
    }
}

int completion_port_add(COMPLETION_PORT_HANDLE completion_port, int epoll_op, SOCKET_HANDLE socket,
    ON_COMPLETION_PORT_EVENT_COMPLETE event_callback, void* event_callback_ctx)
{
    int result;
    if (
        // Codes_SRS_COMPLETION_PORT_LINUX_11_016: [ If completion_port is NULL, completion_port_add shall return a non-NULL value. ]
        completion_port == NULL ||
        // Codes_SRS_COMPLETION_PORT_LINUX_11_017: [ If socket is INVALID_SOCKET, completion_port_add shall return a non-NULL value. ]
        socket == INVALID_SOCKET ||
        // Codes_SRS_COMPLETION_PORT_LINUX_11_018: [ If event_callback is NULL, completion_port_add shall return a non-NULL value. ]
        event_callback == NULL)
    {
        LogError("Invalid arguments: COMPLETION_PORT_HANDLE completion_port=%p, epoll_op=%d, SOCKET_HANDLE socket=%" PRI_SOCKET ", ON_COMPLETION_PORT_EVENT_COMPLETE event_callback=%p, void* event_callback_ctx=%p",
            completion_port, epoll_op, socket, event_callback, event_callback_ctx);
        result = MU_FAILURE;
    }
    else
    {
        // Codes_SRS_COMPLETION_PORT_LINUX_11_019: [ completion_port_add shall ensure the thread completion flag is not set. ]
        if (interlocked_add(&completion_port->worker_thread_continue, 0) != 0)
        {
            LogError("Invalid call sequence completion port module is destroying");
            result = MU_FAILURE;
        }
        else
        {
            // Codes_SRS_COMPLETION_PORT_LINUX_11_020: [ completion_port_add shall increment the ongoing call count value to prevent close. ]
            (void)interlocked_increment(&completion_port->pending_calls);

            // Codes_SRS_COMPLETION_PORT_LINUX_11_021: [ completion_port_add shall allocate a EPOLL_THREAD_DATA object to store thread data. ]
            EPOLL_THREAD_DATA* epoll_thread_data = malloc(sizeof(EPOLL_THREAD_DATA));
            if (epoll_thread_data == NULL)
            {
                LogError("failure allocating epoll thread data size: %zu", sizeof(EPOLL_THREAD_DATA));
                result = MU_FAILURE;
            }
            else
            {
                epoll_thread_data->event_callback = event_callback;
                epoll_thread_data->event_callback_ctx = event_callback_ctx;
                (void)interlocked_exchange(&epoll_thread_data->event_callback_called, COMPLETION_PORT_CALLBACK_INIT);
                epoll_thread_data->socket = socket;
                epoll_thread_data->link.next = NULL;

                struct epoll_event ev = {0};
                ev.events = epoll_op;
                ev.data.ptr = (void*)epoll_thread_data;
                #ifdef USE_VALGRIND
                    ANNOTATE_HAPPENS_BEFORE(epoll_thread_data);
                #endif
                // Codes_SRS_COMPLETION_PORT_LINUX_11_022: [ completion_port_add shall add the EPOLL_THREAD_DATA object to a list for later removal. ]
                if (add_thread_data_to_list(completion_port, epoll_thread_data) != 0)
                {
                    LogError("failure adding data to list");
                }
                else
                {
                    // Codes_SRS_COMPLETION_PORT_LINUX_11_023: [ completion_port_add shall add the socket in the epoll system by calling epoll_ctl with EPOLL_CTL_MOD along with the epoll_op variable. ]
                    if (epoll_ctl(completion_port->epoll, EPOLL_CTL_MOD, epoll_thread_data->socket, &ev) < 0)
                    {
                        if (errno == ENOENT)
                        {
                            // Codes_SRS_COMPLETION_PORT_LINUX_11_024: [ If the epoll_ctl call fails with ENOENT, completion_port_add shall call epoll_ctl again with EPOLL_CTL_ADD. ]
                            if (epoll_ctl(completion_port->epoll, EPOLL_CTL_ADD, epoll_thread_data->socket, &ev) < 0)
                            {
                                LogErrorNo("failure with epoll_ctl EPOLL_CTL_ADD");
                            }
                            else
                            {
                                result = 0;
                                // (void)interlocked_decrement(&completion_port->pending_calls);
                                // wake_by_address_single(&completion_port->pending_calls);
                                goto all_ok;
                            }
                        }
                        else
                        {
                            LogErrorNo("failure with epoll_ctl EPOLL_CTL_MOD");
                        }
                    }
                    else
                    {
                        // Codes_SRS_COMPLETION_PORT_LINUX_11_026: [ On success, completion_port_add shall return 0. ]
                        result = 0;
                        goto all_ok;
                    }
                    // Remove
                    remove_thread_data_from_list(completion_port, epoll_thread_data);
                }
                free(epoll_thread_data);
            }
            // Codes_SRS_COMPLETION_PORT_LINUX_11_027: [ If any error occurs, completion_port_add shall fail and return a non-zero value. ]
            result = MU_FAILURE;
all_ok:
            // Codes_SRS_COMPLETION_PORT_LINUX_11_025: [ completion_port_add shall decrement the ongoing call count value to unblock close. ]
            (void)interlocked_decrement(&completion_port->pending_calls);
            wake_by_address_single(&completion_port->pending_calls);
        }
    }
    return result;
}

void completion_port_remove(COMPLETION_PORT_HANDLE completion_port, SOCKET_HANDLE socket)
{
    if (
        // Codes_SRS_COMPLETION_PORT_LINUX_11_028: [ If completion_port is NULL, completion_port_remove shall return. ]
        completion_port == NULL ||
        // Codes_SRS_COMPLETION_PORT_LINUX_11_029: [ If socket is INVALID_SOCKET, completion_port_remove shall return. ]
        socket == INVALID_SOCKET)
    {
        LogError("Invalid arguments: COMPLETION_PORT_HANDLE completion_port=%p, SOCKET_HANDLE socket=%" PRI_SOCKET "", completion_port, socket);
    }
    else
    {
        if (interlocked_add(&completion_port->worker_thread_continue, 0) != 0)
        {
            LogError("Invalid call sequence completion port module is destroying");
        }
        else
        {
            (void)interlocked_increment(&completion_port->pending_calls);

            // Codes_SRS_COMPLETION_PORT_LINUX_11_030: [ completion_port_remove shall remove the underlying socket from the epoll by calling epoll_ctl with EPOLL_CTL_DEL. ]
            if (epoll_ctl(completion_port->epoll, EPOLL_CTL_DEL, socket, NULL) == -1)
            {
                LogErrorNo("Failure epoll_ctl with EPOLL_CTL_DEL %" PRI_SOCKET "", socket);
            }

            // Codes_SRS_COMPLETION_PORT_LINUX_11_037: [ completion_port_remove shall loop through the list of EPOLL_THREAD_DATA object and call the event_callback with COMPLETION_PORT_EPOLL_ABANDONED ]
            enter_crit_section(completion_port);
            {
                PS_LIST_ENTRY current_item = completion_port->alloc_data_list.next;
                while (current_item != NULL)
                {
                    EPOLL_THREAD_DATA* epoll_data = CONTAINING_RECORD(current_item, EPOLL_THREAD_DATA, link);
                    if (epoll_data->socket == socket)
                    {
                        COMPLETION_PORT_CALLBACK_STATE callback_state = interlocked_compare_exchange(&epoll_data->event_callback_called, COMPLETION_PORT_CALLBACK_EXECUTING, COMPLETION_PORT_CALLBACK_INIT);
                        // Codes_SRS_COMPLETION_PORT_LINUX_11_038: [ If the event_callback has not been called, completion_port_remove shall call the event_callback. ]
                        if (callback_state == COMPLETION_PORT_CALLBACK_INIT)
                        {
                            epoll_data->event_callback(epoll_data->event_callback_ctx, COMPLETION_PORT_EPOLL_ABANDONED);
                            (void)interlocked_exchange(&epoll_data->event_callback_called, COMPLETION_PORT_CALLBACK_EXECUTED);
                        }
                        // Codes_SRS_COMPLETION_PORT_LINUX_11_039: [ If the event_callback is currently executing, completion_port_remove shall wait for the event_callback function to finish before completing. ]
                        else if (callback_state == COMPLETION_PORT_CALLBACK_EXECUTING)
                        {
                            // Callback is executing, so wait till the callback is done
                            (void)InterlockedHL_WaitForNotValue(&epoll_data->event_callback_called, COMPLETION_PORT_CALLBACK_EXECUTING, UINT32_MAX);
                        }
                        else
                        {
                            // callback state is Executed Do Nothing
                        }
                    }
                    current_item = current_item->next;
                }
            }
            leave_crit_section(completion_port);

            (void)interlocked_decrement(&completion_port->pending_calls);
            wake_by_address_single(&completion_port->pending_calls);
        }
    }
}
