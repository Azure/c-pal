// Copyright (C) Microsoft Corporation. All rights reserved.
#include <stddef.h>
#include <stdbool.h>                      // for true
#include <stdlib.h>                       // for free, malloc
#include <inttypes.h>
#include <unistd.h>
#include <errno.h>

#include <sys/epoll.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/log_context.h"
#include "c_logging/log_context_property_type_ascii_char_ptr.h"
#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"        // IWYU pragma: keep
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/refcount.h"
#include "c_pal/socket_handle.h"
#include "c_pal/sync.h"
#include "c_pal/threadapi.h"

#include "c_pal/completion_port_linux.h"

#define MAX_EVENTS_NUM              64
#define EVENTS_TIMEOUT_MS           2*1000
#define SOCKET_ENTRY_BUCKET_COUNT   256

/*
epoll keeps one registration (one event mask and one data pointer) per file descriptor, so all the operations of a socket
share one SOCKET_ENTRY. The socket is registered once, with data.ptr pointing to its SOCKET_ENTRY and with EPOLLONESHOT,
for the union of the directions that have pending operations. Each event completes at most one operation per direction,
then the socket is re-armed for the operations that are still pending.
*/

typedef struct COMPLETION_PORT_OPERATION_TAG
{
    struct COMPLETION_PORT_OPERATION_TAG* next;
    int epoll_op;
    ON_COMPLETION_PORT_EVENT_COMPLETE event_callback;
    void* event_callback_ctx;
} COMPLETION_PORT_OPERATION;

typedef struct COMPLETION_PORT_OPERATION_QUEUE_TAG
{
    COMPLETION_PORT_OPERATION* head;
    COMPLETION_PORT_OPERATION* tail;
} COMPLETION_PORT_OPERATION_QUEUE;

typedef struct SOCKET_ENTRY_TAG
{
    struct SOCKET_ENTRY_TAG* next;
    SOCKET_HANDLE socket;
    bool is_registered;
    bool is_removed;
    COMPLETION_PORT_OPERATION_QUEUE inbound_operations;
    COMPLETION_PORT_OPERATION_QUEUE outbound_operations;
    volatile_atomic int32_t executing_callback_count;
} SOCKET_ENTRY;

typedef struct COMPLETION_PORT_TAG
{
    int epoll;
    THREAD_HANDLE thread_handle;
    volatile_atomic int32_t worker_thread_continue;
    volatile_atomic int32_t lock;
    volatile_atomic int32_t pending_calls;
    SOCKET_ENTRY* socket_entry_buckets[SOCKET_ENTRY_BUCKET_COUNT];
    SOCKET_ENTRY* retired_socket_entries;
} COMPLETION_PORT;

MU_DEFINE_ENUM_STRINGS(COMPLETION_PORT_EPOLL_ACTION, COMPLETION_PORT_EPOLL_ACTION_VALUES)

DEFINE_REFCOUNT_TYPE(COMPLETION_PORT);

static void internal_completion_port_lock(COMPLETION_PORT* completion_port)
{
    do
    {
        int32_t current_value = interlocked_compare_exchange(&completion_port->lock, 1, 0);
        if (current_value == 0)
        {
            break;
        }
        else
        {
            (void)wait_on_address(&completion_port->lock, current_value, UINT32_MAX);
        }
    } while (true);
}

static void internal_completion_port_unlock(COMPLETION_PORT* completion_port)
{
    (void)interlocked_exchange(&completion_port->lock, 0);
    wake_by_address_single(&completion_port->lock);
}

static void internal_completion_port_append_operation(COMPLETION_PORT_OPERATION_QUEUE* queue, COMPLETION_PORT_OPERATION* operation)
{
    operation->next = NULL;
    if (queue->tail == NULL)
    {
        queue->head = operation;
    }
    else
    {
        queue->tail->next = operation;
    }
    queue->tail = operation;
}

static COMPLETION_PORT_OPERATION* internal_completion_port_pop_operation(COMPLETION_PORT_OPERATION_QUEUE* queue)
{
    COMPLETION_PORT_OPERATION* result = queue->head;
    if (result == NULL)
    {
        /* do nothing */
    }
    else
    {
        queue->head = result->next;
        if (queue->head == NULL)
        {
            queue->tail = NULL;
        }
        else
        {
            /* do nothing */
        }
    }
    return result;
}

