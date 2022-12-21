// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>                     // for strerror_r

#include <sys/epoll.h>
#include <sys/socket.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/xlogging.h"

#include "c_pal/containing_record.h"
#include "c_pal/gballoc_hl.h"        // IWYU pragma: keep
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/execution_engine.h"
#include "c_pal/interlocked.h"
#include "c_pal/s_list.h"
#include "c_pal/sync.h"
#include "c_pal/socket_handle.h"
#include "c_pal/threadapi.h"

#include "c_pal/async_socket.h"

#define MAX_EVENTS_NUM      64
#define EVENTS_TIMEOUT_MS   2*1000
#define EPOLL_TIMEOUT_MS    10

#define ASYNC_SOCKET_LINUX_STATE_VALUES \
    ASYNC_SOCKET_LINUX_STATE_CLOSED, \
    ASYNC_SOCKET_LINUX_STATE_OPENING, \
    ASYNC_SOCKET_LINUX_STATE_OPEN, \
    ASYNC_SOCKET_LINUX_STATE_CLOSING

MU_DEFINE_ENUM(ASYNC_SOCKET_LINUX_STATE, ASYNC_SOCKET_LINUX_STATE_VALUES)
MU_DEFINE_ENUM_STRINGS(ASYNC_SOCKET_LINUX_STATE, ASYNC_SOCKET_LINUX_STATE_VALUES)

#define ASYNC_SOCKET_IO_TYPE_VALUES \
    ASYNC_SOCKET_IO_TYPE_SEND, \
    ASYNC_SOCKET_IO_TYPE_RECEIVE

MU_DEFINE_ENUM(ASYNC_SOCKET_IO_TYPE, ASYNC_SOCKET_IO_TYPE_VALUES)
MU_DEFINE_ENUM_STRINGS(ASYNC_SOCKET_IO_TYPE, ASYNC_SOCKET_IO_TYPE_VALUES)

MU_DEFINE_ENUM_STRINGS(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_RESULT_VALUES)

#define THREAD_COUNT    1

static volatile_atomic int32_t g_thread_access_cnt;
static int g_epoll = -1;
static THREAD_HANDLE g_thread_array[THREAD_COUNT];

typedef struct ASYNC_SOCKET_TAG
{
    int socket_handle;
    EXECUTION_ENGINE_HANDLE execution_engine;

    volatile_atomic int32_t state;
    volatile_atomic int32_t pending_api_calls;

    volatile_atomic int32_t epoll_cleanup;
    volatile_atomic int32_t recv_data_access;
    S_LIST_ENTRY recv_data_head;

    int epoll;
} ASYNC_SOCKET;

typedef struct ASYNC_SOCKET_OPEN_CONTEXT_TAG
{
    ON_ASYNC_SOCKET_OPEN_COMPLETE on_open_complete;
    void* on_open_complete_context;
    ASYNC_SOCKET* async_socket;
} ASYNC_SOCKET_OPEN_CONTEXT;

typedef struct ASYNC_SOCKET_IO_CONTEXT_TAG
{
    ASYNC_SOCKET* async_socket;
    uint32_t total_buffer_bytes;
    ON_ASYNC_SOCKET_SEND_COMPLETE on_send_complete;
    void* on_send_complete_context;
} ASYNC_SOCKET_IO_CONTEXT;

typedef struct ASYNC_SOCKET_RECV_CONTEXT_TAG
{
    S_LIST_ENTRY link;
    ASYNC_SOCKET* async_socket;
    uint32_t total_buffer_bytes;
    uint32_t total_buffer_count;
    int socket_handle;
    ON_ASYNC_SOCKET_RECEIVE_COMPLETE on_receive_complete;
    void* on_receive_complete_context;
    ASYNC_SOCKET_BUFFER recv_buffers[];
} ASYNC_SOCKET_RECV_CONTEXT;

typedef struct ASYNC_SOCKET_SEND_CONTEXT_TAG
{
    ASYNC_SOCKET* async_socket;
    uint32_t total_buffer_bytes;
    uint32_t total_buffer_count;
    int socket_handle;
    ON_ASYNC_SOCKET_SEND_COMPLETE on_send_complete;
    void* on_send_complete_context;
    ASYNC_SOCKET_BUFFER buffers[];
} ASYNC_SOCKET_SEND_CONTEXT;

static int remove_item_from_list(ASYNC_SOCKET* async_socket, ASYNC_SOCKET_RECV_CONTEXT* recv_context)
{
    int result;
    do
    {
        int32_t current_val = interlocked_compare_exchange(&async_socket->recv_data_access, 0, 1);
        if (current_val == 0)
        {
            break;
        }
        else
        {
            // Do Nothing
        }
        (void)wait_on_address(&async_socket->recv_data_access, current_val, UINT32_MAX);
    } while (true);

    if (s_list_remove(&async_socket->recv_data_head, &recv_context->link) != 0)
    {
        LogError("failure removing receive data to list %p", recv_context);
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }
    (void)interlocked_exchange(&async_socket->recv_data_access, 0);
    wake_by_address_single(&async_socket->recv_data_access);

    return result;
}

static int add_item_to_list(ASYNC_SOCKET* async_socket, ASYNC_SOCKET_RECV_CONTEXT* recv_context)
{
    int result;
    do
    {
        int32_t current_val = interlocked_compare_exchange(&async_socket->recv_data_access, 0, 1);
        if (current_val == 0)
        {
            break;
        }
        else
        {
            // Do Nothing wait for address
        }
        (void)wait_on_address(&async_socket->recv_data_access, current_val, UINT32_MAX);
    } while (true);

    if (s_list_add(&async_socket->recv_data_head, &recv_context->link) != 0)
    {
        LogError("failure adding receive data to list");
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }
    (void)interlocked_exchange(&async_socket->recv_data_access, 0);
    wake_by_address_single(&async_socket->recv_data_access);

    return result;
}

