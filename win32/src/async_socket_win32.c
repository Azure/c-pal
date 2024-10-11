// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>
#include <inttypes.h>

#include "winsock2.h"
#include "ws2tcpip.h"
#include "windows.h"

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/execution_engine.h"
#include "c_pal/execution_engine_win32.h"
#include "c_pal/timer.h"
#include "c_pal/socket_transport.h"
#include "c_pal/sm.h"

#include "c_pal/async_socket.h"

#define ASYNC_SOCKET_WIN32_STATE_VALUES \
    ASYNC_SOCKET_WIN32_STATE_CLOSED, \
    ASYNC_SOCKET_WIN32_STATE_OPENING, \
    ASYNC_SOCKET_WIN32_STATE_OPEN, \
    ASYNC_SOCKET_WIN32_STATE_CLOSING

MU_DEFINE_ENUM(ASYNC_SOCKET_WIN32_STATE, ASYNC_SOCKET_WIN32_STATE_VALUES)
MU_DEFINE_ENUM_STRINGS(ASYNC_SOCKET_WIN32_STATE, ASYNC_SOCKET_WIN32_STATE_VALUES)

#define ASYNC_SOCKET_IO_TYPE_VALUES \
    ASYNC_SOCKET_IO_TYPE_SEND, \
    ASYNC_SOCKET_IO_TYPE_RECEIVE

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
    EXECUTION_ENGINE_HANDLE execution_engine;
    volatile LONG state;
    PTP_POOL pool;
    TP_CALLBACK_ENVIRON tp_environment;
    PTP_IO tp_io;
    volatile LONG pending_api_calls;
} ASYNC_SOCKET;

// send context
typedef struct ASYNC_SOCKET_SEND_CONTEXT_TAG
{
    ON_ASYNC_SOCKET_SEND_COMPLETE on_send_complete;
    void* on_send_complete_context;
} ASYNC_SOCKET_SEND_CONTEXT;

// receive context
typedef struct ASYNC_SOCKET_RECEIVE_CONTEXT_TAG
{
    ON_ASYNC_SOCKET_RECEIVE_COMPLETE on_receive_complete;
    void* on_receive_complete_context;
} ASYNC_SOCKET_RECEIVE_CONTEXT;

typedef union ASYNC_SOCKET_IO_CONTEXT_UNION_TAG
{
    ASYNC_SOCKET_SEND_CONTEXT send;
    ASYNC_SOCKET_RECEIVE_CONTEXT receive;
} ASYNC_SOCKET_IO_CONTEXT_UNION;

typedef struct ASYNC_SOCKET_IO_CONTEXT_TAG
{
    OVERLAPPED overlapped;
    ASYNC_SOCKET_IO_TYPE io_type;
    uint32_t total_buffer_bytes;
    ASYNC_SOCKET_IO_CONTEXT_UNION io;
    SOCKET_BUFFER wsa_buffers[];
} ASYNC_SOCKET_IO_CONTEXT;