static void internal_completion_port_free_operations(COMPLETION_PORT_OPERATION* operation)
{
    while (operation != NULL)
    {
        COMPLETION_PORT_OPERATION* next_operation = operation->next;
        free(operation);
        operation = next_operation;
    }
}

static void internal_completion_port_free_socket_entries(SOCKET_ENTRY* socket_entry)
{
    while (socket_entry != NULL)
    {
        SOCKET_ENTRY* next_socket_entry = socket_entry->next;
        internal_completion_port_free_operations(socket_entry->inbound_operations.head);
        internal_completion_port_free_operations(socket_entry->outbound_operations.head);
        free(socket_entry);
        socket_entry = next_socket_entry;
    }
}

// Returns the link that points to the socket entry of socket, or the NULL link that ends its bucket if socket has no socket entry
static SOCKET_ENTRY** internal_completion_port_find_socket_entry(COMPLETION_PORT* completion_port, SOCKET_HANDLE socket)
{
    SOCKET_ENTRY** result = &completion_port->socket_entry_buckets[(uint32_t)socket % SOCKET_ENTRY_BUCKET_COUNT];
    while ((*result != NULL) && ((*result)->socket != socket))
    {
        result = &(*result)->next;
    }
    return result;
}

static uint32_t internal_completion_port_get_epoll_interest(bool has_inbound_operations, bool has_outbound_operations)
{
    uint32_t result = EPOLLONESHOT;
    if (has_inbound_operations)
    {
        result |= (EPOLLIN | EPOLLRDHUP);
    }
    else
    {
        /* do nothing */
    }

    if (has_outbound_operations)
    {
        result |= EPOLLOUT;
    }
    else
    {
        /* do nothing */
    }
    return result;
}

// Must be called while holding the completion port lock
static int internal_completion_port_queue_operation(COMPLETION_PORT* completion_port, SOCKET_HANDLE socket, COMPLETION_PORT_OPERATION* operation)
{
    int result;
    SOCKET_ENTRY** socket_entry_link = internal_completion_port_find_socket_entry(completion_port, socket);
    SOCKET_ENTRY* socket_entry = *socket_entry_link;

    if (socket_entry == NULL)
    {
        // Codes_SRS_COMPLETION_PORT_LINUX_01_006: [ If no socket entry exists for socket, completion_port_add shall allocate a new socket entry and add it to the socket entry lookup. ]
        socket_entry = malloc(sizeof(SOCKET_ENTRY));
        if (socket_entry == NULL)
        {
            LogError("failure allocating socket entry size: %zu", sizeof(SOCKET_ENTRY));
        }
        else
        {
            socket_entry->next = NULL;
            socket_entry->socket = socket;
            socket_entry->is_registered = false;
            socket_entry->is_removed = false;
            socket_entry->inbound_operations.head = NULL;
            socket_entry->inbound_operations.tail = NULL;
            socket_entry->outbound_operations.head = NULL;
            socket_entry->outbound_operations.tail = NULL;
            (void)interlocked_exchange(&socket_entry->executing_callback_count, 0);
            *socket_entry_link = socket_entry;
        }
    }
    else
    {
        /* do nothing */
    }

    if (socket_entry == NULL)
    {
        result = MU_FAILURE;
    }
    else
    {
        bool is_outbound = ((operation->epoll_op & EPOLLOUT) != 0);
        struct epoll_event epoll_registration = { 0 };
        epoll_registration.events = internal_completion_port_get_epoll_interest(
            (socket_entry->inbound_operations.head != NULL) || !is_outbound,
            (socket_entry->outbound_operations.head != NULL) || is_outbound);
        epoll_registration.data.ptr = socket_entry;

        int epoll_ctl_result;
        if (socket_entry->is_registered)
        {
            // Codes_SRS_COMPLETION_PORT_LINUX_01_008: [ If the socket entry is registered in epoll, completion_port_add shall call epoll_ctl with EPOLL_CTL_MOD, the epoll interest of the socket entry including the new operation and the socket entry as data.ptr. ]
            epoll_ctl_result = epoll_ctl(completion_port->epoll, EPOLL_CTL_MOD, socket, &epoll_registration);
        }
        else
        {
            // Codes_SRS_COMPLETION_PORT_LINUX_01_007: [ If the socket entry is not registered in epoll, completion_port_add shall call epoll_ctl with EPOLL_CTL_ADD, the epoll interest of the socket entry including the new operation and the socket entry as data.ptr. ]
            epoll_ctl_result = epoll_ctl(completion_port->epoll, EPOLL_CTL_ADD, socket, &epoll_registration);
        }

        if (epoll_ctl_result != 0)
        {
            LogErrorNo("failure epoll_ctl socket=%" PRI_SOCKET ", is_registered=%" PRI_BOOL ", events=0x%" PRIx32 "", socket, MU_BOOL_VALUE(socket_entry->is_registered), epoll_registration.events);
            if (!socket_entry->is_registered)
            {
                // Codes_SRS_COMPLETION_PORT_LINUX_01_011: [ If epoll_ctl with EPOLL_CTL_ADD fails, completion_port_add shall remove the new socket entry from the socket entry lookup and free it. ]
                // The socket entry was never registered in epoll, so no event can reference it
                *socket_entry_link = socket_entry->next;
                free(socket_entry);
            }
            else
            {
                // A failed EPOLL_CTL_MOD leaves the existing registration of the socket unchanged
            }
            result = MU_FAILURE;
        }
        else
        {
            socket_entry->is_registered = true;
            if (is_outbound)
            {
                // Codes_SRS_COMPLETION_PORT_LINUX_01_009: [ If epoll_ctl succeeds and epoll_op contains EPOLLOUT, completion_port_add shall append the operation to the outbound operation queue of the socket entry. ]
                internal_completion_port_append_operation(&socket_entry->outbound_operations, operation);
            }
            else
            {
                // Codes_SRS_COMPLETION_PORT_LINUX_01_010: [ If epoll_ctl succeeds and epoll_op does not contain EPOLLOUT, completion_port_add shall append the operation to the inbound operation queue of the socket entry. ]
                internal_completion_port_append_operation(&socket_entry->inbound_operations, operation);
            }
            result = 0;
        }
    }
    return result;
}