static int send_data(ASYNC_SOCKET* async_socket, const ASYNC_SOCKET_BUFFER* buff_dat)
{
    int result;

    ssize_t data_sent = 0;
    do
    {
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_052: [ async_socket_send_async shall attempt to send the data by calling send with the MSG_NOSIGNAL flag to ensure an exception is not generated. ]
        ssize_t send_size = send(async_socket->socket_handle, buff_dat->buffer, buff_dat->length + data_sent, MSG_NOSIGNAL);
        if (send_size < 0)
        {
            // Log will be done after it is determined if this is an actual error
            result = MU_FAILURE;
            break;
        }
        else
        {
            result = 0;
            data_sent += send_size;
        }
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_053: [ async_socket_send_async shall continue to send the data until the payload length has been sent. ]
    } while(data_sent < buff_dat->length);
    return result;
}

static int thread_worker_func(void* parameter)
{
    (void)parameter;

    // Loop while true
    do
    {
        struct epoll_event events[MAX_EVENTS_NUM];
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_079: [ thread_worker_func shall call epoll_wait waiting for the epoll to become signaled. ]
        int num_ready = epoll_wait(g_epoll, events, MAX_EVENTS_NUM, EVENTS_TIMEOUT_MS);
        if (num_ready == -1)
        {
            LogErrorNo("Failure epoll_wait, MAX_EVENTS_NUM: %d, EVENTS_TIMEOUT_MS: %d", MAX_EVENTS_NUM, EVENTS_TIMEOUT_MS);
        }
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_080: [ Onced signaled thread_worker_func shall loop through the signaled epolls. ]
        for (int index = 0; index < num_ready; index++)
        {
            // Codes_SRS_ASYNC_SOCKET_LINUX_11_081: [ If the events value contains EPOLLRDHUP (hang up), thread_worker_func shall the following: ]
            if (events[index].events & EPOLLRDHUP)
            {
                // Codes_SRS_ASYNC_SOCKET_LINUX_11_082: [ thread_worker_func shall receive the ASYNC_SOCKET_RECV_CONTEXT value from the ptr variable from the epoll_event data ptr. ]
                ASYNC_SOCKET_RECV_CONTEXT* recv_context = events[index].data.ptr;
                if (recv_context != NULL)
                {
                    ASYNC_SOCKET* async_socket = recv_context->async_socket;
                    // Codes_SRS_ASYNC_SOCKET_LINUX_11_083: [ The ASYNC_SOCKET_RECV_CONTEXT object shall be removed from list of stored pointers. ]
                    if (remove_item_from_list(recv_context->async_socket, recv_context) != 0)
                    {
                        LogError("Failure receive context has been previously removed %p", recv_context);
                    }
                    else
                    {
                        // Codes_SRS_ASYNC_SOCKET_LINUX_11_084: [ Then call the on_receive_complete callback with the on_receive_complete_context and ASYNC_SOCKET_RECEIVE_ABANDONED. ]
                        recv_context->on_receive_complete(recv_context->on_receive_complete_context, ASYNC_SOCKET_RECEIVE_ABANDONED, 0);
                        free(recv_context);
                    }
                }
            }
            // Codes_SRS_ASYNC_SOCKET_LINUX_11_085: [ If the events value contains EPOLLIN (recv), thread_worker_func shall the following: ]
            else if (events[index].events & EPOLLIN)
            {
                // Receive data from the socket
                ASYNC_SOCKET_RECEIVE_RESULT receive_result;
                // Codes_SRS_ASYNC_SOCKET_LINUX_11_086: [ thread_worker_func shall receive the ASYNC_SOCKET_RECV_CONTEXT value from the ptr variable from the epoll_event data ptr. ]
                ASYNC_SOCKET_RECV_CONTEXT* recv_context = events[index].data.ptr;

                if (recv_context != NULL)
                {
                    uint32_t index = 0;
                    ssize_t recv_size;
                    do
                    {
                        // Codes_SRS_ASYNC_SOCKET_LINUX_11_087: [ Then thread_worker_func shall call recv and do the following: ]
                        recv_size = recv(recv_context->socket_handle, recv_context->recv_buffers[index].buffer, recv_context->recv_buffers[index].length, 0);
                        if (recv_size < 0)
                        {
                            if (errno == EAGAIN || errno == EWOULDBLOCK)
                            {
                                // Codes_SRS_ASYNC_SOCKET_LINUX_11_089: [ If errno is EAGAIN or EWOULDBLOCK, then unlikely errors will continue ]
                                receive_result = ASYNC_SOCKET_RECEIVE_OK;
                                break;
                            }
                            else
                            {
                                recv_size = 0;
                                if (errno == ECONNRESET)
                                {
                                    // Codes_SRS_ASYNC_SOCKET_LINUX_11_090: [ If errno is ECONNRESET, then thread_worker_func shall call the on_receive_complete callback with the on_receive_complete_context and ASYNC_SOCKET_RECEIVE_ABANDONED ]
                                    receive_result = ASYNC_SOCKET_RECEIVE_ABANDONED;
                                    LogInfo("A reset on the recv socket has been encountered");
                                }
                                else
                                {
                                    // Error here
                                    receive_result = ASYNC_SOCKET_RECEIVE_ERROR;
                                    LogError("failure recv error: %d", errno);
                                }
                                break;
                            }
                        }
                        else if (recv_size == 0)
                        {
                            // Codes_SRS_ASYNC_SOCKET_LINUX_11_091: [ If the recv size equal 0, then thread_worker_func shall call on_receive_complete callback with the on_receive_complete_context and ASYNC_SOCKET_RECEIVE_OK ]
                            receive_result = ASYNC_SOCKET_RECEIVE_OK;
                            break;
                        }
                        else
                        {
                            // Codes_SRS_ASYNC_SOCKET_LINUX_11_092: [ If the recv size > 0, if we have another buffer to fill then we will attempt another read, otherwise we shall call on_receive_complete callback with the on_receive_complete_context and ASYNC_SOCKET_RECEIVE_OK ]
                            if (index + 1 >= recv_context->total_buffer_count || recv_size < recv_context->recv_buffers[index].length)
                            {
                                // We gotten all the data we need to get or we don't have any more recv space
                                receive_result = ASYNC_SOCKET_RECEIVE_OK;
                                break;
                            }
                            else
                            {
                                index++;
                            }
                        }
                    } while (true);

                    // Codes_SRS_ASYNC_SOCKET_LINUX_11_093: [ The ASYNC_SOCKET_RECV_CONTEXT object shall be removed from list of stored pointers. ]
                    if (remove_item_from_list(recv_context->async_socket, recv_context) != 0)
                    {
                        LogError("Failure removing receive context %p", recv_context);
                    }

                    if (recv_size >= 0)
                    {
                        // Call the callback
                        recv_context->on_receive_complete(recv_context->on_receive_complete_context, receive_result, recv_size);
                    }

                    // Free the memory
                    free(recv_context);
                }
            }
            // Codes_SRS_ASYNC_SOCKET_LINUX_11_094: [ If the events value contains EPOLLOUT (send), thread_worker_func shall the following: ]
            else if (events[index].events & EPOLLOUT)
            {
                // Codes_SRS_ASYNC_SOCKET_LINUX_11_095: [ thread_worker_func shall receive the ASYNC_SOCKET_SEND_CONTEXT value from the ptr variable from the epoll_event data ptr. ]
                ASYNC_SOCKET_SEND_CONTEXT* send_context = events[index].data.ptr;

                uint32_t index;
                // Codes_SRS_ASYNC_SOCKET_LINUX_11_096: [ thread_worker_func shall loop through the total buffers and send the data. ]
                for (index = 0; index < send_context->total_buffer_count; index++)
                {
                    ssize_t send_size = 0;
                    ASYNC_SOCKET_SEND_RESULT send_result = ASYNC_SOCKET_SEND_OK;
                    // loop here to send all the bytes
                    do
                    {
                        void* send_pos = send_context->buffers[index].buffer + send_size;
                        uint32_t send_length = send_context->buffers[index].length - send_size;
                        // Setting MSG_NOSIGNAL so the send doesn't generate a exception when the other end closes the socket
                        send_size += send(send_context->socket_handle, send_pos, send_length, MSG_NOSIGNAL);
                        // Codes_SRS_ASYNC_SOCKET_LINUX_11_097: [ if send returns value is < 0 thread_worker_func shall do the following: ]
                        if (send_size < 0 && (errno != EAGAIN && errno != EWOULDBLOCK) )
                        {
                            if (errno == ECONNRESET)
                            {
                                // Codes_SRS_ASYNC_SOCKET_LINUX_11_098: [ if errno is ECONNRESET, then on_send_complete shall be called with ASYNC_SOCKET_SEND_ABANDONED. ]
                                send_result = ASYNC_SOCKET_SEND_ABANDONED;
                                LogInfo("A reset on the send socket has been encountered");
                            }
                            else
                            {
                                // Codes_SRS_ASYNC_SOCKET_LINUX_11_099: [ if errno is anything else, then on_send_complete shall be called with ASYNC_SOCKET_SEND_ERROR. ]
                                send_result = ASYNC_SOCKET_SEND_ERROR;
                                // Log Error here
                                LogError("failure sending data length: %" PRIu32 "", send_length);
                            }
                            break;
                        }
                    } while (send_size < send_context->buffers[index].length);

                    send_context->on_send_complete(send_context->on_send_complete_context, send_result);
                }
                free(send_context);
            }
        }
    // Codes_SRS_ASYNC_SOCKET_LINUX_11_100: [ If the thread_access_cnt variable is not 0, thread_worker_func continue, otherwise it shall exit ]
    } while (interlocked_add(&g_thread_access_cnt, 0) > 0);
    return 0;
}

