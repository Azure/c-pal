// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>

#include <sys/epoll.h>
#include <sys/socket.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/xlogging.h"

#include "c_pal/gballoc_hl.h"        // IWYU pragma: keep
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/execution_engine.h"
#include "c_pal/interlocked.h"
#include "c_pal/sync.h"
#include "c_pal/socket_handle.h"
#include "c_pal/threadapi.h"

#include "c_pal/async_socket.h"

#define MAX_EVENTS_NUM      64
#define EVENTS_TIMEOUT_MS   2*1000

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

// These will be in a separate file maybe along with the
// thread function?
static volatile_atomic int32_t g_threads_num;
static int g_epoll;
static THREAD_HANDLE g_thread_array[THREAD_COUNT];

typedef struct ASYNC_SOCKET_TAG
{
    int socket_handle;
    EXECUTION_ENGINE_HANDLE execution_engine;

    volatile_atomic int32_t state;
    volatile_atomic int32_t pending_api_calls;

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

static int thread_worker_func(void* parameter)
{
    (void)parameter;

    // Loop while true
    do
    {
        struct epoll_event events[MAX_EVENTS_NUM];
        int num_ready = epoll_wait(g_epoll, events, MAX_EVENTS_NUM, EVENTS_TIMEOUT_MS);
        for(int index = 0; index < num_ready; index++)
        {
            if (events[index].events & EPOLLRDHUP)
            {
                ASYNC_SOCKET_RECV_CONTEXT* recv_context = events[index].data.ptr;
                if (recv_context != NULL)
                {
                    recv_context->on_receive_complete(recv_context->on_receive_complete_context, ASYNC_SOCKET_RECEIVE_ABANDONED, 0);

                    (void)interlocked_decrement(&recv_context->async_socket->pending_api_calls);
                    wake_by_address_single(&recv_context->async_socket->pending_api_calls);

                    free(recv_context);
                }
            }
            else if (events[index].events & EPOLLIN)
            {
                // Receive data from the socket
                ASYNC_SOCKET_RECEIVE_RESULT receive_result;
                ASYNC_SOCKET_RECV_CONTEXT* recv_context = events[index].data.ptr;

                if (recv_context != NULL)
                {
                    uint32_t index = 0;
                    ssize_t recv_size;
                    do
                    {

                        recv_size = recv(recv_context->socket_handle, recv_context->recv_buffers[index].buffer, recv_context->recv_buffers[index].length, 0);
                        if (recv_size < 0)
                        {
                            if (errno == EAGAIN || errno == EWOULDBLOCK)
                            {
                                // No more data to recv
                                receive_result = ASYNC_SOCKET_RECEIVE_OK;
                                break;
                            }
                            else
                            {
                                if (errno == ECONNRESET)
                                {
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
                            receive_result = ASYNC_SOCKET_RECEIVE_OK;
                            break;
                        }
                        else
                        {
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
                    // Call the callback
                    recv_context->on_receive_complete(recv_context->on_receive_complete_context, receive_result, recv_size);

                    (void)interlocked_decrement(&recv_context->async_socket->pending_api_calls);
                    wake_by_address_single(&recv_context->async_socket->pending_api_calls);

                    // Free the memory
                    free(recv_context);
                }
            }
            else if (events[index].events & EPOLLOUT)
            {
                ASYNC_SOCKET_SEND_CONTEXT* send_context = events[index].data.ptr;

                uint32_t index;
                for (index = 0; index < send_context->total_buffer_count; index++)
                {
                    ssize_t send_size = 0;
                    ASYNC_SOCKET_SEND_RESULT send_result = ASYNC_SOCKET_SEND_OK;
                    // loop here to send all the bytes
                    do
                    {
                        void* send_pos = send_context->buffers[index].buffer + send_size;
                        uint32_t send_length = send_context->buffers[index].length - send_size;
                        send_size += send(send_context->socket_handle, send_pos, send_length, MSG_NOSIGNAL);
                        if (send_size < 0 && (errno != EAGAIN && errno != EWOULDBLOCK) )
                        {
                            if (errno == ECONNRESET)
                            {
                                send_result = ASYNC_SOCKET_SEND_ABANDONED;
                                LogInfo("A reset on the send socket has been encountered");
                            }
                            else
                            {
                                send_result = ASYNC_SOCKET_SEND_ERROR;
                                // Log Error here
                                LogError("failure sending data length: %" PRIu32 "", send_length);
                                break;
                            }
                        }
                    } while (send_size < send_context->buffers[index].length);

                    send_context->on_send_complete(send_context->on_send_complete_context, send_result);
                }
                free(send_context);
            }
        }
    } while (interlocked_add(&g_threads_num, 0) > 0);
    return 0;
}

static int initialize_global_thread(void)
{
    int result = -1;

    int32_t current_count = interlocked_increment(&g_threads_num);
    if (current_count <= THREAD_COUNT)
    {
        g_epoll = epoll_create(MAX_EVENTS_NUM);
        if (g_epoll == -1)
        {
            LogError("failure epoll_create error num: %d", errno);
        }
        else
        {
            result = g_epoll;
            for (int32_t index = 0; index < THREAD_COUNT; index++)
            {
                if (ThreadAPI_Create(&g_thread_array[index], thread_worker_func, NULL) != THREADAPI_OK)
                {
                    LogCritical("Failure creating thread %" PRId32 "", index);
                    result = -1;
                    (void)close(g_epoll);
                    break;
                }
            }
        }
    }
    else
    {
        result = g_epoll;
    }
    return result;
}

static void deinitialize_global_thread(void)
{
    int32_t current_count = interlocked_decrement(&g_threads_num);
    if (current_count == 0)
    {
        (void)close(g_epoll);

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
    while ((value = interlocked_add(&async_socket->pending_api_calls, 0)) != 0)
    {
        (void)wait_on_address(&async_socket->pending_api_calls, value, UINT32_MAX);
    }

    // Remove the socket from the epoll
    if (epoll_ctl(async_socket->epoll, EPOLL_CTL_DEL, async_socket->socket_handle, NULL) == -1)
    {
        LogError("Failure removing socket from epoll_ctrl");
    }

    LogInfo("async socket is closing socket: %" PRI_MU_SOCKET "", async_socket->socket_handle);

    // Close the socket
    (void)close(async_socket->socket_handle);
    async_socket->socket_handle = INVALID_SOCKET;

    (void)interlocked_exchange(&async_socket->state, ASYNC_SOCKET_LINUX_STATE_CLOSED);
    wake_by_address_single(&async_socket->state);
}

ASYNC_SOCKET_HANDLE async_socket_create(EXECUTION_ENGINE_HANDLE execution_engine, SOCKET_HANDLE socket_handle)
{
    ASYNC_SOCKET_HANDLE result;
    if (
        (execution_engine == NULL) ||
        (socket_handle == INVALID_SOCKET))
    {
        LogError("EXECUTION_ENGINE_HANDLE execution_engine=%p, SOCKET_HANDLE socket_handle=%d", execution_engine, socket_handle);
    }
    else
    {
        result = malloc(sizeof(ASYNC_SOCKET));
        if (result == NULL)
        {
            LogError("failure allocating asyn socket %zu", sizeof(ASYNC_SOCKET));
        }
        else
        {
            execution_engine_inc_ref(execution_engine);
            result->execution_engine = execution_engine;

            result->epoll = initialize_global_thread();
            if (result->epoll == -1)
            {
                LogError("failure epoll_create error num: %d", errno);
            }
            else
            {
                result->socket_handle = socket_handle;

                (void)interlocked_exchange(&result->pending_api_calls, 0);
                (void)interlocked_exchange(&result->state, ASYNC_SOCKET_LINUX_STATE_CLOSED);
                goto all_ok;
            }
            free(result);
        }
    }
    result = NULL;
all_ok:
    return result;
}

void async_socket_destroy(ASYNC_SOCKET_HANDLE async_socket)
{
    if (async_socket == NULL)
    {
        LogError("Invalid arguments: ASYNC_SOCKET_HANDLE async_socket=%p", async_socket);
    }
    else
    {
        do
        {
            int32_t current_state = interlocked_compare_exchange(&async_socket->state, ASYNC_SOCKET_LINUX_STATE_CLOSING, ASYNC_SOCKET_LINUX_STATE_OPEN);
            if (current_state == ASYNC_SOCKET_LINUX_STATE_OPEN)
            {
                internal_close(async_socket);
                break;
            }
            else if (current_state == ASYNC_SOCKET_LINUX_STATE_CLOSED)
            {
                if (async_socket->socket_handle != INVALID_SOCKET)
                {
                    LogInfo("async socket destroy, closesocket(%d);", async_socket->socket_handle);

                    (void)close(async_socket->socket_handle);
                }
                break;
            }

            (void)wait_on_address(&async_socket->state, current_state, UINT32_MAX);
        } while (1);

        deinitialize_global_thread();
        execution_engine_dec_ref(async_socket->execution_engine);
        free(async_socket);
    }
}

int async_socket_open_async(ASYNC_SOCKET_HANDLE async_socket, ON_ASYNC_SOCKET_OPEN_COMPLETE on_open_complete, void* on_open_complete_context)
{
    int result;
    if (
        (async_socket == NULL) ||
        (on_open_complete == NULL))
    {
        LogError("ASYNC_SOCKET_HANDLE async_socket=%p, ON_ASYNC_SOCKET_OPEN_COMPLETE on_open_complete=%p, void* on_open_complete_context=%p",
            async_socket, on_open_complete, on_open_complete_context);
        result = MU_FAILURE;
    }
    else
    {
        int32_t current_state = interlocked_compare_exchange(&async_socket->state, (int32_t)ASYNC_SOCKET_LINUX_STATE_OPENING, (int32_t)ASYNC_SOCKET_LINUX_STATE_CLOSED);
        if (current_state != ASYNC_SOCKET_LINUX_STATE_CLOSED)
        {
            LogError("Open called in state %" PRI_MU_ENUM "", MU_ENUM_VALUE(ASYNC_SOCKET_LINUX_STATE, current_state));
            result = MU_FAILURE;
        }
        else
        {
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
                if (epoll_ctl(async_socket->epoll, EPOLL_CTL_ADD, async_socket->socket_handle, &ev) < 0)
                {
                    LogError("failure with epoll_ctrl EPOLL_CTL_ADD error no: %d", errno);
                    result = MU_FAILURE;
                }
                else
                {
                    interlocked_exchange(&async_socket->state, ASYNC_SOCKET_LINUX_STATE_OPEN);
                    on_open_complete(on_open_complete_context, ASYNC_SOCKET_OPEN_OK);

                    result = 0;
                    goto all_ok;
                }
                on_open_complete(on_open_complete_context, ASYNC_SOCKET_OPEN_ERROR);
            }
            interlocked_exchange(&async_socket->state, ASYNC_SOCKET_LINUX_STATE_CLOSED);
        }
    }

all_ok:
    return result;
}

void async_socket_close(ASYNC_SOCKET_HANDLE async_socket)
{
    if (async_socket == NULL)
    {
        LogError("Invalid arguments: ASYNC_SOCKET_HANDLE async_socket=%p", async_socket);
    }
    else
    {
        ASYNC_SOCKET_LINUX_STATE current_state;
        if ( (current_state = interlocked_compare_exchange(&async_socket->state, ASYNC_SOCKET_LINUX_STATE_CLOSING, ASYNC_SOCKET_LINUX_STATE_OPEN)) != ASYNC_SOCKET_LINUX_STATE_OPEN)
        {
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

    if (
        (async_socket == NULL) ||
        (buffers == NULL) ||
        (buffer_count == 0) ||
        (on_send_complete == NULL)
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
                (buffers[index].buffer == NULL) ||
                (buffers[index].length == 0)
                )
            {
                LogError("Invalid buffer %" PRIu32 ": buffer=%p, length = %" PRIu32, index, buffers[index].buffer, buffers[index].length);
                break;
            }

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
            int32_t value = interlocked_increment(&async_socket->pending_api_calls);

            ASYNC_SOCKET_LINUX_STATE current_state;
            if ((current_state = interlocked_add(&async_socket->state, 0)) != ASYNC_SOCKET_LINUX_STATE_OPEN)
            {
                LogWarning("Not open, current state is %" PRI_MU_ENUM "", MU_ENUM_VALUE(ASYNC_SOCKET_LINUX_STATE, current_state));
                result = ASYNC_SOCKET_SEND_SYNC_ERROR;
            }
            else
            {
                result = ASYNC_SOCKET_SEND_SYNC_OK;
                ASYNC_SOCKET_SEND_RESULT send_result;
                for (index = 0; index < buffer_count; index++)
                {
                    // Try to send the data with MSG_NOSIGNAL
                    ssize_t send_size = send(async_socket->socket_handle, buffers[index].buffer, buffers[index].length, MSG_NOSIGNAL);
                    if (send_size < 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                        {
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
                                // Modify the socket value in the epoll, if the socket doesn't exist then we
                                // need to add the socket
                                if (epoll_ctl(async_socket->epoll, EPOLL_CTL_MOD, async_socket->socket_handle, &ev) < 0 &&
                                    (errno == ENOENT && epoll_ctl(async_socket->epoll, EPOLL_CTL_ADD, async_socket->socket_handle, &ev) < 0) )
                                {
                                    LogError("failure with epoll_ctrl EPOLL_CTL_MOD error no: %d", errno);
                                    result = ASYNC_SOCKET_SEND_SYNC_ERROR;
                                    send_result = ASYNC_SOCKET_SEND_ERROR;
                                    break;
                                }
                                else
                                {
                                    // Do Nothing and the epoll will get called in the thread
                                }
                            }
                        }
                        else if (errno == ECONNRESET || errno == ENOTCONN || errno == EPIPE)
                        {
                            LogWarning("The connection was forcibly closed by the peer");
                            send_result = ASYNC_SOCKET_SEND_ABANDONED;
                            // Socket was closed
                            result = ASYNC_SOCKET_SEND_SYNC_ABANDONED;
                        }
                        else
                        {
                            LogError("failure sending socket error no: %d", errno);
                            send_result = ASYNC_SOCKET_SEND_ERROR;
                        }
                    }
                    else
                    {
                        send_result = ASYNC_SOCKET_SEND_OK;
                    }
                }
                // Only call the callback if the call was successfully sent
                // Otherwise we're going to be returning an error
                if (send_result == ASYNC_SOCKET_SEND_OK)
                {
                    // The buffer was sent immediatly call the callback
                    on_send_complete(on_send_complete_context, send_result);
                }
            }
            (void)interlocked_decrement(&async_socket->pending_api_calls);
            wake_by_address_single(&async_socket->pending_api_calls);
        }
    }

all_ok:
    return result;
}

int async_socket_receive_async(ASYNC_SOCKET_HANDLE async_socket, ASYNC_SOCKET_BUFFER* payload, uint32_t buffer_count, ON_ASYNC_SOCKET_RECEIVE_COMPLETE on_receive_complete, void* on_receive_complete_context)
{
    int result;

    if (
        (async_socket == NULL) ||
        (payload == NULL) ||
        (buffer_count == 0) ||
        (on_receive_complete == NULL)
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
                (payload[index].buffer == NULL) ||
                (payload[index].length == 0)
                )
            {
                LogError("Invalid buffer %" PRIu32 ": buffer=%p, length = %" PRIu32, index, payload[index].buffer, payload[index].length);
                break;
            }

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
            int32_t dec_value = interlocked_increment(&async_socket->pending_api_calls);

            ASYNC_SOCKET_LINUX_STATE current_state;
            if ((current_state = interlocked_add(&async_socket->state, 0)) != ASYNC_SOCKET_LINUX_STATE_OPEN)
            {
                /* Codes_SRS_ASYNC_SOCKET_WIN32_01_098: [ If async_socket is not OPEN, async_socket_receive_async shall fail and return a non-zero value. ]*/
                LogWarning("Not open, current state is %" PRI_MU_ENUM "", MU_ENUM_VALUE(ASYNC_SOCKET_LINUX_STATE, current_state));
                result = MU_FAILURE;
            }
            else
            {
                ASYNC_SOCKET_RECV_CONTEXT* recv_context = malloc_flex(sizeof(ASYNC_SOCKET_RECV_CONTEXT), buffer_count, sizeof(ASYNC_SOCKET_BUFFER));
                if (recv_context == NULL)
                {
                    LogError("failure in malloc_flex(sizeof(ASYNC_SOCKET_RECV_CONTEXT)=%zu, buffer_count=%" PRIu32 ", sizeof(ASYNC_SOCKET_BUFFER)=%zu) failed",
                        sizeof(ASYNC_SOCKET_RECV_CONTEXT), buffer_count, sizeof(ASYNC_SOCKET_BUFFER));
                    result = ASYNC_SOCKET_SEND_SYNC_ERROR;
                }
                else
                {
                    recv_context->total_buffer_bytes = total_buffer_bytes;
                    recv_context->total_buffer_count = buffer_count;
                    recv_context->on_receive_complete = on_receive_complete;
                    recv_context->on_receive_complete_context = on_receive_complete_context;
                    recv_context->socket_handle = async_socket->socket_handle;
                    recv_context->async_socket = async_socket;

                    for (index = 0; index < buffer_count; index++)
                    {
                        recv_context->recv_buffers[index].buffer = payload[index].buffer;
                        recv_context->recv_buffers[index].length = payload[index].length;
                    }

                    struct epoll_event ev = {0};
                    ev.events = EPOLLIN | EPOLLRDHUP | EPOLLONESHOT;
                    ev.data.ptr = recv_context;
                    // Modify the socket value in the epoll, if the socket doesn't exist then we
                    // need to add the socket
                    if (epoll_ctl(async_socket->epoll, EPOLL_CTL_MOD, async_socket->socket_handle, &ev) < 0 &&
                        (errno == ENOENT && epoll_ctl(async_socket->epoll, EPOLL_CTL_ADD, async_socket->socket_handle, &ev) < 0) )
                    {
                        LogError("failure with epoll_ctrl EPOLL_CTL_MOD error no: %d", errno);
                        result = MU_FAILURE;
                    }
                    else
                    {
                        result = 0;
                        goto all_ok;
                    }
                    free(recv_context);
                }
            }
            (void)interlocked_decrement(&async_socket->pending_api_calls);
            wake_by_address_single(&async_socket->pending_api_calls);
        }
    }

all_ok:
    return result;
}