static void internal_completion_port_abandon_operations(COMPLETION_PORT_OPERATION* operation)
{
    while (operation != NULL)
    {
        COMPLETION_PORT_OPERATION* next_operation = operation->next;
        // Codes_SRS_COMPLETION_PORT_LINUX_01_019: [ completion_port_remove shall call the event_callback of every pending operation of the socket entry with COMPLETION_PORT_EPOLL_ABANDONED. ]
        operation->event_callback(operation->event_callback_ctx, COMPLETION_PORT_EPOLL_ABANDONED);
        // Codes_SRS_COMPLETION_PORT_LINUX_01_020: [ completion_port_remove shall free every abandoned operation. ]
        free(operation);
        operation = next_operation;
    }
}

static void internal_completion_port_free_retired_socket_entries(COMPLETION_PORT* completion_port)
{
    internal_completion_port_lock(completion_port);
    SOCKET_ENTRY* retired_socket_entries = completion_port->retired_socket_entries;
    completion_port->retired_socket_entries = NULL;
    internal_completion_port_unlock(completion_port);

    internal_completion_port_free_socket_entries(retired_socket_entries);
}

static void internal_completion_port_process_event(COMPLETION_PORT* completion_port, uint32_t events, SOCKET_ENTRY* socket_entry)
{
    COMPLETION_PORT_OPERATION* inbound_operation = NULL;
    COMPLETION_PORT_OPERATION* outbound_operation = NULL;

    // Codes_SRS_COMPLETION_PORT_LINUX_01_023: [ For each event, epoll_worker_func shall acquire the completion port lock. ]
    internal_completion_port_lock(completion_port);

    // Codes_SRS_COMPLETION_PORT_LINUX_01_024: [ If the socket entry of the event is marked as removed, epoll_worker_func shall skip the event. ]
    if (socket_entry->is_removed)
    {
        // completion_port_remove already abandoned the pending operations of this socket entry
    }
    else
    {
        if ((events & (EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLHUP)) != 0)
        {
            // Codes_SRS_COMPLETION_PORT_LINUX_01_025: [ If the event contains EPOLLIN, EPOLLRDHUP, EPOLLERR or EPOLLHUP, epoll_worker_func shall remove the first operation, if any, from the inbound operation queue of the socket entry. ]
            inbound_operation = internal_completion_port_pop_operation(&socket_entry->inbound_operations);
        }
        else
        {
            /* do nothing */
        }

        if ((events & (EPOLLOUT | EPOLLERR | EPOLLHUP)) != 0)
        {
            // Codes_SRS_COMPLETION_PORT_LINUX_01_026: [ If the event contains EPOLLOUT, EPOLLERR or EPOLLHUP, epoll_worker_func shall remove the first operation, if any, from the outbound operation queue of the socket entry. ]
            outbound_operation = internal_completion_port_pop_operation(&socket_entry->outbound_operations);
        }
        else
        {
            /* do nothing */
        }

        if ((inbound_operation == NULL) && (outbound_operation == NULL))
        {
            /* do nothing */
        }
        else
        {
            // Codes_SRS_COMPLETION_PORT_LINUX_01_027: [ If an operation was removed from a queue, epoll_worker_func shall increment the executing callback count of the socket entry. ]
            (void)interlocked_increment(&socket_entry->executing_callback_count);
        }

        if ((socket_entry->inbound_operations.head == NULL) && (socket_entry->outbound_operations.head == NULL))
        {
            // Codes_SRS_COMPLETION_PORT_LINUX_01_029: [ If no operation is pending for the socket entry, epoll_worker_func shall not re-arm the socket. ]
            // EPOLLONESHOT disarmed the socket, completion_port_add re-arms it when an operation is queued
        }
        else
        {
            struct epoll_event epoll_registration = { 0 };
            epoll_registration.events = internal_completion_port_get_epoll_interest(socket_entry->inbound_operations.head != NULL, socket_entry->outbound_operations.head != NULL);
            epoll_registration.data.ptr = socket_entry;

            // Codes_SRS_COMPLETION_PORT_LINUX_01_028: [ If any operation is still pending for the socket entry, epoll_worker_func shall re-arm the socket by calling epoll_ctl with EPOLL_CTL_MOD, the epoll interest of the socket entry and the socket entry as data.ptr. ]
            if (epoll_ctl(completion_port->epoll, EPOLL_CTL_MOD, socket_entry->socket, &epoll_registration) != 0)
            {
                LogErrorNo("failure re-arming socket=%" PRI_SOCKET " with epoll_ctl EPOLL_CTL_MOD, events=0x%" PRIx32 "", socket_entry->socket, epoll_registration.events);
            }
            else
            {
                /* do nothing */
            }
        }
    }

    // Codes_SRS_COMPLETION_PORT_LINUX_01_030: [ epoll_worker_func shall release the completion port lock before calling any event_callback. ]
    internal_completion_port_unlock(completion_port);

    if (inbound_operation == NULL)
    {
        /* do nothing */
    }
    else
    {
        COMPLETION_PORT_EPOLL_ACTION inbound_action;
        if (((inbound_operation->epoll_op & EPOLLRDHUP) != 0) && ((events & (EPOLLRDHUP | EPOLLERR | EPOLLHUP)) != 0))
        {
            // Codes_SRS_COMPLETION_PORT_LINUX_01_031: [ If the removed inbound operation was added with EPOLLRDHUP in epoll_op and the event contains EPOLLRDHUP, EPOLLERR or EPOLLHUP, epoll_worker_func shall call its event_callback with COMPLETION_PORT_EPOLL_EPOLLRDHUP. ]
            inbound_action = COMPLETION_PORT_EPOLL_EPOLLRDHUP;
        }
        else
        {
            // Codes_SRS_COMPLETION_PORT_LINUX_01_032: [ Otherwise, epoll_worker_func shall call the event_callback of the removed inbound operation with COMPLETION_PORT_EPOLL_EPOLLIN. ]
            inbound_action = COMPLETION_PORT_EPOLL_EPOLLIN;
        }
        inbound_operation->event_callback(inbound_operation->event_callback_ctx, inbound_action);

        // Codes_SRS_COMPLETION_PORT_LINUX_01_034: [ epoll_worker_func shall free the removed operations after calling their event_callback. ]
        free(inbound_operation);
    }

    if (outbound_operation == NULL)
    {
        /* do nothing */
    }
    else
    {
        // Codes_SRS_COMPLETION_PORT_LINUX_01_033: [ epoll_worker_func shall call the event_callback of the removed outbound operation with COMPLETION_PORT_EPOLL_EPOLLOUT. ]
        outbound_operation->event_callback(outbound_operation->event_callback_ctx, COMPLETION_PORT_EPOLL_EPOLLOUT);

        // Codes_SRS_COMPLETION_PORT_LINUX_01_034: [ epoll_worker_func shall free the removed operations after calling their event_callback. ]
        free(outbound_operation);
    }

    if ((inbound_operation == NULL) && (outbound_operation == NULL))
    {
        /* do nothing */
    }
    else
    {
        // Codes_SRS_COMPLETION_PORT_LINUX_01_035: [ After calling the event callbacks, epoll_worker_func shall decrement the executing callback count of the socket entry by calling InterlockedHL_DecrementAndWake. ]
        (void)InterlockedHL_DecrementAndWake(&socket_entry->executing_callback_count);
    }
}