static int initialize_global_thread(void)
{
    int result = -1;

    // Codes_SRS_ASYNC_SOCKET_LINUX_11_008: [ initialize_global_thread shall increment the global g_thread_access_cnt variable. ]
    int32_t current_count = interlocked_increment(&g_thread_access_cnt);
    // Codes_SRS_ASYNC_SOCKET_LINUX_11_009: [ If the g_thread_access_cnt count is 1, initialize_global_thread shall do the following: ]
    if (current_count == 1)
    {
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_010: [ initialize_global_thread shall create the epoll variable by calling epoll_create. ]
        g_epoll = epoll_create(MAX_EVENTS_NUM);
        if (g_epoll == -1)
        {
            LogErrorNo("failure epoll_create MAX_EVENTS_NUM: %d", MAX_EVENTS_NUM);
            interlocked_decrement(&g_thread_access_cnt);
        }
        else
        {
            result = g_epoll;

            int32_t index;
            // Codes_SRS_ASYNC_SOCKET_LINUX_11_011: [ initialize_global_thread shall create the threads specified in THREAD_COUNT by calling ThreadAPI_Create. ]
            for (index = 0; index < THREAD_COUNT; index++)
            {
                if (ThreadAPI_Create(&g_thread_array[index], thread_worker_func, NULL) != THREADAPI_OK)
                {
                    LogCritical("Failure creating thread %" PRId32 "", index);
                    break;
                }
            }
            if (index < THREAD_COUNT)
            {
                // stop the threads that were started
                for (int32_t count = 0; count < index; count++)
                {
                    int dont_care;
                    if (ThreadAPI_Join(g_thread_array[index], &dont_care) != THREADAPI_OK)
                    {
                        LogError("Failure joining thread number %" PRId32 "", index);
                    }
                }
                // Codes_SRS_ASYNC_SOCKET_LINUX_11_012: [ If any error occurs initialize_global_thread shall fail and return -1. ]
                result = -1;
                (void)close(g_epoll);
                g_epoll = -1;
                interlocked_decrement(&g_thread_access_cnt);
            }
        }
    }
    else
    {
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_013: [ On success initialize_global_thread shall return the value returned by epoll_create. ]
        result = g_epoll;
    }
    return result;
}