static VOID WINAPI on_io_complete(PTP_CALLBACK_INSTANCE instance, PVOID context, PVOID overlapped, ULONG io_result, ULONG_PTR number_of_bytes_transferred, PTP_IO io)
{
    if (overlapped == NULL)
    {
        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_063: [ If overlapped is NULL, on_io_complete shall return. ]*/
        LogError("Invalid arguments: PTP_CALLBACK_INSTANCE instance=%p, PVOID context=%p, PVOID overlapped=%p, ULONG io_result=%lu, ULONG_PTR number_of_bytes_transferred=%p, PTP_IO io=%p",
            instance, context, overlapped, io_result, (void*)number_of_bytes_transferred, io);
    }
    else
    {
        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_064: [ overlapped shall be used to determine the context of the IO. ]*/
        ASYNC_SOCKET_IO_CONTEXT* io_context = (ASYNC_SOCKET_IO_CONTEXT*)(((unsigned char*)overlapped) - offsetof(ASYNC_SOCKET_IO_CONTEXT, overlapped));
        switch (io_context->io_type)
        {
            default:
                LogError("Unknown IO type: %" PRI_MU_ENUM "", MU_ENUM_VALUE(ASYNC_SOCKET_IO_TYPE, io_context->io_type));
                break;

                /* Codes_SRS_ASYNC_SOCKET_WIN32_01_065: [ If the context of the IO indicates that a send has completed: ]*/
            case ASYNC_SOCKET_IO_TYPE_SEND:
            {
                ASYNC_SOCKET_SEND_RESULT send_result;

                if (io_result == NO_ERROR)
                {
                    uint32_t bytes_sent = (uint32_t)number_of_bytes_transferred;

#ifdef ENABLE_SOCKET_LOGGING
                    LogVerbose("Asynchronous send of %" PRIu32 " bytes completed at %lf", bytes_sent, timer_global_get_elapsed_us());
#endif

                    if (bytes_sent != io_context->total_buffer_bytes)
                    {
                        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_102: [ If io_result is NO_ERROR, but the number of bytes send is different than the sum of all buffer sizes passed to async_socket_send_async, the on_send_complete callback passed to async_socket_send_async shall be called with on_send_complete_context as context and ASYNC_SOCKET_SEND_ERROR. ]*/
                        send_result = ASYNC_SOCKET_SEND_ERROR;
                    }
                    else
                    {
                        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_066: [ If io_result is NO_ERROR, the on_send_complete callback passed to async_socket_send_async shall be called with on_send_complete_context as argument and ASYNC_SOCKET_SEND_OK. ]*/
                        send_result = ASYNC_SOCKET_SEND_OK;
                    }
                }
                else
                {
                    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_067: [ If io_result is not NO_ERROR, the on_send_complete callback passed to async_socket_send_async shall be called with on_send_complete_context as argument and ASYNC_SOCKET_SEND_ERROR. ]*/
                    LogError("Send IO completed with error %lu", io_result);
                    send_result = ASYNC_SOCKET_SEND_ERROR;
                }

                io_context->io.send.on_send_complete(io_context->io.send.on_send_complete_context, send_result);

                break;
            }
            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_071: [ If the context of the IO indicates that a receive has completed: ]*/
            case ASYNC_SOCKET_IO_TYPE_RECEIVE:
            {
                ASYNC_SOCKET_RECEIVE_RESULT receive_result;
                uint32_t bytes_received;

                switch (io_result)
                {
                    default:
                    {
                        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_070: [ If io_result is not NO_ERROR, the on_receive_complete callback passed to async_socket_receive_async shall be called with on_receive_complete_context as context, ASYNC_SOCKET_RECEIVE_ERROR as result and 0 for bytes_received. ]*/
                        LogError("Receive IO completed with error %lu", io_result);
                        receive_result = ASYNC_SOCKET_RECEIVE_ERROR;
                        bytes_received = 0;
                        break;
                    }
                    case ERROR_NETNAME_DELETED:
                    case ERROR_CONNECTION_ABORTED:
                    case ERROR_OPERATION_ABORTED:
                    {
                        /* Codes_SRS_ASYNC_SOCKET_WIN32_42_001: [ If io_result is ERROR_NETNAME_DELETED or ERROR_CONNECTION_ABORTED, the on_receive_complete callback passed to async_socket_receive_async shall be called with on_receive_complete_context as context, ASYNC_SOCKET_RECEIVE_ABANDONED as result and 0 for bytes_received. ]*/
                        LogError("Receive IO completed with error %lu (socket seems to be closed)", io_result);
                        receive_result = ASYNC_SOCKET_RECEIVE_ABANDONED;
                        bytes_received = 0;
                        break;
                    }
                    case NO_ERROR:
                    {
                        bytes_received = (uint32_t)number_of_bytes_transferred;

#ifdef ENABLE_SOCKET_LOGGING
                        LogVerbose("Asynchronous receive of %" PRIu32 " bytes completed at %lf", bytes_received, timer_global_get_elapsed_us());
#endif

                        if (bytes_received > io_context->total_buffer_bytes)
                        {
                            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_095: [If io_result is NO_ERROR, but the number of bytes received is greater than the sum of all buffer sizes passed to async_socket_receive_async, the on_receive_complete callback passed to async_socket_receive_async shall be called with on_receive_complete_context as context, ASYNC_SOCKET_RECEIVE_ERROR as result and number_of_bytes_transferred for bytes_received. ]*/
                            LogError("Invalid number of bytes received: %" PRIu32 " expected max: %" PRIu32,
                                bytes_received, io_context->total_buffer_bytes);
                            receive_result = ASYNC_SOCKET_RECEIVE_ERROR;
                        }
                        else if (bytes_received == 0)
                        {
                            /* Codes_SRS_ASYNC_SOCKET_WIN32_42_003: [ If io_result is NO_ERROR, but the number of bytes received is 0, the on_receive_complete callback passed to async_socket_receive_async shall be called with on_receive_complete_context as context, ASYNC_SOCKET_RECEIVE_ABANDONED as result and 0 for bytes_received. ]*/
                            LogError("Socket received 0 bytes, assuming socket is closed");
                            receive_result = ASYNC_SOCKET_RECEIVE_ABANDONED;
                        }
                        else
                        {
                            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_069: [ If io_result is NO_ERROR, the on_receive_complete callback passed to async_socket_receive_async shall be called with on_receive_complete_context as context, ASYNC_SOCKET_RECEIVE_OK as result and number_of_bytes_transferred as bytes_received. ]*/
                            receive_result = ASYNC_SOCKET_RECEIVE_OK;
                        }
                        break;
                    }
                }

                io_context->io.receive.on_receive_complete(io_context->io.receive.on_receive_complete_context, receive_result, bytes_received);

                break;
            }
        }

        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_068: [ on_io_complete shall close the event handle created in async_socket_send_async/async_socket_receive_async. ]*/
        if (!CloseHandle(io_context->overlapped.hEvent))
        {
            LogLastError("CloseHandle failed");
        }

        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_072: [ on_io_complete shall free the IO context. ]*/
        free(io_context);
    }
}