static int epoll_worker_func(void* parameter)
{
    COMPLETION_PORT_HANDLE completion_port = parameter;
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
            // Codes_SRS_COMPLETION_PORT_LINUX_01_022: [ Before each call to epoll_wait, epoll_worker_func shall free the socket entries in the retired list. ]
            // Only epoll_wait batches that returned before a socket was removed from epoll can reference its retired socket entry,
            // and all of them have been processed by now
            internal_completion_port_free_retired_socket_entries(completion_port);

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
                else
                {
                    /* do nothing */
                }
            }
            else
            {
                /* do nothing */
            }

            // Codes_SRS_COMPLETION_PORT_LINUX_11_034: [ epoll_worker_func shall loop through the num of descriptors that was returned. ]
            for (int index = 0; index < num_ready; index++)
            {
                internal_completion_port_process_event(completion_port, events[index].events, events[index].data.ptr);
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
        (void)interlocked_exchange(&result->lock, 0);
        (void)interlocked_exchange(&result->pending_calls, 0);

        for (uint32_t bucket_index = 0; bucket_index < SOCKET_ENTRY_BUCKET_COUNT; bucket_index++)
        {
            result->socket_entry_buckets[bucket_index] = NULL;
        }
        result->retired_socket_entries = NULL;

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
            // Codes_SRS_COMPLETION_PORT_LINUX_11_012: [ increment the flag signaling that the threads can complete. ]
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

            for (uint32_t bucket_index = 0; bucket_index < SOCKET_ENTRY_BUCKET_COUNT; bucket_index++)
            {
                // Codes_SRS_COMPLETION_PORT_LINUX_01_001: [ free the pending operations of every socket entry without calling their event_callback. ]
                // Codes_SRS_COMPLETION_PORT_LINUX_01_002: [ free every socket entry. ]
                internal_completion_port_free_socket_entries(completion_port->socket_entry_buckets[bucket_index]);
            }

            // Codes_SRS_COMPLETION_PORT_LINUX_01_003: [ free every retired socket entry. ]
            internal_completion_port_free_socket_entries(completion_port->retired_socket_entries);

            // Codes_SRS_COMPLETION_PORT_LINUX_11_015: [ then the memory associated with completion_port shall be freed. ]
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
        // Codes_SRS_COMPLETION_PORT_LINUX_11_016: [ If completion_port is NULL, completion_port_add shall return a non-zero value. ]
        completion_port == NULL ||
        // Codes_SRS_COMPLETION_PORT_LINUX_11_017: [ If socket is INVALID_SOCKET, completion_port_add shall return a non-zero value. ]
        socket == INVALID_SOCKET ||
        // Codes_SRS_COMPLETION_PORT_LINUX_11_018: [ If event_callback is NULL, completion_port_add shall return a non-zero value. ]
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

            // Codes_SRS_COMPLETION_PORT_LINUX_01_004: [ completion_port_add shall allocate a pending operation that stores epoll_op, event_callback and event_callback_ctx. ]
            COMPLETION_PORT_OPERATION* operation = malloc(sizeof(COMPLETION_PORT_OPERATION));
            if (operation == NULL)
            {
                LogError("failure allocating completion port operation size: %zu", sizeof(COMPLETION_PORT_OPERATION));
                // Codes_SRS_COMPLETION_PORT_LINUX_11_027: [ If any error occurs, completion_port_add shall fail and return a non-zero value. ]
                result = MU_FAILURE;
            }
            else
            {
                operation->next = NULL;
                operation->epoll_op = epoll_op;
                operation->event_callback = event_callback;
                operation->event_callback_ctx = event_callback_ctx;

                // Codes_SRS_COMPLETION_PORT_LINUX_01_005: [ completion_port_add shall acquire the completion port lock. ]
                internal_completion_port_lock(completion_port);

                int queue_result = internal_completion_port_queue_operation(completion_port, socket, operation);

                // Codes_SRS_COMPLETION_PORT_LINUX_01_012: [ completion_port_add shall release the completion port lock. ]
                internal_completion_port_unlock(completion_port);

                if (queue_result != 0)
                {
                    LogError("failure queueing operation for socket=%" PRI_SOCKET ", epoll_op=%d", socket, epoll_op);
                    // Codes_SRS_COMPLETION_PORT_LINUX_01_013: [ If queueing the operation fails, completion_port_add shall free the pending operation. ]
                    free(operation);
                    // Codes_SRS_COMPLETION_PORT_LINUX_11_027: [ If any error occurs, completion_port_add shall fail and return a non-zero value. ]
                    result = MU_FAILURE;
                }
                else
                {
                    // Codes_SRS_COMPLETION_PORT_LINUX_11_026: [ On success, completion_port_add shall return 0. ]
                    result = 0;
                }
            }
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

            COMPLETION_PORT_OPERATION* abandoned_inbound_operations;
            COMPLETION_PORT_OPERATION* abandoned_outbound_operations;

            // Codes_SRS_COMPLETION_PORT_LINUX_01_014: [ completion_port_remove shall acquire the completion port lock. ]
            internal_completion_port_lock(completion_port);

            // Codes_SRS_COMPLETION_PORT_LINUX_11_030: [ completion_port_remove shall remove the underlying socket from the epoll by calling epoll_ctl with EPOLL_CTL_DEL. ]
            if (epoll_ctl(completion_port->epoll, EPOLL_CTL_DEL, socket, NULL) == -1)
            {
                LogErrorNo("Failure epoll_ctl with EPOLL_CTL_DEL %" PRI_SOCKET "", socket);
            }
            else
            {
                /* do nothing */
            }

            SOCKET_ENTRY** socket_entry_link = internal_completion_port_find_socket_entry(completion_port, socket);
            SOCKET_ENTRY* socket_entry = *socket_entry_link;
            if (socket_entry == NULL)
            {
                abandoned_inbound_operations = NULL;
                abandoned_outbound_operations = NULL;
            }
            else
            {
                // Codes_SRS_COMPLETION_PORT_LINUX_01_016: [ completion_port_remove shall remove the socket entry from the socket entry lookup. ]
                *socket_entry_link = socket_entry->next;
                socket_entry->next = NULL;

                // Codes_SRS_COMPLETION_PORT_LINUX_01_017: [ completion_port_remove shall mark the socket entry as removed. ]
                socket_entry->is_removed = true;

                abandoned_inbound_operations = socket_entry->inbound_operations.head;
                abandoned_outbound_operations = socket_entry->outbound_operations.head;
                socket_entry->inbound_operations.head = NULL;
                socket_entry->inbound_operations.tail = NULL;
                socket_entry->outbound_operations.head = NULL;
                socket_entry->outbound_operations.tail = NULL;
            }

            // Codes_SRS_COMPLETION_PORT_LINUX_01_018: [ completion_port_remove shall release the completion port lock before calling any event_callback. ]
            internal_completion_port_unlock(completion_port);

            if (socket_entry == NULL)
            {
                // Codes_SRS_COMPLETION_PORT_LINUX_01_015: [ If no socket entry exists for socket, completion_port_remove shall not call any event_callback. ]
            }
            else
            {
                internal_completion_port_abandon_operations(abandoned_inbound_operations);
                internal_completion_port_abandon_operations(abandoned_outbound_operations);

                // Codes_SRS_COMPLETION_PORT_LINUX_11_039: [ If the event_callback is currently executing, completion_port_remove shall wait for the event_callback function to finish before completing. ]
                if (InterlockedHL_WaitForValue(&socket_entry->executing_callback_count, 0, UINT32_MAX) != INTERLOCKED_HL_OK)
                {
                    LogError("failure waiting for the executing callbacks of socket=%" PRI_SOCKET "", socket);
                }
                else
                {
                    /* do nothing */
                }

                // The socket entry is not freed here: epoll_worker_func may still hold a pointer to it from an epoll_wait batch
                // that returned before EPOLL_CTL_DEL. epoll_worker_func frees retired socket entries before its next epoll_wait.
                // Codes_SRS_COMPLETION_PORT_LINUX_01_021: [ completion_port_remove shall add the socket entry to the retired list of the completion port. ]
                internal_completion_port_lock(completion_port);
                socket_entry->next = completion_port->retired_socket_entries;
                completion_port->retired_socket_entries = socket_entry;
                internal_completion_port_unlock(completion_port);
            }

            (void)interlocked_decrement(&completion_port->pending_calls);
            wake_by_address_single(&completion_port->pending_calls);
        }
    }
}