static void deinitialize_global_thread(void)
{
    // Codes_SRS_ASYNC_SOCKET_LINUX_11_015: [ deinitialize_global_thread shall decrement the global g_thread_access_cnt variable. ]
    int32_t current_count = interlocked_decrement(&g_thread_access_cnt);
    // Codes_SRS_ASYNC_SOCKET_LINUX_11_016: [ If the g_thread_access_cnt count is 0, deinitialize_global_thread shall do the following: ]
    if (current_count == 0)
    {
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_017: [ deinitialize_global_thread shall call close on the global epoll variable. ]
        (void)close(g_epoll);

        // Codes_SRS_ASYNC_SOCKET_LINUX_11_018: [ deinitialize_global_thread shall wait for the global threads to close by calling ThreadAPI_Join. ]
        for (int32_t index = 0; index < THREAD_COUNT; index++)
        {
            int dont_care;
            if (ThreadAPI_Join(g_thread_array[index], &dont_care) != THREADAPI_OK)
            {
                LogError("Failure joining thread number %" PRId32 "", index);
            }
        }
    }
}

static void internal_close(ASYNC_SOCKET_HANDLE async_socket)
{
    int32_t value;
    // Codes_SRS_ASYNC_SOCKET_LINUX_11_037: [ async_socket_close shall wait for all executing async_socket_send_async and async_socket_receive_async APIs. ]
    while ((value = interlocked_add(&async_socket->pending_api_calls, 0)) != 0)
    {
        (void)wait_on_address(&async_socket->pending_api_calls, value, UINT32_MAX);
    }

    // Codes_SRS_ASYNC_SOCKET_LINUX_11_038: [ Then async_socket_close shall remove the underlying socket form the epoll by calling epoll_ctl with EPOLL_CTL_DEL. ]
    if (epoll_ctl(async_socket->epoll, EPOLL_CTL_DEL, async_socket->socket_handle, NULL) == -1)
    {
        LogErrorNo("Failure epoll_ctrl with EPOLL_CTL_DEL");
    }

    // Codes_SRS_ASYNC_SOCKET_LINUX_11_039: [ async_socket_close shall call close on the underlying socket. ]
    (void)close(async_socket->socket_handle);
    async_socket->socket_handle = INVALID_SOCKET;

    // Ensure that all the cleanup on the epoll is complete
    do
    {
        int32_t current_val = interlocked_compare_exchange(&async_socket->recv_data_access, 0, 1);
        if (current_val == 0)
        {
            break;
        }
        else
        {
            // Do Nothing
        }
        (void)wait_on_address(&async_socket->recv_data_access, current_val, UINT32_MAX);
    } while (true);

    // Codes_SRS_ASYNC_SOCKET_LINUX_11_040: [ async_socket_close shall remove any memory that is stored in the epoll system ]
    PS_LIST_ENTRY entry;
    while ((entry = s_list_remove_head(&async_socket->recv_data_head)) != &async_socket->recv_data_head)
    {
        ASYNC_SOCKET_RECV_CONTEXT* recv_context = CONTAINING_RECORD(entry, ASYNC_SOCKET_RECV_CONTEXT, link);
        recv_context->on_receive_complete(recv_context->on_receive_complete_context, ASYNC_SOCKET_RECEIVE_ABANDONED, 0);
        free(recv_context);
    }
    (void)interlocked_exchange(&async_socket->recv_data_access, 0);
    wake_by_address_single(&async_socket->recv_data_access);

    // Codes_SRS_ASYNC_SOCKET_LINUX_11_041: [ async_socket_close shall set the state to closed. ]
    (void)interlocked_exchange(&async_socket->state, ASYNC_SOCKET_LINUX_STATE_CLOSED);
    wake_by_address_single(&async_socket->state);
}

ASYNC_SOCKET_HANDLE async_socket_create(EXECUTION_ENGINE_HANDLE execution_engine, SOCKET_HANDLE socket_handle)
{
    ASYNC_SOCKET_HANDLE result;
    if (
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_002: [ If execution_engine is NULL, async_socket_create shall fail and return NULL. ]
        (execution_engine == NULL) ||
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_003: [ If socket_handle is INVALID_SOCKET, async_socket_create shall fail and return NULL. ]
        (socket_handle == INVALID_SOCKET))
    {
        LogError("EXECUTION_ENGINE_HANDLE execution_engine:%p, SOCKET_HANDLE socket_handle:%" PRI_SOCKET "", execution_engine, socket_handle);
    }
    else
    {
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_001: [ async_socket_create shall allocate a new async socket and on success shall return a non-NULL handle. ]
        result = malloc(sizeof(ASYNC_SOCKET));
        if (result == NULL)
        {
            LogError("failure allocating asyn socket %zu", sizeof(ASYNC_SOCKET));
        }
        else
        {
            if (s_list_initialize(&result->recv_data_head) != 0)
            {
                LogError("failure initializing recv list");
            }
            else
            {
                // Codes_SRS_ASYNC_SOCKET_LINUX_11_004: [ async_socket_create shall increment the reference count on execution_engine. ]
                execution_engine_inc_ref(execution_engine);
                result->execution_engine = execution_engine;

                // Codes_SRS_ASYNC_SOCKET_LINUX_11_005: [ async_socket_create shall initialize the global thread. ]
                result->epoll = initialize_global_thread();
                if (result->epoll == -1)
                {
                    LogError("Failure initializing global thread");
                }
                else
                {
                    result->socket_handle = socket_handle;

                    (void)interlocked_exchange(&result->epoll_cleanup, 0);
                    (void)interlocked_exchange(&result->recv_data_access, 0);
                    (void)interlocked_exchange(&result->pending_api_calls, 0);
                    (void)interlocked_exchange(&result->state, ASYNC_SOCKET_LINUX_STATE_CLOSED);
                    goto all_ok;
                }
            }
            free(result);
        }
    }
    // Codes_SRS_ASYNC_SOCKET_LINUX_11_006: [ If any error occurs, async_socket_create shall fail and return NULL. ]
    result = NULL;
all_ok:
    return result;
}

