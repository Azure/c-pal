// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <errno.h>

#include <sys/epoll.h>
#include <sys/socket.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/log_context.h"
#include "c_logging/log_context_property_type_ascii_char_ptr.h"
#include "c_logging/logger.h"
#include "c_logging/log_errno.h"
#include "c_logging/log_level.h"

#include "c_pal/completion_port_linux.h"
#include "c_pal/execution_engine.h"
#include "c_pal/gballoc_hl.h"        // IWYU pragma: keep
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_pal/platform_linux.h"
#include "c_pal/sync.h"
#include "c_pal/socket_transport.h"
#include "c_pal/socket_handle.h"    // IWYU pragma: keep

#ifdef ENABLE_SOCKET_LOGGING
#include "c_pal/timer.h"
#endif

#include "c_pal/async_socket.h"

#define ASYNC_SOCKET_LINUX_STATE_VALUES \
    ASYNC_SOCKET_LINUX_STATE_CLOSED, \
    ASYNC_SOCKET_LINUX_STATE_OPENING, \
    ASYNC_SOCKET_LINUX_STATE_OPEN, \
    ASYNC_SOCKET_LINUX_STATE_CLOSING

MU_DEFINE_ENUM(ASYNC_SOCKET_LINUX_STATE, ASYNC_SOCKET_LINUX_STATE_VALUES)
MU_DEFINE_ENUM_STRINGS(ASYNC_SOCKET_LINUX_STATE, ASYNC_SOCKET_LINUX_STATE_VALUES)

#define ASYNC_SOCKET_IO_TYPE_VALUES \
    ASYNC_SOCKET_IO_TYPE_SEND, \
    ASYNC_SOCKET_IO_TYPE_RECEIVE, \
    ASYNC_SOCKET_IO_TYPE_NOTIFY

MU_DEFINE_ENUM(ASYNC_SOCKET_IO_TYPE, ASYNC_SOCKET_IO_TYPE_VALUES)
MU_DEFINE_ENUM_STRINGS(ASYNC_SOCKET_IO_TYPE, ASYNC_SOCKET_IO_TYPE_VALUES)

MU_DEFINE_ENUM_STRINGS(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_RESULT_VALUES)
MU_DEFINE_ENUM_STRINGS(ASYNC_SOCKET_OPEN_RESULT, ASYNC_SOCKET_OPEN_RESULT_VALUES)
MU_DEFINE_ENUM_STRINGS(ASYNC_SOCKET_SEND_RESULT, ASYNC_SOCKET_SEND_RESULT_VALUES)
MU_DEFINE_ENUM_STRINGS(ASYNC_SOCKET_NOTIFY_IO_TYPE, ASYNC_SOCKET_NOTIFY_IO_TYPE_VALUES)
MU_DEFINE_ENUM_STRINGS(ASYNC_SOCKET_NOTIFY_IO_RESULT, ASYNC_SOCKET_NOTIFY_IO_RESULT_VALUES)

typedef struct ASYNC_SOCKET_TAG
{
    SOCKET_TRANSPORT_HANDLE socket_transport_handle;
    volatile_atomic int32_t state;
    volatile_atomic int32_t pending_api_calls;
    COMPLETION_PORT_HANDLE completion_port;
    volatile_atomic int32_t added_to_completion_port;
    ON_ASYNC_SOCKET_SEND on_send;
    void* on_send_context;
    ON_ASYNC_SOCKET_RECV on_recv;
    void* on_recv_context;
} ASYNC_SOCKET;

typedef struct ASYNC_SOCKET_RECV_CONTEXT_TAG
{
    uint32_t total_buffer_count;
    ASYNC_SOCKET_BUFFER recv_buffers[];
} ASYNC_SOCKET_RECV_CONTEXT;

typedef struct ASYNC_SOCKET_SEND_CONTEXT_TAG
{
    uint32_t total_buffer_bytes;
    ASYNC_SOCKET_BUFFER socket_buffer;
} ASYNC_SOCKET_SEND_CONTEXT;

typedef struct ASYNC_SOCKET_IO_CONTEXT_TAG
{
    ASYNC_SOCKET_IO_TYPE io_type;
    ON_ASYNC_SOCKET_RECEIVE_COMPLETE on_receive_complete;
    ON_ASYNC_SOCKET_SEND_COMPLETE on_send_complete;
    ON_ASYNC_SOCKET_NOTIFY_IO_COMPLETE on_notify_io_complete;
    void* callback_context;
    ASYNC_SOCKET* async_socket;
    union
    {
        ASYNC_SOCKET_SEND_CONTEXT send_ctx;
        ASYNC_SOCKET_RECV_CONTEXT recv_ctx;
    } data;
} ASYNC_SOCKET_IO_CONTEXT;

static int on_socket_send(void* context, ASYNC_SOCKET_HANDLE async_socket, const void* buf, size_t len)
{
    int result;

    (void)context;

    if (async_socket == NULL)
    {
        LogCritical("Invalid argument on_send void* context, ASYNC_SOCKET_HANDLE async_socket, const void* buf, size_t len");
        result = -1;
    }
    else
    {
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_052: [ on_socket_send shall attempt to send the data by calling socket_transport_send with the MSG_NOSIGNAL flag to ensure SIGPIPE is not generated on errors. ]
        SOCKET_BUFFER input_buf;
        input_buf.buffer = (unsigned char*)buf;

        input_buf.length = len;
        uint32_t bytes_sent;
        if(socket_transport_send(async_socket->socket_transport_handle, &input_buf, 1, &bytes_sent, MSG_NOSIGNAL, NULL) != SOCKET_SEND_OK)
        {
            result = -1;
            LogError("socket_transport_send failed input_buf.buffer: %p, input_buf.length: %" PRIu32 ".", input_buf.buffer, input_buf.length);
        }
        else
        {
            result = bytes_sent;
        }
    }
    return result;
}