static void internal_close(ASYNC_SOCKET_HANDLE async_socket)
{
    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_020: [ async_socket_close shall wait for all executing async_socket_send_async and async_socket_receive_async APIs. ]*/
    do
    {
        LONG current_pending_api_calls = InterlockedAdd(&async_socket->pending_api_calls, 0);
        if (current_pending_api_calls == 0)
        {
            break;
        }

        (void)WaitOnAddress(&async_socket->pending_api_calls, &current_pending_api_calls, sizeof(current_pending_api_calls), INFINITE);
    } while (1);

    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_040: [ async_socket_close shall wait for any executing callbacks by calling WaitForThreadpoolIoCallbacks, passing FALSE as fCancelPendingCallbacks. ]*/
    WaitForThreadpoolIoCallbacks(async_socket->tp_io, FALSE);

    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_059: [ async_socket_close shall close the threadpool IO created in async_socket_open_async by calling CloseThreadpoolIo. ]*/
    CloseThreadpoolIo(async_socket->tp_io);

    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_042: [ async_socket_close shall destroy the thread pool environment created in async_socket_open_async. ]*/
    DestroyThreadpoolEnvironment(&async_socket->tp_environment);

    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_021: [ Then async_socket_close shall close the async socket. ]*/
    (void)InterlockedExchange(&async_socket->state, (LONG)ASYNC_SOCKET_WIN32_STATE_CLOSED);
    WakeByAddressSingle((PVOID)&async_socket->state);
}

ASYNC_SOCKET_HANDLE async_socket_create_with_transport(EXECUTION_ENGINE_HANDLE execution_engine, ON_ASYNC_SOCKET_SEND on_send, void* on_send_context, ON_ASYNC_SOCKET_RECV on_recv, void* on_recv_context)
{
    (void)execution_engine;
    (void)on_send;
    (void)on_send_context;
    (void)on_recv;
    (void)on_recv_context;

    /* Codes_SRS_ASYNC_SOCKET_WIN32_04_001: [ async_socket_create_with_transport shall fail by returning NULL. ]*/
    LogError("async_socket_create_with_transport is not supported on Windows");
    return NULL;
}