void async_socket_destroy(ASYNC_SOCKET_HANDLE async_socket)
{
    // Codes_SRS_ASYNC_SOCKET_LINUX_11_019: [ If async_socket is NULL, async_socket_destroy shall return. ]
    if (async_socket == NULL)
    {
        LogError("Invalid arguments: ASYNC_SOCKET_HANDLE async_socket=%p", async_socket);
    }
    else
    {
        do
        {
            // Codes_SRS_ASYNC_SOCKET_LINUX_11_020: [ While async_socket is OPENING or CLOSING, async_socket_destroy shall wait for the open to complete either successfully or with error. ]
            int32_t current_state = interlocked_compare_exchange(&async_socket->state, ASYNC_SOCKET_LINUX_STATE_CLOSING, ASYNC_SOCKET_LINUX_STATE_OPEN);
            if (current_state == ASYNC_SOCKET_LINUX_STATE_OPEN)
            {
                // Codes_SRS_ASYNC_SOCKET_LINUX_11_021: [ async_socket_destroy shall perform an implicit close if async_socket is OPEN. ]
                internal_close(async_socket);
                break;
            }
            else if (current_state == ASYNC_SOCKET_LINUX_STATE_CLOSED)
            {
                break;
            }
            (void)wait_on_address(&async_socket->state, current_state, UINT32_MAX);
        } while (1);

        deinitialize_global_thread();
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_022: [ async_socket_destroy shall decrement the reference count on the execution engine. ]
        execution_engine_dec_ref(async_socket->execution_engine);
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_023: [ async_socket_destroy shall free all resources associated with async_socket. ]
        free(async_socket);
    }
}

int async_socket_open_async(ASYNC_SOCKET_HANDLE async_socket, ON_ASYNC_SOCKET_OPEN_COMPLETE on_open_complete, void* on_open_complete_context)
{
    int result;
    // Codes_SRS_ASYNC_SOCKET_LINUX_11_026: [ on_open_complete_context shall be allowed to be NULL. ]
    if (
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_024: [ If async_socket is NULL, async_socket_open_async shall fail and return a non-zero value. ]
        async_socket == NULL ||
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_025: [ If on_open_complete is NULL, async_socket_open_async shall fail and return a non-zero value. ]
        on_open_complete == NULL)
    {
        LogError("ASYNC_SOCKET_HANDLE async_socket=%p, ON_ASYNC_SOCKET_OPEN_COMPLETE on_open_complete=%p, void* on_open_complete_context=%p",
            async_socket, on_open_complete, on_open_complete_context);
        result = MU_FAILURE;
    }
    else
    {
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_027: [ Otherwise, async_socket_open_async shall switch the state to OPENING. ]
        int32_t current_state = interlocked_compare_exchange(&async_socket->state, (int32_t)ASYNC_SOCKET_LINUX_STATE_OPENING, (int32_t)ASYNC_SOCKET_LINUX_STATE_CLOSED);
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_029: [ If async_socket is already OPEN or OPENING, async_socket_open_async shall fail and return a non-zero value. ]
        if (current_state != ASYNC_SOCKET_LINUX_STATE_CLOSED)
        {
            LogError("Open called in state %" PRI_MU_ENUM "", MU_ENUM_VALUE(ASYNC_SOCKET_LINUX_STATE, current_state));
            result = MU_FAILURE;
        }
        else
        {
            // Codes_SRS_ASYNC_SOCKET_LINUX_11_030: [ If async_socket has already closed the underlying socket handle then async_socket_open_async shall fail and return a non-zero value. ]
            if (async_socket->socket_handle == INVALID_SOCKET)
            {
                LogError("Open called after socket was closed");
                result = MU_FAILURE;
            }
            else
            {
                // Add the socket to the epoll so it can be just modified later
                struct epoll_event ev = {0};
                ev.events = 0;
                // Codes_SRS_ASYNC_SOCKET_LINUX_11_031: [ async_socket_open_async shall add the socket to the epoll system by calling epoll_ctl with EPOLL_CTL_ADD. ]
                if (epoll_ctl(async_socket->epoll, EPOLL_CTL_ADD, async_socket->socket_handle, &ev) < 0)
                {
                    // Codes_SRS_ASYNC_SOCKET_LINUX_11_034: [ If any error occurs, async_socket_open_async shall fail and return a non-zero value. ]
                    LogErrorNo("failure with epoll_ctrl EPOLL_CTL_ADD");
                    result = MU_FAILURE;
                }
                else
                {
                    // Codes_SRS_ASYNC_SOCKET_LINUX_11_032: [ async_socket_open_async shall set the state to OPEN. ]
                    (void)interlocked_exchange(&async_socket->state, ASYNC_SOCKET_LINUX_STATE_OPEN);
                    // Codes_SRS_ASYNC_SOCKET_LINUX_11_033: [ On success async_socket_open_async shall call on_open_complete_context with ASYNC_SOCKET_OPEN_OK. ]
                    on_open_complete(on_open_complete_context, ASYNC_SOCKET_OPEN_OK);

                    // Codes_SRS_ASYNC_SOCKET_LINUX_11_028: [ On success, async_socket_open_async shall return 0. ]
                    result = 0;
                    goto all_ok;
                }
            }
            interlocked_exchange(&async_socket->state, ASYNC_SOCKET_LINUX_STATE_CLOSED);
        }
    }

all_ok:
    return result;
}

void async_socket_close(ASYNC_SOCKET_HANDLE async_socket)
{
    // Codes_SRS_ASYNC_SOCKET_LINUX_11_035: [ If async_socket is NULL, async_socket_close shall return. ]
    if (async_socket == NULL)
    {
        LogError("Invalid arguments: ASYNC_SOCKET_HANDLE async_socket=%p", async_socket);
    }
    else
    {
        ASYNC_SOCKET_LINUX_STATE current_state;
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_036: [ Otherwise, async_socket_close shall switch the state to CLOSING. ]
        if ( (current_state = interlocked_compare_exchange(&async_socket->state, ASYNC_SOCKET_LINUX_STATE_CLOSING, ASYNC_SOCKET_LINUX_STATE_OPEN)) != ASYNC_SOCKET_LINUX_STATE_OPEN)
        {
            // Codes_SRS_ASYNC_SOCKET_LINUX_11_042: [ If async_socket is not OPEN, async_socket_close shall return. ]
            LogWarning("Not open, current_state=%" PRI_MU_ENUM "", MU_ENUM_VALUE(ASYNC_SOCKET_LINUX_STATE, current_state));
        }
        else
        {
            internal_close(async_socket);
        }
    }
}