static int on_socket_recv(void* context, ASYNC_SOCKET_HANDLE async_socket, void* buf, size_t len)
{
    int result;
    (void)context;

    if (async_socket == NULL)
    {
        result = -1;
        LogCritical("Invalid argument on_recv void* context, ASYNC_SOCKET_HANDLE async_socket, const void* buf, size_t len");
    }
    else
    {
        // Codes_SRS_ASYNC_SOCKET_LINUX_04_007: [ on_socket_recv shall attempt to receive data by calling the socket_transport_receive API. ]
        SOCKET_BUFFER input_buf;
        input_buf.buffer = buf;
        input_buf.length = len;
        uint32_t bytes_recv;

        SOCKET_RECEIVE_RESULT recv_result = socket_transport_receive(async_socket->socket_transport_handle, &input_buf, 1, &bytes_recv, 0, NULL);

        if (recv_result != SOCKET_RECEIVE_OK && recv_result != SOCKET_RECEIVE_WOULD_BLOCK && recv_result != SOCKET_RECEIVE_SHUTDOWN)
        {
            result = -1;
            LogError("socket_transport_receive failed input_buf.buffer: %p, input_buf.length: %" PRIu32 ".", input_buf.buffer, input_buf.length);
        }
        else if (recv_result == SOCKET_RECEIVE_WOULD_BLOCK)
        {
            result = -1;
            LogInfo("Not enough space in send buffer of nonblocking socket. bytes sent: %" PRIu32 " input_buf.buffer: %p, input_buf.length: %" PRIu32 ".", bytes_recv, input_buf.buffer, input_buf.length);

        }
        else if (recv_result == SOCKET_RECEIVE_SHUTDOWN)
        {
            result = 0;
            LogError("Socket received 0 bytes. bytes sent: %" PRIu32 " input_buf.buffer: %p, input_buf.length: %" PRIu32 ".", bytes_recv, input_buf.buffer, input_buf.length);
        }
        else
        {
            result = bytes_recv;

        }
    }

    return result;
}

#ifdef TEST_SUITE_NAME_FROM_CMAKE

ON_ASYNC_SOCKET_SEND get_async_socket_send_callback()
{
    return on_socket_send;
}

ON_ASYNC_SOCKET_RECV get_async_socket_recv_callback()
{
    return on_socket_recv;
}

#endif