ASYNC_SOCKET_HANDLE async_socket_create(EXECUTION_ENGINE_HANDLE execution_engine)
{
    ASYNC_SOCKET_HANDLE result;

    if (
        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_002: [ If execution_engine is NULL, async_socket_create shall fail and return NULL. ]*/
        (execution_engine == NULL)
       )
    {
        LogError("EXECUTION_ENGINE_HANDLE execution_engine=%p", execution_engine);
    }
    else
    {
        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_001: [ async_socket_create shall allocate a new async socket and on success shall return a non-NULL handle. ]*/
        result = malloc(sizeof(ASYNC_SOCKET));
        if (result == NULL)
        {
            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_003: [ If any error occurs, async_socket_create shall fail and return NULL. ]*/
            LogError("malloc failed");
        }
        else
        {
            /* Codes_SRS_ASYNC_SOCKET_WIN32_42_004: [ async_socket_create shall increment the reference count on execution_engine. ]*/
            execution_engine_inc_ref(execution_engine);
            result->execution_engine = execution_engine;

            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_035: [ async_socket_create shall obtain the PTP_POOL from the execution engine passed to async_socket_create by calling execution_engine_win32_get_threadpool. ]*/
            result->pool = execution_engine_win32_get_threadpool(execution_engine);

            result->socket_transport_handle = NULL;
            (void)InterlockedExchange(&result->pending_api_calls, 0);
            (void)InterlockedExchange(&result->state, (LONG)ASYNC_SOCKET_WIN32_STATE_CLOSED);

            goto all_ok;
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
        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_004: [ If async_socket is NULL, async_socket_destroy shall return. ]*/
        LogError("Invalid arguments: ASYNC_SOCKET_HANDLE async_socket=%p", async_socket);
    }
    else
    {
        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_093: [ While async_socket is OPENING or CLOSING, async_socket_destroy shall wait for the open to complete either successfully or with error. ]*/
        do
        {
            LONG current_state = InterlockedCompareExchange(&async_socket->state, (LONG)ASYNC_SOCKET_WIN32_STATE_CLOSING, (LONG)ASYNC_SOCKET_WIN32_STATE_OPEN);

            if (current_state == (LONG)ASYNC_SOCKET_WIN32_STATE_OPEN)
            {
                /* Codes_SRS_ASYNC_SOCKET_WIN32_01_006: [ async_socket_destroy shall perform an implicit close if async_socket is OPEN. ]*/
                internal_close(async_socket);
                break;
            }
            else if (current_state == (LONG)ASYNC_SOCKET_WIN32_STATE_CLOSED)
            {
                break;
            }

            (void)WaitOnAddress(&async_socket->state, &current_state, sizeof(current_state), INFINITE);
        } while (1);

        /* Codes_SRS_ASYNC_SOCKET_WIN32_42_005: [ async_socket_destroy shall decrement the reference count on the execution engine. ]*/
        execution_engine_dec_ref(async_socket->execution_engine);

        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_005: [ async_socket_destroy shall free all resources associated with async_socket. ]*/
        free(async_socket);
    }
}

int async_socket_open_async(ASYNC_SOCKET_HANDLE async_socket, SOCKET_TRANSPORT_HANDLE socket_transport, ON_ASYNC_SOCKET_OPEN_COMPLETE on_open_complete, void* on_open_complete_context)
{
    int result;

    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_009: [ on_open_complete_context shall be allowed to be NULL. ]*/

    if (
        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_007: [ If async_socket is NULL, async_socket_open_async shall fail and return a non-zero value. ]*/
        (async_socket == NULL) ||
        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_008: [ If on_open_complete is NULL, async_socket_open_async shall fail and return a non-zero value. ]*/
        (on_open_complete == NULL) ||
        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_034: [ If socket_transport is NULL, async_socket_open_async shall fail and return a non-zero value. ]*/
        (socket_transport == NULL)
        )
    {
        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_039: [ If any error occurs, async_socket_open_async shall fail and return a non-zero value. ]*/
        LogError("ASYNC_SOCKET_HANDLE async_socket=%p, ON_ASYNC_SOCKET_OPEN_COMPLETE on_open_complete=%p, void* on_open_complete_context=%p",
            async_socket, on_open_complete, on_open_complete_context);
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_023: [ Otherwise, async_socket_open_async shall switch the state to OPENING. ]*/
        LONG current_state = InterlockedCompareExchange(&async_socket->state, (LONG)ASYNC_SOCKET_WIN32_STATE_OPENING, (LONG)ASYNC_SOCKET_WIN32_STATE_CLOSED);
        if (current_state != (LONG)ASYNC_SOCKET_WIN32_STATE_CLOSED)
        {
            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_015: [ If async_socket is already OPEN or OPENING, async_socket_open_async shall fail and return a non-zero value. ]*/
            LogError("Open called in state %" PRI_MU_ENUM "", MU_ENUM_VALUE(ASYNC_SOCKET_WIN32_STATE, current_state));
            result = MU_FAILURE;
        }
        else
        {
            async_socket->socket_transport_handle = socket_transport;

            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_016: [ Otherwise async_socket_open_async shall initialize a thread pool environment by calling InitializeThreadpoolEnvironment. ]*/
            InitializeThreadpoolEnvironment(&async_socket->tp_environment);

            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_036: [ async_socket_open_async shall set the thread pool for the environment to the pool obtained from the execution engine by calling SetThreadpoolCallbackPool. ]*/
            SetThreadpoolCallbackPool(&async_socket->tp_environment, async_socket->pool);

            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_058: [ async_socket_open_async shall create a threadpool IO by calling CreateThreadpoolIo and passing socket_transport_get_underlying_socket, the callback environment to it and on_io_complete as callback. ]*/
            async_socket->tp_io = CreateThreadpoolIo((HANDLE)socket_transport_get_underlying_socket(async_socket->socket_transport_handle), on_io_complete, NULL, &async_socket->tp_environment);
            if (async_socket->tp_io == NULL)
            {
                /* Codes_SRS_ASYNC_SOCKET_WIN32_01_039: [ If any error occurs, async_socket_open_async shall fail and return a non-zero value. ]*/
                LogLastError("CreateThreadpoolIo failed");
                result = MU_FAILURE;
            }
            else
            {
                /* Codes_SRS_ASYNC_SOCKET_WIN32_01_094: [ async_socket_open_async shall set the state to OPEN. ]*/
                (void)InterlockedExchange(&async_socket->state, (LONG)ASYNC_SOCKET_WIN32_STATE_OPEN);
                WakeByAddressSingle((PVOID)&async_socket->state);

                /* Codes_SRS_ASYNC_SOCKET_WIN32_01_017: [ On success async_socket_open_async shall call on_open_complete_context with ASYNC_SOCKET_OPEN_OK. ]*/
                on_open_complete(on_open_complete_context, ASYNC_SOCKET_OPEN_OK);

                /* Codes_SRS_ASYNC_SOCKET_WIN32_01_014: [ On success, async_socket_open_async shall return 0. ]*/
                result = 0;

                goto all_ok;
            }
            DestroyThreadpoolEnvironment(&async_socket->tp_environment);

            (void)InterlockedExchange(&async_socket->state, (LONG)ASYNC_SOCKET_WIN32_STATE_CLOSED);
            WakeByAddressSingle((PVOID)&async_socket->state);
        }
    }

all_ok:
    return result;
}

void async_socket_close(ASYNC_SOCKET_HANDLE async_socket)
{
    if (async_socket == NULL)
    {
        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_018: [ If async_socket is NULL, async_socket_close shall return. ]*/
        LogError("Invalid arguments: ASYNC_SOCKET_HANDLE async_socket=%p", async_socket);
    }
    else
    {
        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_019: [ Otherwise, async_socket_close shall switch the state to CLOSING. ]*/
        ASYNC_SOCKET_WIN32_STATE current_state;
        if ((current_state = InterlockedCompareExchange(&async_socket->state, (LONG)ASYNC_SOCKET_WIN32_STATE_CLOSING, (LONG)ASYNC_SOCKET_WIN32_STATE_OPEN)) != (LONG)ASYNC_SOCKET_WIN32_STATE_OPEN)
        {
            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_022: [ If async_socket is not OPEN, async_socket_close shall return. ]*/
            LogWarning("Not open, current_state=%" PRI_MU_ENUM "", MU_ENUM_VALUE(ASYNC_SOCKET_WIN32_STATE, current_state));
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

    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_027: [ on_send_complete_context shall be allowed to be NULL. ]*/

    if (
        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_024: [ If async_socket is NULL, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]*/
        (async_socket == NULL) ||
        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_025: [ If buffers is NULL, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]*/
        (buffers == NULL) ||
        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_085: [ If buffer_count is 0, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]*/
        (buffer_count == 0) ||
        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_026: [ If on_send_complete is NULL, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]*/
        (on_send_complete == NULL)
        )
    {
        LogError("Invalid arguments: ASYNC_SOCKET_HANDLE async_socket=%p, const ASYNC_SOCKET_BUFFER* payload=%p, uint32_t buffer_count=%" PRIu32 ", ON_ASYNC_SOCKET_SEND_COMPLETE on_send_complete=%p, void*, on_send_complete_context=%p",
            async_socket, buffers, buffer_count, on_send_complete, on_send_complete_context);
        result = ASYNC_SOCKET_SEND_SYNC_ERROR;
    }
    else
    {
        // limit memory needed to UINT32_MAX
        if (buffer_count > (UINT32_MAX - sizeof(ASYNC_SOCKET_IO_CONTEXT)) / sizeof(WSABUF))
        {
            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_103: [ If the amount of memory needed to allocate the context and the WSABUF items is exceeding UINT32_MAX, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]*/
            LogError("Buffer count too big: %" PRIu32, buffer_count);
            result = ASYNC_SOCKET_SEND_SYNC_ERROR;
        }
        else
        {
            uint32_t i;
            uint32_t total_buffer_bytes = 0;

            for (i = 0; i < buffer_count; i++)
            {
                if (
                    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_089: [ If any of the buffers in payload has buffer set to NULL, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]*/
                    (buffers[i].buffer == NULL) ||
                    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_090: [ If any of the buffers in payload has length set to 0, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]*/
                    (buffers[i].length == 0)
                    )
                {
                    LogError("Invalid buffer %" PRIu32 ": buffer=%p, length = %" PRIu32, i, buffers[i].buffer, buffers[i].length);
                    break;
                }

                if (total_buffer_bytes + buffers[i].length < total_buffer_bytes)
                {
                    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_101: [ If the sum of buffer lengths for all the buffers in payload is greater than UINT32_MAX, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]*/
                    LogError("Overflow in total buffer length computation (total_buffer_bytes=%" PRIu32 " + buffers[i=%" PRIu32 "].length=%" PRIu32 "", total_buffer_bytes, i, buffers[i].length);
                    break;
                }
                else
                {
                    total_buffer_bytes += buffers[i].length;
                }
            }

            if (i < buffer_count)
            {
                LogError("Invalid buffers passed to async_socket_send_async");
                result = ASYNC_SOCKET_SEND_SYNC_ERROR;
            }
            else
            {
                (void)InterlockedIncrement(&async_socket->pending_api_calls);

                ASYNC_SOCKET_WIN32_STATE current_state;
                if ((current_state = InterlockedAdd(&async_socket->state, 0)) != (LONG)ASYNC_SOCKET_WIN32_STATE_OPEN)
                {
                    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_097: [ If async_socket is not OPEN, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_NOT_OPEN. ]*/
                    LogWarning("Not open, current state is %" PRI_MU_ENUM "", MU_ENUM_VALUE(ASYNC_SOCKET_WIN32_STATE, current_state));
                    result = ASYNC_SOCKET_SEND_SYNC_NOT_OPEN;
                }
                else
                {
                    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_028: [ Otherwise async_socket_send_async shall create a context for the send where the payload, on_send_complete and on_send_complete_context shall be stored. ]*/
                    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_050: [ The context shall also allocate enough memory to keep an array of buffer_count WSABUF items. ]*/
                    ASYNC_SOCKET_IO_CONTEXT* send_context = malloc_flex(sizeof(ASYNC_SOCKET_IO_CONTEXT), buffer_count, sizeof(WSABUF));
                    if (send_context == NULL)
                    {
                        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_029: [ If any error occurs, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]*/
                        LogError("failure in malloc_flex(sizeof(ASYNC_SOCKET_IO_CONTEXT)=%zu, buffer_count=%" PRIu32 ", sizeof(WSABUF)=%zu) failed",
                            sizeof(ASYNC_SOCKET_IO_CONTEXT), buffer_count, sizeof(WSABUF));
                        result = ASYNC_SOCKET_SEND_SYNC_ERROR;
                    }
                    else
                    {
                        send_context->total_buffer_bytes = total_buffer_bytes;

                        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_056: [ async_socket_send_async shall set the WSABUF items to point to the memory/length of the buffers in payload. ]*/
                        for (i = 0; i < buffer_count; i++)
                        {
                            send_context->wsa_buffers[i].buffer = buffers[i].buffer;
                            send_context->wsa_buffers[i].length = buffers[i].length;
                        }

                        (void)memset(&send_context->overlapped, 0, sizeof(send_context->overlapped));

                        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_057: [ An event to be used for the OVERLAPPED structure passed to socket_transport_send shall be created and stored in the context. ]*/
                        send_context->overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
                        if (send_context->overlapped.hEvent == NULL)
                        {
                            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_029: [ If any error occurs, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]*/
                            LogLastError("CreateEvent failed");
                            result = ASYNC_SOCKET_SEND_SYNC_ERROR;
                        }
                        else
                        {
                            SOCKET_SEND_RESULT socket_transport_send_result;
                            int wsa_last_error;

                            send_context->io_type = ASYNC_SOCKET_IO_TYPE_SEND;
                            send_context->io.send.on_send_complete = on_send_complete;
                            send_context->io.send.on_send_complete_context = on_send_complete_context;

                            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_060: [ An asynchronous IO shall be started by calling StartThreadpoolIo. ]*/
                            StartThreadpoolIo(async_socket->tp_io);

#ifdef ENABLE_SOCKET_LOGGING
                            LogVerbose("Starting send of %" PRIu32 " bytes at %lf", total_buffer_bytes, timer_global_get_elapsed_us());
#endif

                            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_061: [ The SOCKET_BUFFER array associated with the context shall be sent by calling socket_transport_send and passing to it the OVERLAPPED structure with the event that was just created, flags set to 0, and bytes_sent set to NULL. ]*/
                            socket_transport_send_result = socket_transport_send(async_socket->socket_transport_handle, send_context->wsa_buffers, buffer_count, NULL, 0, &send_context->overlapped);

                            switch (socket_transport_send_result)
                            {
                                default:
                                case SOCKET_SEND_ERROR:
                                {
                                    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_106: [ If socket_transport_send fails with any other error, async_socket_send_async shall call CancelThreadpoolIo and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]*/
                                    LogLastError("socket_transport_send failed with %d", socket_transport_send_result);
                                    result = ASYNC_SOCKET_SEND_SYNC_ERROR;

                                    break;
                                }
                                case SOCKET_SEND_FAILED:
                                {
                                    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_062: [ If socket_transport_send fails, async_socket_send_async shall call WSAGetLastError. ]*/
                                    wsa_last_error = WSAGetLastError();

                                    switch (wsa_last_error)
                                    {
                                        default:
                                        {
                                            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_029: [ If any error occurs, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]*/
                                            LogLastError("socket_transport_send failed with %d, WSAGetLastError returned %lu", socket_transport_send_result, (unsigned long)wsa_last_error);
                                            result = ASYNC_SOCKET_SEND_SYNC_ERROR;

                                            break;
                                        }
                                        case WSAECONNRESET:
                                        {
                                            /* Codes_SRS_ASYNC_SOCKET_WIN32_42_002: [ If WSAGetLastError returns WSAECONNRESET, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_NOT_OPEN. ]*/
                                            LogLastError("socket_transport_send failed with %d, WSAGetLastError returned %lu", socket_transport_send_result, (unsigned long)wsa_last_error);
                                            result = ASYNC_SOCKET_SEND_SYNC_NOT_OPEN;

                                            break;
                                        }
                                        case WSA_IO_PENDING:
                                        {
                                            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_053: [ If WSAGetLastError returns WSA_IO_PENDING, it shall be not treated as an error. ]*/
                                            result = ASYNC_SOCKET_SEND_SYNC_OK;

                                            break;
                                        }
                                    }
                                    break;
                                }
                                case SOCKET_SEND_OK:
                                {
                                    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_045: [ On success, async_socket_send_async shall return ASYNC_SOCKET_SEND_SYNC_OK. ]*/
#ifdef ENABLE_SOCKET_LOGGING
                                    LogVerbose("Send completed synchronously at %lf", timer_global_get_elapsed_us());
#endif
                                    result = ASYNC_SOCKET_SEND_SYNC_OK;

                                    break;
                                }
                            }

                            if (result != ASYNC_SOCKET_SEND_SYNC_OK)
                            {
                                /* Codes_SRS_ASYNC_SOCKET_WIN32_01_100: [ If WSAGetLastError returns any other error, async_socket_send_async shall call CancelThreadpoolIo. ]*/
                                CancelThreadpoolIo(async_socket->tp_io);
                            }
                            else
                            {
                                (void)InterlockedDecrement(&async_socket->pending_api_calls);
                                WakeByAddressSingle((PVOID)&async_socket->pending_api_calls);

                                goto all_ok;
                            }

                            if (!CloseHandle(send_context->overlapped.hEvent))
                            {
                                LogLastError("CloseHandle failed");
                            }
                        }

                        free(send_context);
                    }
                }

                (void)InterlockedDecrement(&async_socket->pending_api_calls);
                WakeByAddressSingle((PVOID)&async_socket->pending_api_calls);
            }
        }
    }

all_ok:
    return result;
}

int async_socket_receive_async(ASYNC_SOCKET_HANDLE async_socket, ASYNC_SOCKET_BUFFER* payload, uint32_t buffer_count, ON_ASYNC_SOCKET_RECEIVE_COMPLETE on_receive_complete, void* on_receive_complete_context)
{
    int result;

    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_076: [ on_receive_complete_context shall be allowed to be NULL. ]*/

    if (
        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_073: [ If async_socket is NULL, async_socket_receive_async shall fail and return a non-zero value. ]*/
        (async_socket == NULL) ||
        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_074: [ If buffers is NULL, async_socket_receive_async shall fail and return a non-zero value. ]*/
        (payload == NULL) ||
        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_086: [ If buffer_count is 0, async_socket_receive_async shall fail and return a non-zero value. ]*/
        (buffer_count == 0) ||
        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_075: [ If on_receive_complete is NULL, async_socket_receive_async shall fail and return a non-zero value. ]*/
        (on_receive_complete == NULL)
        )
    {
        LogError("Invalid arguments: ASYNC_SOCKET_HANDLE async_socket=%p, const ASYNC_SOCKET_BUFFER* payload=%p, uint32_t buffer_count=%" PRIu32 ", ON_ASYNC_SOCKET_RECEIVE_COMPLETE on_receive_complete=%p, void*, on_receive_complete_context=%p",
            async_socket, payload, buffer_count, on_receive_complete, on_receive_complete_context);
        result = MU_FAILURE;
    }
    else
    {
        uint32_t i;
        uint32_t total_buffer_bytes = 0;

        for (i = 0; i < buffer_count; i++)
        {
            if (
                /* Codes_SRS_ASYNC_SOCKET_WIN32_01_091: [ If any of the buffers in payload has buffer set to NULL, async_socket_receive_async shall fail and return a non-zero value. ]*/
                (payload[i].buffer == NULL) ||
                /* Codes_SRS_ASYNC_SOCKET_WIN32_01_092: [ If any of the buffers in payload has length set to 0, async_socket_receive_async shall fail and return a non-zero value. ]*/
                (payload[i].length == 0)
                )
            {
                LogError("Invalid buffer %" PRIu32 ": buffer=%p, length = %" PRIu32, i, payload[i].buffer, payload[i].length);
                break;
            }

            if (total_buffer_bytes + payload[i].length < total_buffer_bytes)
            {
                /* Codes_SRS_ASYNC_SOCKET_WIN32_01_096: [ If the sum of buffer lengths for all the buffers in payload is greater than UINT32_MAX, async_socket_receive_async shall fail and return a non-zero value. ]*/
                LogError("Overflow in total buffer length computation total_buffer_bytes=%" PRIu32 " + payload[i=%" PRIu32 "].length=%" PRIu32 "", total_buffer_bytes, i, payload[i].length);
                break;
            }
            else
            {
                total_buffer_bytes += payload[i].length;
            }
        }

        if (i < buffer_count)
        {
            LogError("Invalid buffers passed to async_socket_receive_async");
            result = MU_FAILURE;
        }
        else
        {
            (void)InterlockedIncrement(&async_socket->pending_api_calls);

            ASYNC_SOCKET_WIN32_STATE current_state;
            if ((current_state = InterlockedAdd(&async_socket->state, 0)) != (LONG)ASYNC_SOCKET_WIN32_STATE_OPEN)
            {
                /* Codes_SRS_ASYNC_SOCKET_WIN32_01_098: [ If async_socket is not OPEN, async_socket_receive_async shall fail and return a non-zero value. ]*/
                LogWarning("Not open, current state is %" PRI_MU_ENUM "", MU_ENUM_VALUE(ASYNC_SOCKET_WIN32_STATE, current_state));
                result = MU_FAILURE;
            }
            else
            {
                /* Codes_SRS_ASYNC_SOCKET_WIN32_01_077: [ Otherwise async_socket_receive_async shall create a context for the send where the payload, on_receive_complete and on_receive_complete_context shall be stored. ]*/
                /* Codes_SRS_ASYNC_SOCKET_WIN32_01_078: [ The context shall also allocate enough memory to keep an array of buffer_count WSABUF items. ]*/
                ASYNC_SOCKET_IO_CONTEXT* receive_context = malloc_flex(sizeof(ASYNC_SOCKET_IO_CONTEXT), buffer_count, sizeof(WSABUF));
                if (receive_context == NULL)
                {
                    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_084: [ If any error occurs, async_socket_receive_async shall fail and return a non-zero value. ]*/
                    LogError("failure in malloc_flex(sizeof(ASYNC_SOCKET_IO_CONTEXT)=%zu, buffer_count=%" PRIu32 ", sizeof(WSABUF)=%zu",
                        sizeof(ASYNC_SOCKET_IO_CONTEXT), buffer_count, sizeof(WSABUF));
                    result = MU_FAILURE;
                }
                else
                {
                    receive_context->total_buffer_bytes = total_buffer_bytes;

                    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_079: [ async_socket_receive_async shall set the WSABUF items to point to the memory/length of the buffers in payload. ]*/
                    for (i = 0; i < buffer_count; i++)
                    {
                        receive_context->wsa_buffers[i].buffer = payload[i].buffer;
                        receive_context->wsa_buffers[i].length = payload[i].length;
                    }

                    (void)memset(&receive_context->overlapped, 0, sizeof(receive_context->overlapped));

                    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_080: [ An event to be used for the OVERLAPPED structure passed to WSARecv shall be created and stored in the context. ]*/
                    receive_context->overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
                    if (receive_context->overlapped.hEvent == NULL)
                    {
                        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_084: [ If any error occurs, async_socket_receive_async shall fail and return a non-zero value. ]*/
                        LogLastError("CreateEvent failed");
                        result = MU_FAILURE;
                    }
                    else
                    {
                        SOCKET_RECEIVE_RESULT socket_transport_receive_result;
                        int wsa_last_error;
                        DWORD flags = 0;

                        receive_context->io_type = ASYNC_SOCKET_IO_TYPE_RECEIVE;
                        receive_context->io.receive.on_receive_complete = on_receive_complete;
                        receive_context->io.receive.on_receive_complete_context = on_receive_complete_context;

                        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_081: [ An asynchronous IO shall be started by calling StartThreadpoolIo. ]*/
                        StartThreadpoolIo(async_socket->tp_io);

#ifdef ENABLE_SOCKET_LOGGING
                        LogVerbose("Starting receive at %lf", timer_global_get_elapsed_us());
#endif

                        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_082: [ A receive shall be started for the WSABUF array associated with the context calling socket_transport_receive and passing to it the OVERLAPPED structure with the event that was just created, flags set to 0, and bytes_sent set to NULL. ]*/
                        socket_transport_receive_result = socket_transport_receive(async_socket->socket_transport_handle, receive_context->wsa_buffers, buffer_count, 0, flags, &receive_context->overlapped);

                        if ((socket_transport_receive_result != SOCKET_RECEIVE_OK) && (socket_transport_receive_result != SOCKET_RECEIVE_ERROR) && (socket_transport_receive_result != SOCKET_RECEIVE_WOULD_BLOCK))
                        {
                            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_105: [ If socket_transport_receive fails with any other error, async_socket_receive_async shall call CancelThreadpoolIo and return a non-zero value. ]*/
                            LogLastError("socket_transport_receive failed with %d", socket_transport_receive_result);
                            CancelThreadpoolIo(async_socket->tp_io);

                            result = MU_FAILURE;
                        }
                        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_054: [ If socket_transport_receive fails with SOCKET_RECEIVE_ERROR, async_socket_receive_async shall call WSAGetLastError. ]*/
                        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_055: [ If WSAGetLastError returns IO_PENDING, it shall be not treated as an error. ]*/
                        else if ((socket_transport_receive_result == SOCKET_RECEIVE_ERROR) && ((wsa_last_error = WSAGetLastError()) != WSA_IO_PENDING))
                        {
                            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_084: [ If any error occurs, async_socket_receive_async shall fail and return a non-zero value. ]*/
                            LogLastError("socket_transport_receive failed with %d, WSAGetLastError returned %lu", socket_transport_receive_result, wsa_last_error);

                            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_099: [ If WSAGetLastError returns any other error, async_socket_receive_async shall call CancelThreadpoolIo. ]*/
                            CancelThreadpoolIo(async_socket->tp_io);

                            result = MU_FAILURE;
                        }
                        else
                        {
                            (void)InterlockedDecrement(&async_socket->pending_api_calls);
                            WakeByAddressSingle((PVOID)&async_socket->pending_api_calls);

                            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_083: [ On success, async_socket_receive_async shall return 0. ]*/
                            result = 0;
                            goto all_ok;
                        }

                        if (!CloseHandle(receive_context->overlapped.hEvent))
                        {
                            LogLastError("CloseHandle failed");
                        }
                    }

                    free(receive_context);
                }
            }

            (void)InterlockedDecrement(&async_socket->pending_api_calls);
            WakeByAddressSingle((PVOID)&async_socket->pending_api_calls);
        }
    }

all_ok:
    return result;
}

/* Codes_SRS_ASYNC_SOCKET_WIN32_04_002: [ async_socket_notify_io_async shall fail by returning a non-zero value. ]*/
int async_socket_notify_io_async(ASYNC_SOCKET_HANDLE async_socket, ASYNC_SOCKET_NOTIFY_IO_TYPE io_type, ON_ASYNC_SOCKET_NOTIFY_IO_COMPLETE on_io_complete, void* on_io_complete_context)
{
    // On Windows we just fail the API call because we don't support this yet.
    (void)async_socket;
    (void)io_type;
    (void)on_io_complete;
    (void)on_io_complete_context;

    LogError("async_socket_notify_io_async is not supported on Windows");
    return MU_FAILURE;
}