ASYNC_SOCKET_SEND_SYNC_RESULT async_socket_send_async(ASYNC_SOCKET_HANDLE async_socket, const ASYNC_SOCKET_BUFFER* buffers, uint32_t buffer_count, ON_ASYNC_SOCKET_SEND_COMPLETE on_send_complete, void* on_send_complete_context)
{
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    // Codes_SRS_ASYNC_SOCKET_LINUX_11_050: [ on_send_complete_context shall be allowed to be NULL. ]
    if (
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_043: [ If async_socket is NULL, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]
        async_socket == NULL ||
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_044: [ If buffers is NULL, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]
        buffers == NULL ||
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_045: [ If buffer_count is 0, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]
        buffer_count == 0 ||
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_049: [ If on_send_complete is NULL, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]
        on_send_complete == NULL
        )
    {
        LogError("Invalid arguments: ASYNC_SOCKET_HANDLE async_socket=%p, const ASYNC_SOCKET_BUFFER* payload=%p, uint32_t buffer_count=%" PRIu32 ", ON_ASYNC_SOCKET_SEND_COMPLETE on_send_complete=%p, void*, on_send_complete_context=%p",
            async_socket, buffers, buffer_count, on_send_complete, on_send_complete_context);
        result = ASYNC_SOCKET_SEND_SYNC_ERROR;
    }
    else
    {
        uint32_t index;
        uint32_t total_buffer_bytes = 0;

        for (index = 0; index < buffer_count; index++)
        {
            if (
                // Codes_SRS_ASYNC_SOCKET_LINUX_11_046: [ If any of the buffers in payload has buffer set to NULL, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]
                (buffers[index].buffer == NULL) ||
                // Codes_SRS_ASYNC_SOCKET_LINUX_11_047: [ If any of the buffers in payload has length set to 0, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]
                (buffers[index].length == 0)
                )
            {
                LogError("Invalid buffer %" PRIu32 ": buffer=%p, length = %" PRIu32, index, buffers[index].buffer, buffers[index].length);
                break;
            }
            // Codes_SRS_ASYNC_SOCKET_LINUX_11_048: [ If the sum of buffer lengths for all the buffers in payload is greater than UINT32_MAX, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]
            if (total_buffer_bytes + buffers[index].length < total_buffer_bytes)
            {
                LogError("Overflow in total buffer length computation (total_buffer_bytes=%" PRIu32 " + buffers[i=%" PRIu32 "].length=%" PRIu32 "", total_buffer_bytes, index, buffers[index].length);
                break;
            }
            else
            {
                total_buffer_bytes += buffers[index].length;
            }
        }

        if (index < buffer_count)
        {
            LogError("Invalid buffers passed to async_socket_send_async");
            result = ASYNC_SOCKET_SEND_SYNC_ERROR;
        }
        else
        {
            ASYNC_SOCKET_LINUX_STATE current_state;
            // Codes_SRS_ASYNC_SOCKET_LINUX_11_051: [ If async_socket is not OPEN, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ABANDONED. ]
            if ((current_state = interlocked_add(&async_socket->state, 0)) != ASYNC_SOCKET_LINUX_STATE_OPEN)
            {
                LogWarning("Not open, current state is %" PRI_MU_ENUM "", MU_ENUM_VALUE(ASYNC_SOCKET_LINUX_STATE, current_state));
                result = ASYNC_SOCKET_SEND_SYNC_ABANDONED;
            }
            else
            {
                (void)interlocked_increment(&async_socket->pending_api_calls);

                ASYNC_SOCKET_SEND_RESULT send_result;
                for (index = 0; index < buffer_count; index++)
                {
                    // Codes_SRS_ASYNC_SOCKET_LINUX_11_054: [ If the send fails to send the data, async_socket_send_async shall do the following: ]
                    if (send_data(async_socket, &buffers[index]) != 0)
                    {
                        // Codes_SRS_ASYNC_SOCKET_LINUX_11_055: [ If the errno value is EAGAIN or EWOULDBLOCK. ]
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            // Codes_SRS_ASYNC_SOCKET_LINUX_11_056: [ async_socket_send_async shall create a context for the send where the payload, on_send_complete and on_send_complete_context shall be stored. ]
                            ASYNC_SOCKET_SEND_CONTEXT* send_context = malloc_flex(sizeof(ASYNC_SOCKET_SEND_CONTEXT), buffer_count, sizeof(ASYNC_SOCKET_BUFFER));
                            if (send_context == NULL)
                            {
                                LogError("failure in malloc_flex(sizeof(ASYNC_SOCKET_SEND_CONTEXT)=%zu, buffer_count=%" PRIu32 ", sizeof(ASYNC_SOCKET_BUFFER)=%zu) failed",
                                    sizeof(ASYNC_SOCKET_SEND_CONTEXT), buffer_count, sizeof(ASYNC_SOCKET_BUFFER));
                                result = ASYNC_SOCKET_SEND_SYNC_ERROR;
                                send_result = ASYNC_SOCKET_SEND_ABANDONED;
                                break;
                            }
                            else
                            {
                                send_result = ASYNC_SOCKET_SEND_ERROR;
                                send_context->total_buffer_bytes = total_buffer_bytes;
                                send_context->total_buffer_count = buffer_count;
                                send_context->on_send_complete = on_send_complete;
                                send_context->on_send_complete_context = on_send_complete_context;
                                send_context->socket_handle = async_socket->socket_handle;
                                send_context->async_socket = async_socket;

                                send_context->buffers[index].buffer = buffers[index].buffer;
                                send_context->buffers[index].length = buffers[index].length;

                                struct epoll_event ev = {0};
                                ev.events = EPOLLOUT;
                                ev.data.ptr = (void*)send_context;
                                // Codes_SRS_ASYNC_SOCKET_LINUX_11_057: [ The the context shall then be added to the epoll system by calling epoll_ctl with EPOLL_CTL_MOD. ]
                                if (epoll_ctl(async_socket->epoll, EPOLL_CTL_MOD, async_socket->socket_handle, &ev) < 0)
                                {
                                    if (errno == ENOENT)
                                    {
                                        // Codes_SRS_ASYNC_SOCKET_LINUX_11_058: [ If the epoll_ctl call fails with ENOENT, async_socket_send_async shall call epoll_ctl again with EPOLL_CTL_ADD. ]
                                        if (epoll_ctl(async_socket->epoll, EPOLL_CTL_ADD, async_socket->socket_handle, &ev) < 0)
                                        {
                                            // Codes_SRS_ASYNC_SOCKET_LINUX_11_063: [ If any error occurs, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]
                                            LogErrorNo("failure with epoll_ctrl EPOLL_CTL_ADD");
                                            result = ASYNC_SOCKET_SEND_SYNC_ERROR;
                                        }
                                        else
                                        {
                                            result = ASYNC_SOCKET_SEND_SYNC_OK;
                                        }
                                    }
                                    else
                                    {
                                        LogErrorNo("failure with epoll_ctrl EPOLL_CTL_MOD");
                                        result = ASYNC_SOCKET_SEND_SYNC_ERROR;
                                    }
                                }
                                else
                                {
                                    result = ASYNC_SOCKET_SEND_SYNC_OK;
                                }
                            }
                        }
                        // Codes_SRS_ASYNC_SOCKET_LINUX_11_059: [ If the errno value is ECONNRESET, ENOTCONN, or EPIPE shall fail and return ASYNC_SOCKET_SEND_SYNC_ABANDONED. ]
                        else if (errno == ECONNRESET || errno == ENOTCONN || errno == EPIPE)
                        {
                            LogWarning("The connection was forcibly closed by the peer");
                            send_result = ASYNC_SOCKET_SEND_ABANDONED;
                            // Socket was closed
                            result = ASYNC_SOCKET_SEND_SYNC_ABANDONED;
                        }
                        else
                        {
                            // Codes_SRS_ASYNC_SOCKET_LINUX_11_060: [ If any other error is encountered, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]
                            LogError("failure sending socket error no: %d", errno);
                            result = ASYNC_SOCKET_SEND_SYNC_ERROR;
                            send_result = ASYNC_SOCKET_SEND_ERROR;
                        }
                    }
                    else
                    {
                        // Codes_SRS_ASYNC_SOCKET_LINUX_11_062: [ On success, async_socket_send_async shall return ASYNC_SOCKET_SEND_SYNC_OK. ]
                        send_result = ASYNC_SOCKET_SEND_OK;
                        result = ASYNC_SOCKET_SEND_SYNC_OK;
                    }
                }
                // Only call the callback if the call was successfully sent
                // Otherwise we're going to be returning an error
                if (send_result == ASYNC_SOCKET_SEND_OK)
                {
                    // Codes_SRS_ASYNC_SOCKET_LINUX_11_061: [ If the send is successful, async_socket_send_async shall call the on_send_complete with on_send_complete_context and ASYNC_SOCKET_SEND_SYNC_OK. ]
                    on_send_complete(on_send_complete_context, send_result);
                }
                else
                {
                    // Do nothing failure happend do not call the callback
                }
                (void)interlocked_decrement(&async_socket->pending_api_calls);
                wake_by_address_single(&async_socket->pending_api_calls);
            }
        }
    }