static int send_data(ASYNC_SOCKET* async_socket, const ASYNC_SOCKET_BUFFER* buff_data, ssize_t* total_data_sent, int* error_no)
{
    int result;

    ssize_t data_sent = 0;
    do
    {
        // Codes_SRS_ASYNC_SOCKET_LINUX_04_004: [ async_socket_send_async shall call the on_send callback to send the buffer. ]
        ssize_t send_size = async_socket->on_send(async_socket->on_send_context, async_socket, buff_data->buffer+data_sent, buff_data->length-data_sent);
        if (send_size < 0)
        {
            *error_no = errno;
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
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_101: [ If send returns a value > 0 but less than the amount to be sent, event_complete_callback shall continue to send the data until the payload length has been sent. ]
    } while(data_sent < buff_data->length);
    *total_data_sent = data_sent;
    return result;
}

static void event_complete_callback(void* context, COMPLETION_PORT_EPOLL_ACTION action)
{
    // Codes_SRS_ASYNC_SOCKET_LINUX_11_079: [ If context is NULL, event_complete_callback shall do nothing. ]
    if (context == NULL)
    {
        LogCritical("Invalid arguement event_complete_callback void* context, COMPLETION_PORT_EPOLL_EPOLL_ACTION epoll_action, COMPLETION_PORT_EPOLL_EPOLL_RESULT result, int32_t amount_transfered");
    }
    else
    {
        ASYNC_SOCKET_IO_CONTEXT* io_context = (ASYNC_SOCKET_IO_CONTEXT*)context;
        switch (action)
        {
            // Codes_SRS_ASYNC_SOCKET_LINUX_11_080: [ If COMPLETION_PORT_EPOLL_ACTION is COMPLETION_PORT_EPOLL_EPOLLRDHUP or COMPLETION_PORT_EPOLL_ABANDONED, event_complete_callback shall do the following: ]
            case COMPLETION_PORT_EPOLL_ABANDONED:
            case COMPLETION_PORT_EPOLL_EPOLLRDHUP:
            {
                // Codes_SRS_ASYNC_SOCKET_LINUX_11_081: [ event_complete_callback shall call either the send or recv complete callback with an ABANDONED flag when the IO type is either ASYNC_SOCKET_IO_TYPE_SEND or ASYNC_SOCKET_IO_TYPE_RECEIVE respectively. ]
                if (io_context->io_type == ASYNC_SOCKET_IO_TYPE_RECEIVE)
                {
                    LogError("Receive socket has encountered %" PRI_MU_ENUM "", MU_ENUM_VALUE(COMPLETION_PORT_EPOLL_ACTION, action));
                    io_context->on_receive_complete(io_context->callback_context, ASYNC_SOCKET_RECEIVE_ABANDONED, 0);
                }
                else if (io_context->io_type == ASYNC_SOCKET_IO_TYPE_SEND)
                {
                    LogError("Send socket has encountered %" PRI_MU_ENUM "", MU_ENUM_VALUE(COMPLETION_PORT_EPOLL_ACTION, action));
                    io_context->on_send_complete(io_context->callback_context, ASYNC_SOCKET_SEND_ABANDONED);
                }
                else
                {
                    LogError("Notify socket has encountered %" PRI_MU_ENUM "", MU_ENUM_VALUE(COMPLETION_PORT_EPOLL_ACTION, action));
                    // Codes_SRS_ASYNC_SOCKET_LINUX_04_008: [ event_complete_callback shall call the notify complete callback with an ABANDONED flag when the IO type is ASYNC_SOCKET_IO_TYPE_NOTIFY. ]
                    io_context->on_notify_io_complete(io_context->callback_context, ASYNC_SOCKET_NOTIFY_IO_RESULT_ABANDONED);
                }

                // Codes_SRS_ASYNC_SOCKET_LINUX_11_084: [ Then event_complete_callback shall free the context memory. ]
                free(io_context);
                break;
            }
            // Codes_SRS_ASYNC_SOCKET_LINUX_11_082: [ If COMPLETION_PORT_EPOLL_ACTION is COMPLETION_PORT_EPOLL_EPOLLIN, event_complete_callback shall do the following: ]
            case COMPLETION_PORT_EPOLL_EPOLLIN:
            {
                if (io_context->io_type == ASYNC_SOCKET_IO_TYPE_NOTIFY)
                {
                    // Codes_SRS_ASYNC_SOCKET_LINUX_04_009: [ If the IO type is ASYNC_SOCKET_IO_TYPE_NOTIFY then event_complete_callback shall call the notify complete callback with an IN flag. ]
                    io_context->on_notify_io_complete(io_context->callback_context, ASYNC_SOCKET_NOTIFY_IO_RESULT_IN);
                }
                else
                {
                    ASYNC_SOCKET_RECEIVE_RESULT receive_result;
                    uint32_t index = 0;
                    uint32_t total_recv_size = 0;

                    do
                    {
                        // Codes_SRS_ASYNC_SOCKET_LINUX_11_083: [ Otherwise event_complete_callback shall call the on_recv callback with the recv_buffer buffer and length and do the following: ]
                        ssize_t recv_size = io_context->async_socket->on_recv(io_context->async_socket->on_recv_context, io_context->async_socket, io_context->data.recv_ctx.recv_buffers[index].buffer, io_context->data.recv_ctx.recv_buffers[index].length);
                        // Codes_SRS_ASYNC_SOCKET_LINUX_11_088: [ If the recv size < 0, then: ]
                        if (recv_size < 0)
                        {
                            if (errno == EAGAIN || errno == EWOULDBLOCK)
                            {
                                // Codes_SRS_ASYNC_SOCKET_LINUX_11_089: [ If errno is EAGAIN or EWOULDBLOCK, then no data is available and event_complete_callback will break out of the function. ]
                                receive_result = ASYNC_SOCKET_RECEIVE_OK;
                                break;
                            }
                            else
                            {
                                total_recv_size = 0;
                                recv_size = 0;
                                if (errno == ECONNRESET)
                                {
                                    // Codes_SRS_ASYNC_SOCKET_LINUX_11_090: [ If errno is ECONNRESET, then thread_worker_func shall call the on_receive_complete callback with the on_receive_complete_context and ASYNC_SOCKET_RECEIVE_ABANDONED. ]
                                    receive_result = ASYNC_SOCKET_RECEIVE_ABANDONED;
                                    LogError("A reset on the recv socket has been encountered");
                                }
                                else
                                {
                                    // Codes_SRS_ASYNC_SOCKET_LINUX_11_095: [ If errno is any other error, then event_complete_callback shall call the on_receive_complete callback with the on_receive_complete_context and ASYNC_SOCKET_RECEIVE_ERROR. ]
                                    receive_result = ASYNC_SOCKET_RECEIVE_ERROR;
                                    LogErrorNo("failure recv data");
                                }
                                break;
                            }
                        }
                        else if (recv_size == 0)
                        {
                            // Codes_SRS_ASYNC_SOCKET_LINUX_11_091: [ If the socket_transport_receive size equals 0, then event_complete_callback shall call on_receive_complete callback with the on_receive_complete_context and ASYNC_SOCKET_RECEIVE_ABANDONED. ]
                            LogError("Socket received 0 bytes, assuming socket is closed");
                            receive_result = ASYNC_SOCKET_RECEIVE_ABANDONED;
                            break;
                        }
                        else
                        {
                            if (recv_size > UINT32_MAX ||
                                UINT32_MAX - total_recv_size < recv_size)
                            {
                                // Handle unlikely overflow
                                LogError("Overflow in computing receive size (total_recv_size=%" PRIu32 " + recv_size=%zi > UINT32_MAX=%" PRIu32 ")",
                                    total_recv_size, recv_size, UINT32_MAX);
                                receive_result = ASYNC_SOCKET_RECEIVE_ERROR;
                            }
                            else
                            {
                                // Codes_SRS_ASYNC_SOCKET_LINUX_11_092: [ If the socket_transport_receive size > 0, if we have another buffer to fill then we will attempt another read, otherwise we shall call on_receive_complete callback with the on_receive_complete_context and ASYNC_SOCKET_RECEIVE_OK ]
                                total_recv_size += recv_size;
                                if (index + 1 >= io_context->data.recv_ctx.total_buffer_count || recv_size <= io_context->data.recv_ctx.recv_buffers[index].length)
                                {
#ifdef ENABLE_SOCKET_LOGGING
                                    LogVerbose("Asynchronous receive of %" PRIu32 " bytes completed at %lf", bytes_received, timer_global_get_elapsed_us());
#endif
                                    receive_result = ASYNC_SOCKET_RECEIVE_OK;
                                    break;
                                }
                                else
                                {
                                    index++;
                                }
                            }
                        }
                    } while (true);

                    // Call the callback
                    io_context->on_receive_complete(io_context->callback_context, receive_result, total_recv_size);
                }

                // Codes_SRS_ASYNC_SOCKET_LINUX_11_093: [ event_complete_callback shall then free the io_context memory. ]
                free(io_context);
                break;
            }
            // Codes_SRS_ASYNC_SOCKET_LINUX_11_094: [ If the events value contains COMPLETION_PORT_EPOLL_EPOLLOUT, event_complete_callback shall the following: ]
            case COMPLETION_PORT_EPOLL_EPOLLOUT:
            {
                if (io_context->io_type == ASYNC_SOCKET_IO_TYPE_NOTIFY)
                {
                    // Codes_SRS_ASYNC_SOCKET_LINUX_04_010: [ If the IO type is ASYNC_SOCKET_IO_TYPE_NOTIFY then event_complete_callback shall call the notify complete callback with an OUT flag. ]
                    io_context->on_notify_io_complete(io_context->callback_context, ASYNC_SOCKET_NOTIFY_IO_RESULT_OUT);
                }
                else
                {
                    ASYNC_SOCKET_SEND_RESULT send_result;

                    int error_no;
                    ssize_t total_data_sent;
                    // Codes_SRS_ASYNC_SOCKET_LINUX_11_096: [ event_complete_callback shall call send on the data in the ASYNC_SOCKET_SEND_CONTEXT buffer. ]
                    if (send_data(io_context->async_socket, &io_context->data.send_ctx.socket_buffer, &total_data_sent, &error_no) != 0)
                    {
                        // Codes_SRS_ASYNC_SOCKET_LINUX_11_097: [ If socket_transport_send returns value is < 0 event_complete_callback shall do the following: ]
                        if (error_no == ECONNRESET)
                        {
                            // Codes_SRS_ASYNC_SOCKET_LINUX_11_098: [ if errno is ECONNRESET, then on_send_complete shall be called with ASYNC_SOCKET_SEND_ABANDONED. ]
                            send_result = ASYNC_SOCKET_SEND_ABANDONED;
                            LogError("A reset on the send socket has been encountered");
                        }
                        else
                        {
                            // Codes_SRS_ASYNC_SOCKET_LINUX_11_099: [ if errno is anything else, then on_send_complete shall be called with ASYNC_SOCKET_SEND_ERROR. ]
                            send_result = ASYNC_SOCKET_SEND_ERROR;
                            LogErrorNo("failure sending data length: %" PRIu32 "", io_context->data.send_ctx.socket_buffer.length);
                        }
                    }
                    else
                    {
#ifdef ENABLE_SOCKET_LOGGING
                        LogVerbose("Asynchronous send of %" PRIu32 " bytes completed at %lf", bytes_sent, timer_global_get_elapsed_us());
#endif
                        send_result = ASYNC_SOCKET_SEND_OK;
                    }

                    io_context->on_send_complete(io_context->callback_context, send_result);
                }

                // Codes_SRS_ASYNC_SOCKET_LINUX_11_100: [ Then event_complete_callback shall free the io_context memory ]
                free(io_context);
                break;
            }
            // Codes_SRS_ASYNC_SOCKET_LINUX_11_085: [ If the events value contains COMPLETION_PORT_EPOLL_ERROR, event_complete_callback shall the following: ]
            case COMPLETION_PORT_EPOLL_ERROR:
            default:
            {
                // Codes_SRS_ASYNC_SOCKET_LINUX_11_086: [ Otherwise event_complete_callback shall call either the send or recv complete callback with an ERROR flag. ]
                if (io_context->io_type == ASYNC_SOCKET_IO_TYPE_RECEIVE)
                {
                    LogError("Receive socket has encountered COMPLETION_PORT_EPOLL_ERROR");
                    io_context->on_receive_complete(io_context->callback_context, ASYNC_SOCKET_RECEIVE_ERROR, 0);
                }
                else if (io_context->io_type == ASYNC_SOCKET_IO_TYPE_SEND)
                {
                    LogError("Send socket has encountered COMPLETION_PORT_EPOLL_ERROR");
                    io_context->on_send_complete(io_context->callback_context, ASYNC_SOCKET_SEND_ERROR);
                }
                else
                {
                    LogError("Notify socket has encountered COMPLETION_PORT_EPOLL_ERROR");
                    // Codes_SRS_ASYNC_SOCKET_LINUX_04_011: [ If the IO type is ASYNC_SOCKET_IO_TYPE_NOTIFY then event_complete_callback shall call the notify complete callback with an ERROR flag. ]
                    io_context->on_notify_io_complete(io_context->callback_context, ASYNC_SOCKET_NOTIFY_IO_RESULT_ERROR);
                }
                // Codes_SRS_ASYNC_SOCKET_LINUX_11_087: [ Then event_complete_callback shall and free the io_context memory. ]
                free(io_context);
                break;
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

    if (interlocked_add(&async_socket->added_to_completion_port, 0) > 0)
    {
        completion_port_remove(async_socket->completion_port, socket_transport_get_underlying_socket(async_socket->socket_transport_handle));
    }

    // Codes_SRS_ASYNC_SOCKET_LINUX_11_041: [ async_socket_close shall set the state to closed. ]
    (void)interlocked_exchange(&async_socket->state, ASYNC_SOCKET_LINUX_STATE_CLOSED);
    wake_by_address_single(&async_socket->state);
}

ASYNC_SOCKET_HANDLE async_socket_create(EXECUTION_ENGINE_HANDLE execution_engine)
{
    // Codes_SRS_ASYNC_SOCKET_LINUX_04_001: [ async_socket_create shall delegate to async_socket_create_with_transport passing in callbacks for on_send and on_recv that implement socket read and write by calling send and recv respectively from system socket API. ]
    return async_socket_create_with_transport(execution_engine, on_socket_send, NULL, on_socket_recv, NULL);
}

ASYNC_SOCKET_HANDLE async_socket_create_with_transport(EXECUTION_ENGINE_HANDLE execution_engine, ON_ASYNC_SOCKET_SEND on_send, void* on_send_context, ON_ASYNC_SOCKET_RECV on_recv, void* on_recv_context)
{
    ASYNC_SOCKET_HANDLE result;
    // Codes_SRS_ASYNC_SOCKET_LINUX_11_002: [ execution_engine shall be allowed to be NULL. ]

    // Codes_SRS_ASYNC_SOCKET_LINUX_04_002: [ If on_send is NULL , async_socket_create_with_transport shall fail and return NULL. ]
    // Codes_SRS_ASYNC_SOCKET_LINUX_04_003: [ If on_recv is NULL , async_socket_create_with_transport shall fail and return NULL. ]
    if (
        on_send == NULL ||
        on_recv == NULL)
    {
        LogError("EXECUTION_ENGINE_HANDLE execution_engine:%p, ON_ASYNC_SOCKET_SEND on_send: %p, void* on_send_context: %p, ON_ASYNC_SOCKET_RECV on_recv: %p, void* on_recv_context: %p", execution_engine, on_send, on_send_context, on_recv, on_recv_context);
    }
    else
    {
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_001: [ async_socket_create_with_transport shall allocate a new async socket and on success shall return a non-NULL handle. ]
        result = malloc(sizeof(ASYNC_SOCKET));
        if (result == NULL)
        {
            LogError("failure allocating async socket %zu", sizeof(ASYNC_SOCKET));
        }
        else
        {
            // Codes_SRS_ASYNC_SOCKET_LINUX_11_005: [ async_socket_create_with_transport shall retrieve an COMPLETION_PORT_HANDLE object by calling platform_get_completion_port. ]
            if ((result->completion_port = platform_get_completion_port()) == NULL)
            {
                LogError("failure platform_get_completion_port");
            }
            else
            {
                result->on_send = on_send;
                result->on_send_context = on_send_context;
                result->on_recv = on_recv;
                result->on_recv_context = on_recv_context;

                (void)interlocked_exchange(&result->pending_api_calls, 0);
                (void)interlocked_exchange(&result->added_to_completion_port, 0);
                (void)interlocked_exchange(&result->state, ASYNC_SOCKET_LINUX_STATE_CLOSED);
                goto all_ok;
            }
            free(result);
        }
    }
    // Codes_SRS_ASYNC_SOCKET_LINUX_11_006: [ If any error occurs, async_socket_create_with_transport shall fail and return NULL. ]
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
            // Codes_SRS_ASYNC_SOCKET_LINUX_11_020: [ While async_socket is OPENING or CLOSING, async_socket_destroy shall wait for the open/close to complete either successfully or with error. ]
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

        // Codes_SRS_ASYNC_SOCKET_LINUX_11_022: [ async_socket_destroy shall decrement the reference count on the completion port. ]
        completion_port_dec_ref(async_socket->completion_port);
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_023: [ async_socket_destroy shall free all resources associated with async_socket. ]
        free(async_socket);
    }
}

int async_socket_open_async(ASYNC_SOCKET_HANDLE async_socket, SOCKET_TRANSPORT_HANDLE socket_transport_handle, ON_ASYNC_SOCKET_OPEN_COMPLETE on_open_complete, void* on_open_complete_context)
{
    int result;
    // Codes_SRS_ASYNC_SOCKET_LINUX_11_026: [ on_open_complete_context shall be allowed to be NULL. ]
    if (
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_024: [ If async_socket is NULL, async_socket_open_async shall fail and return a non-zero value. ]
        async_socket == NULL ||
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_025: [ If on_open_complete is NULL, async_socket_open_async shall fail and return a non-zero value. ]
        on_open_complete == NULL ||
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_003: [ If socket_transport_handle is NULL, async_socket_open_async shall fail and return a non-zero value. ]
        socket_transport_handle == NULL
       )
    {
        LogError("ASYNC_SOCKET_HANDLE async_socket=%p, ON_ASYNC_SOCKET_OPEN_COMPLETE on_open_complete=%p, void* on_open_complete_context=%p",
            async_socket, on_open_complete, on_open_complete_context);
        result = MU_FAILURE;
    }
    else
    {
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_027: [ Otherwise, async_socket_open_async shall switch the state to OPENING. ]
        int32_t current_state = interlocked_compare_exchange(&async_socket->state, ASYNC_SOCKET_LINUX_STATE_OPENING, ASYNC_SOCKET_LINUX_STATE_CLOSED);
        // Codes_SRS_ASYNC_SOCKET_LINUX_11_029: [ If async_socket is already OPEN or OPENING, async_socket_open_async shall fail and return a non-zero value. ]
        if (current_state != ASYNC_SOCKET_LINUX_STATE_CLOSED)
        {
            // Codes_SRS_ASYNC_SOCKET_LINUX_11_034: [ If any error occurs, async_socket_open_async shall fail and return a non-zero value. ]
            LogError("Open called in state %" PRI_MU_ENUM "", MU_ENUM_VALUE(ASYNC_SOCKET_LINUX_STATE, current_state));
            result = MU_FAILURE;
        }
        else
        {
            async_socket->socket_transport_handle = socket_transport_handle;

            // Codes_SRS_ASYNC_SOCKET_LINUX_11_031: [ async_socket_open_async shall add the socket to the epoll system by calling epoll_ctl with EPOLL_CTL_ADD. ]
            // Codes_SRS_ASYNC_SOCKET_LINUX_11_032: [ async_socket_open_async shall set the state to OPEN. ]
            (void)interlocked_exchange(&async_socket->state, ASYNC_SOCKET_LINUX_STATE_OPEN);
            // Codes_SRS_ASYNC_SOCKET_LINUX_11_033: [ On success async_socket_open_async shall call on_open_complete_context with ASYNC_SOCKET_OPEN_OK. ]
            on_open_complete(on_open_complete_context, ASYNC_SOCKET_OPEN_OK);

            // Codes_SRS_ASYNC_SOCKET_LINUX_11_028: [ On success, async_socket_open_async shall return 0. ]
            result = 0;
            goto all_ok;

            (void)interlocked_exchange(&async_socket->state, ASYNC_SOCKET_LINUX_STATE_CLOSED);
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
            // Codes_SRS_ASYNC_SOCKET_LINUX_11_063: [ If any error occurs, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]
            LogError("Invalid buffers passed to async_socket_send_async");
            result = ASYNC_SOCKET_SEND_SYNC_ERROR;
        }
        else
        {
            (void)interlocked_increment(&async_socket->pending_api_calls);

            ASYNC_SOCKET_LINUX_STATE current_state;
            // Codes_SRS_ASYNC_SOCKET_LINUX_11_051: [ If async_socket is not OPEN, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_NOT_OPEN. ]
            if ((current_state = interlocked_add(&async_socket->state, 0)) != ASYNC_SOCKET_LINUX_STATE_OPEN)
            {
                LogWarning("Not open, current state is %" PRI_MU_ENUM "", MU_ENUM_VALUE(ASYNC_SOCKET_LINUX_STATE, current_state));
                result = ASYNC_SOCKET_SEND_SYNC_NOT_OPEN;
            }
            else
            {
#ifdef ENABLE_SOCKET_LOGGING
                LogVerbose("Starting send of %" PRIu32 " bytes at %lf", total_buffer_bytes, timer_global_get_elapsed_us());
#endif

                ASYNC_SOCKET_SEND_RESULT send_result;
                for (index = 0; index < buffer_count; index++)
                {
                    int error_no;
                    ssize_t total_data_sent;
                    // Codes_SRS_ASYNC_SOCKET_LINUX_11_054: [ If socket_transport_send fails to send the data, async_socket_send_async shall do the following: ]
                    if (send_data(async_socket, &buffers[index], &total_data_sent, &error_no) != 0)
                    {
                        // Codes_SRS_ASYNC_SOCKET_LINUX_11_055: [ If the errno value is EAGAIN or EWOULDBLOCK. ]
                        if (error_no == EAGAIN || error_no == EWOULDBLOCK)
                        {
                            // Codes_SRS_ASYNC_SOCKET_LINUX_11_056: [ async_socket_send_async shall create a context for the send where the payload, on_send_complete and on_send_complete_context shall be stored. ]
                            ASYNC_SOCKET_IO_CONTEXT* io_context = malloc(sizeof(ASYNC_SOCKET_IO_CONTEXT));
                            if (io_context == NULL)
                            {
                                LogError("malloc(sizeof(ASYNC_SOCKET_SEND_CONTEXT)=%zu failed", sizeof(ASYNC_SOCKET_IO_CONTEXT));
                                result = ASYNC_SOCKET_SEND_SYNC_ERROR;
                                send_result = ASYNC_SOCKET_SEND_ABANDONED;
                                break;
                            }
                            else
                            {
                                send_result = ASYNC_SOCKET_SEND_ERROR;
                                io_context->io_type = ASYNC_SOCKET_IO_TYPE_SEND;
                                io_context->data.send_ctx.total_buffer_bytes = total_buffer_bytes - total_data_sent;
                                io_context->on_send_complete = on_send_complete;
                                io_context->callback_context = on_send_complete_context;
                                io_context->async_socket = async_socket;

                                io_context->data.send_ctx.socket_buffer.buffer = buffers[index].buffer + total_data_sent;
                                io_context->data.send_ctx.socket_buffer.length = buffers[index].length - total_data_sent;

                                // Codes_SRS_ASYNC_SOCKET_LINUX_11_057: [ The context shall then be added to the completion port system by calling completion_port_add with EPOLL_CTL_MOD and `event_complete_callback` as the callback. ]
                                if (completion_port_add(async_socket->completion_port, EPOLLOUT, socket_transport_get_underlying_socket(async_socket->socket_transport_handle), event_complete_callback, io_context) != 0)
                                {
                                    LogError("failure with completion_port_add");
                                    result = ASYNC_SOCKET_SEND_SYNC_ERROR;
                                }
                                else
                                {
                                    (void)interlocked_increment(&async_socket->added_to_completion_port);
                                    result = ASYNC_SOCKET_SEND_SYNC_OK;
                                    goto all_ok;
                                }
                            }
                        }
                        // Codes_SRS_ASYNC_SOCKET_LINUX_11_059: [ If the errno value is ECONNRESET, ENOTCONN, or EPIPE shall fail and return ASYNC_SOCKET_SEND_SYNC_NOT_OPEN. ]
                        else if (error_no == ECONNRESET || error_no == ENOTCONN || error_no == EPIPE)
                        {
                            LOGGER_LOG_EX(LOG_LEVEL_WARNING, LOG_ERRNO(), LOG_MESSAGE("The connection was forcibly closed by the peer"));
                            send_result = ASYNC_SOCKET_SEND_ABANDONED;
                            // Socket was closed
                            result = ASYNC_SOCKET_SEND_SYNC_NOT_OPEN;
                        }
                        else
                        {
                            // Codes_SRS_ASYNC_SOCKET_LINUX_11_060: [ If any other error is encountered, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]
                            LogErrorNo("failure sending socket error no");
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
#ifdef ENABLE_SOCKET_LOGGING
                    LogVerbose("Send completed synchronously at %lf", timer_global_get_elapsed_us());
#endif
                    // Codes_SRS_ASYNC_SOCKET_LINUX_11_061: [ If the send is successful, async_socket_send_async shall call the on_send_complete with on_send_complete_context and ASYNC_SOCKET_SEND_SYNC_OK. ]
                    on_send_complete(on_send_complete_context, send_result);
                }
                else
                {
                    // Do nothing.  If failure happend do not call the callback
                }
            }
all_ok:
            if (interlocked_decrement(&async_socket->pending_api_calls) == 0)
            {
                wake_by_address_single(&async_socket->pending_api_calls);
            }
        }
    }

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
            (void)interlocked_increment(&async_socket->pending_api_calls);

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
                // Codes_SRS_ASYNC_SOCKET_LINUX_11_074: [ The context shall also allocate enough memory to keep an array of buffer_count items. ]
                ASYNC_SOCKET_IO_CONTEXT* io_context = malloc_flex(sizeof(ASYNC_SOCKET_IO_CONTEXT), buffer_count, sizeof(ASYNC_SOCKET_BUFFER));
                if (io_context == NULL)
                {
                    LogError("failure in malloc_flex(sizeof(ASYNC_SOCKET_RECV_CONTEXT)=%zu, buffer_count=%" PRIu32 ", sizeof(ASYNC_SOCKET_BUFFER)=%zu) failed",
                        sizeof(ASYNC_SOCKET_IO_CONTEXT), buffer_count, sizeof(ASYNC_SOCKET_BUFFER));
                    result = ASYNC_SOCKET_SEND_SYNC_ERROR;
                }
                else
                {
                    // Codes_SRS_ASYNC_SOCKET_LINUX_11_073: [ Otherwise async_socket_receive_async shall create a context for the recv where the payload, on_receive_complete and on_receive_complete_context shall be stored. ]
                    io_context->io_type = ASYNC_SOCKET_IO_TYPE_RECEIVE;
                    io_context->on_receive_complete = on_receive_complete;
                    io_context->callback_context = on_receive_complete_context;
                    io_context->async_socket = async_socket;
                    io_context->data.recv_ctx.total_buffer_count = buffer_count;

                    for (index = 0; index < buffer_count; index++)
                    {
                        io_context->data.recv_ctx.recv_buffers[index].buffer = payload[index].buffer;
                        io_context->data.recv_ctx.recv_buffers[index].length = payload[index].length;
                    }

#ifdef ENABLE_SOCKET_LOGGING
                    LogVerbose("Starting receive at %lf", timer_global_get_elapsed_us());
#endif

                    // Codes_SRS_ASYNC_SOCKET_LINUX_11_102: [ Then the context shall then be added to the completion port system by calling completion_port_add with EPOLLIN and event_complete_callback as the callback. ]
                    if (completion_port_add(async_socket->completion_port, EPOLLIN | EPOLLRDHUP | EPOLLONESHOT, socket_transport_get_underlying_socket(async_socket->socket_transport_handle), event_complete_callback, io_context) != 0)
                    {
                        // Codes_SRS_ASYNC_SOCKET_LINUX_11_078: [ If any error occurs, async_socket_receive_async shall fail and return a non-zero value. ]
                        LogWarning("failure with completion_port_add");
                        result = MU_FAILURE;
                    }
                    else
                    {
                        // Codes_SRS_ASYNC_SOCKET_LINUX_11_077: [ On success, async_socket_receive_async shall return 0. ]
                        result = 0;
                        (void)interlocked_increment(&async_socket->added_to_completion_port);
                        goto all_ok;
                    }
                    free(io_context);
                }
            }
all_ok:
            if (interlocked_decrement(&async_socket->pending_api_calls) == 0)
            {
                wake_by_address_single(&async_socket->pending_api_calls);
            }
        }
    }
    return result;
}

int async_socket_notify_io_async(ASYNC_SOCKET_HANDLE async_socket, ASYNC_SOCKET_NOTIFY_IO_TYPE io_type, ON_ASYNC_SOCKET_NOTIFY_IO_COMPLETE on_notify_io_complete, void* on_notify_io_complete_context)
{
    int result;

    // Codes_SRS_ASYNC_SOCKET_LINUX_04_012: [ If async_socket is NULL, async_socket_notify_io_async shall fail and return a non-zero value. ]
    // Codes_SRS_ASYNC_SOCKET_LINUX_04_013: [ If on_notify_io_complete is NULL, async_socket_notify_io_async shall fail and return a non-zero value. ]
    // Codes_SRS_ASYNC_SOCKET_LINUX_04_014: [ If io_type has an invalid value, then async_socket_notify_io_async shall fail and return a non-zero value. ]
    // Codes_SRS_ASYNC_SOCKET_LINUX_04_016: [ on_notify_io_complete_context is allowed to be NULL. ]
    if (async_socket == NULL || on_notify_io_complete == NULL || (io_type != ASYNC_SOCKET_NOTIFY_IO_TYPE_IN && io_type != ASYNC_SOCKET_NOTIFY_IO_TYPE_OUT))
    {
        LogError(
            "Invalid arguments: ASYNC_SOCKET_HANDLE async_socket=%p, ASYNC_SOCKET_NOTIFY_IO_TYPE io_type=%" PRI_MU_ENUM
            ", ON_ASYNC_SOCKET_NOTIFY_IO_COMPLETE on_io_complete=%p, void* on_io_complete_context=%p",
            async_socket, MU_ENUM_VALUE(ASYNC_SOCKET_NOTIFY_IO_TYPE, io_type), on_notify_io_complete, on_notify_io_complete_context);
        result = MU_FAILURE;
    }
    else
    {
        (void)interlocked_increment(&async_socket->pending_api_calls);

        // Codes_SRS_ASYNC_SOCKET_LINUX_04_015: [ If the async socket's current state is not ASYNC_SOCKET_LINUX_STATE_OPEN then async_socket_notify_io_async shall fail and return a non-zero value. ]
        ASYNC_SOCKET_LINUX_STATE current_state;
        if ((current_state = interlocked_add(&async_socket->state, 0)) != ASYNC_SOCKET_LINUX_STATE_OPEN)
        {
            LogWarning("Not open, current state is %" PRI_MU_ENUM "", MU_ENUM_VALUE(ASYNC_SOCKET_LINUX_STATE, current_state));
            result = MU_FAILURE;
        }
        else
        {
            // Codes_SRS_ASYNC_SOCKET_LINUX_04_017: [ Otherwise async_socket_notify_io_async shall create a context for the notify where the on_notify_io_complete and on_notify_io_complete_context shall be stored. ]
            ASYNC_SOCKET_IO_CONTEXT* io_context = malloc(sizeof(ASYNC_SOCKET_IO_CONTEXT));
            if (io_context == NULL)
            {
                // Codes_SRS_ASYNC_SOCKET_LINUX_04_020: [ If any error occurs, async_socket_notify_io_async shall fail and return a non-zero value. ]
                LogError("failure in malloc(sizeof(ASYNC_SOCKET_IO_CONTEXT)=%zu) failed", sizeof(ASYNC_SOCKET_IO_CONTEXT));
                result = MU_FAILURE;
            }
            else
            {
                io_context->io_type = ASYNC_SOCKET_IO_TYPE_NOTIFY;
                io_context->on_notify_io_complete = on_notify_io_complete;
                io_context->callback_context = on_notify_io_complete_context;
                io_context->async_socket = async_socket;

                int epoll_op = (io_type == ASYNC_SOCKET_NOTIFY_IO_TYPE_IN) ? EPOLLIN : EPOLLOUT;

                // Codes_SRS_ASYNC_SOCKET_LINUX_04_018: [ Then the context shall then be added to the completion port system by calling completion_port_add with EPOLLIN if io_type is ASYNC_SOCKET_NOTIFY_IO_TYPE_IN and EPOLLOUT otherwise and event_complete_callback as the callback. ]
                if (completion_port_add(async_socket->completion_port, epoll_op, socket_transport_get_underlying_socket(async_socket->socket_transport_handle), event_complete_callback, io_context) != 0)
                {
                    // Codes_SRS_ASYNC_SOCKET_LINUX_04_020: [ If any error occurs, async_socket_notify_io_async shall fail and return a non-zero value. ]
                    LogWarning("failure with completion_port_add");
                    result = MU_FAILURE;
                }
                else
                {
                    // Codes_SRS_ASYNC_SOCKET_LINUX_04_019: [ On success, async_socket_notify_io_async shall return 0. ]
                    result = 0;
                    (void)interlocked_increment(&async_socket->added_to_completion_port);
                    goto all_ok;
                }

                free(io_context);
            }
        }
all_ok:
        if (interlocked_decrement(&async_socket->pending_api_calls) == 0)
        {
            wake_by_address_single(&async_socket->pending_api_calls);
        }
    }

    return result;
}