all_ok:
    return result;
}

int async_socket_receive_async(ASYNC_SOCKET_HANDLE async_socket, ASYNC_SOCKET_BUFFER* payload, uint32_t buffer_count, ON_ASYNC_SOCKET_RECEIVE_COMPLETE on_receive_complete, void* on_receive_complete_context)
{
    int result;
    // Codes_SRS_ASYNC_SOCKET_LINUX_11_071: [ on_receive_complete_context shall be allowed to be NULL. ]
    if (
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_064: [ If async_socket is NULL, async_socket_receive_async shall fail and return a non-zero value. ]
        async_socket == NULL ||
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_065: [ If buffers is NULL, async_socket_receive_async shall fail and return a non-zero value. ]
        payload == NULL ||
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_066: [ If buffer_count is 0, async_socket_receive_async shall fail and return a non-zero value. ]
        buffer_count == 0 ||
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_070: [ If on_receive_complete is NULL, async_socket_receive_async shall fail and return a non-zero value. ]
        on_receive_complete == NULL
        )
    {
        LogError("Invalid arguments: ASYNC_SOCKET_HANDLE async_socket=%p, const ASYNC_SOCKET_BUFFER* payload=%p, uint32_t buffer_count=%" PRIu32 ", ON_ASYNC_SOCKET_RECEIVE_COMPLETE on_receive_complete=%p, void*, on_receive_complete_context=%p",
            async_socket, payload, buffer_count, on_receive_complete, on_receive_complete_context);
        result = MU_FAILURE;
    }
    else
    {
        uint32_t total_buffer_bytes = 0;
        uint32_t index;
        for (index = 0; index < buffer_count; index++)
        {
            if (
                // Codes_SRS_ASYNC_SOCKET_LINUX_11_067: [ If any of the buffers in payload has buffer set to NULL, async_socket_receive_async shall fail and return a non-zero value. ]
                payload[index].buffer == NULL ||
                // Codes_SRS_ASYNC_SOCKET_LINUX_11_068: [ If any of the buffers in payload has length set to 0, async_socket_receive_async shall fail and return a non-zero value. ]
                payload[index].length == 0
                )
            {
                LogError("Invalid buffer %" PRIu32 ": buffer=%p, length = %" PRIu32, index, payload[index].buffer, payload[index].length);
                break;
            }

            // Codes_SRS_ASYNC_SOCKET_LINUX_11_069: [ If the sum of buffer lengths for all the buffers in payload is greater than UINT32_MAX, async_socket_receive_async shall fail and return a non-zero value. ]
            if (total_buffer_bytes + payload[index].length < total_buffer_bytes)
            {
                LogError("Overflow in total buffer length computation total_buffer_bytes=%" PRIu32 " + payload[i=%" PRIu32 "].length=%" PRIu32 "", total_buffer_bytes, index, payload[index].length);
                break;
            }
            else
            {
                total_buffer_bytes += payload[index].length;
            }
        }
        if (index < buffer_count)
        {
            LogError("Invalid buffers passed to async_socket_receive_async");
            result = MU_FAILURE;
        }
        else
        {
            ASYNC_SOCKET_LINUX_STATE current_state;
            // Codes_SRS_ASYNC_SOCKET_LINUX_11_072: [ If async_socket is not OPEN, async_socket_receive_async shall fail and return a non-zero value. ]
            if ((current_state = interlocked_add(&async_socket->state, 0)) != ASYNC_SOCKET_LINUX_STATE_OPEN)
            {
                /* Codes_SRS_ASYNC_SOCKET_WIN32_01_098: [ If async_socket is not OPEN, async_socket_receive_async shall fail and return a non-zero value. ]*/
                LogWarning("Not open, current state is %" PRI_MU_ENUM "", MU_ENUM_VALUE(ASYNC_SOCKET_LINUX_STATE, current_state));
                result = MU_FAILURE;
            }
            else
            {
                (void)interlocked_increment(&async_socket->pending_api_calls);

                // Codes_SRS_ASYNC_SOCKET_LINUX_11_074: [ The context shall also allocate enough memory to keep an array of buffer_count items. ]
                ASYNC_SOCKET_RECV_CONTEXT* recv_context = malloc_flex(sizeof(ASYNC_SOCKET_RECV_CONTEXT), buffer_count, sizeof(ASYNC_SOCKET_BUFFER));
                if (recv_context == NULL)
                {
                    LogError("failure in malloc_flex(sizeof(ASYNC_SOCKET_RECV_CONTEXT)=%zu, buffer_count=%" PRIu32 ", sizeof(ASYNC_SOCKET_BUFFER)=%zu) failed",
                        sizeof(ASYNC_SOCKET_RECV_CONTEXT), buffer_count, sizeof(ASYNC_SOCKET_BUFFER));
                    result = ASYNC_SOCKET_SEND_SYNC_ERROR;
                }
                else
                {
                    // Codes_SRS_ASYNC_SOCKET_LINUX_11_073: [ Otherwise async_socket_receive_async shall create a context for the send where the payload, on_receive_complete and on_receive_complete_context shall be stored. ]
                    recv_context->total_buffer_bytes = total_buffer_bytes;
                    recv_context->total_buffer_count = buffer_count;
                    recv_context->on_receive_complete = on_receive_complete;
                    recv_context->on_receive_complete_context = on_receive_complete_context;
                    recv_context->socket_handle = async_socket->socket_handle;
                    recv_context->async_socket = async_socket;
                    recv_context->link.next = NULL;

                    for (index = 0; index < buffer_count; index++)
                    {
                        recv_context->recv_buffers[index].buffer = payload[index].buffer;
                        recv_context->recv_buffers[index].length = payload[index].length;
                    }

                    if (add_item_to_list(async_socket, recv_context) != 0)
                    {
                        LogError("failure adding receive data to list");
                        result = MU_FAILURE;
                    }
                    else
                    {
                        struct epoll_event ev = {0};
                        ev.events = EPOLLIN | EPOLLRDHUP | EPOLLONESHOT;
                        ev.data.ptr = recv_context;
                        // Codes_SRS_ASYNC_SOCKET_LINUX_11_075: [ async_socket_receive_async shall add the socket in the epoll system by calling epoll_ctl with EPOLL_CTL_MOD ]
                        if (epoll_ctl(async_socket->epoll, EPOLL_CTL_MOD, async_socket->socket_handle, &ev) < 0)
                        {
                            if (errno == ENOENT)
                            {
                                // Codes_SRS_ASYNC_SOCKET_LINUX_11_076: [ If the epoll_ctl call fails with ENOENT, async_socket_send_async shall call epoll_ctl again with EPOLL_CTL_ADD. ]
                                if (epoll_ctl(async_socket->epoll, EPOLL_CTL_ADD, async_socket->socket_handle, &ev) < 0)
                                {
                                    // Codes_SRS_ASYNC_SOCKET_LINUX_11_078: [ If any error occurs, async_socket_receive_async shall fail and return a non-zero value. ]
                                    LogErrorNo("failure with epoll_ctrl EPOLL_CTL_ADD");
                                    result = MU_FAILURE;
                                }
                                else
                                {
                                    // Codes_SRS_ASYNC_SOCKET_LINUX_11_077: [ On success, async_socket_receive_async shall return 0. ]
                                    result = 0;
                                }
                            }
                            else
                            {
                                // Codes_SRS_ASYNC_SOCKET_LINUX_11_078: [ If any error occurs, async_socket_receive_async shall fail and return a non-zero value. ]
                                LogErrorNo("failure with epoll_ctrl EPOLL_CTL_MOD");
                                result = MU_FAILURE;
                            }
                        }
                        else
                        {
                            // Codes_SRS_ASYNC_SOCKET_LINUX_11_077: [ On success, async_socket_receive_async shall return 0. ]
                            result = 0;
                        }

                        if (result != 0)
                        {
                            remove_item_from_list(async_socket, recv_context);
                        }
                        else
                        {
                            (void)interlocked_decrement(&async_socket->pending_api_calls);
                            wake_by_address_single(&async_socket->pending_api_calls);
                            goto all_ok;
                        }
                    }
                    free(recv_context);
                }
                (void)interlocked_decrement(&async_socket->pending_api_calls);
                wake_by_address_single(&async_socket->pending_api_calls);
            }
        }
    }

all_ok:
    return result;
}
