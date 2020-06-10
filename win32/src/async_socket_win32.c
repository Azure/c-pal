// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>
#include <inttypes.h>
#include "winsock2.h"
#include "ws2tcpip.h"
#include "windows.h"
#include "azure_macro_utils/macro_utils.h"
#include "azure_c_logging/xlogging.h"
#include "gballoc.h"
#include "async_socket.h"
#include "execution_engine.h"
#include "execution_engine_win32.h"

#include "../inc/async_socket.h"


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

typedef struct ASYNC_SOCKET_TAG
{
    SOCKET_HANDLE socket_handle;
    volatile LONG state;
    PTP_POOL pool;
    TP_CALLBACK_ENVIRON tp_environment;
    PTP_CLEANUP_GROUP tp_cleanup_group;
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
    WSABUF wsa_buffers[];
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
                uint32_t bytes_send = (uint32_t)number_of_bytes_transferred;

                if (bytes_send != io_context->total_buffer_bytes)
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
            
            if (io_result == NO_ERROR)
            {
                bytes_received = (uint32_t)number_of_bytes_transferred;

                if (bytes_received > io_context->total_buffer_bytes)
                {
                    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_095: [If io_result is NO_ERROR, but the number of bytes received is greater than the sum of all buffer sizes passed to async_socket_receive_async, the on_receive_complete callback passed to async_socket_receive_async shall be called with on_receive_complete_context as context, ASYNC_SOCKET_RECEIVE_ERROR as result and number_of_bytes_transferred for bytes_received. ]*/
                    LogError("Invalid number of bytes received: %" PRIu32 " expected max: %" PRIu32,
                        bytes_received, io_context->total_buffer_bytes);
                    receive_result = ASYNC_SOCKET_RECEIVE_ERROR;
                }
                else
                {
                    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_069: [ If io_result is NO_ERROR, the on_receive_complete callback passed to async_socket_receive_async shall be called with on_receive_complete_context as context, ASYNC_SOCKET_RECEIVE_OK as result and number_of_bytes_transferred as bytes_received. ]*/
                    receive_result = ASYNC_SOCKET_RECEIVE_OK;
                }
            }
            else
            {
                /* Codes_SRS_ASYNC_SOCKET_WIN32_01_070: [ If io_result is not NO_ERROR, the on_receive_complete callback passed to async_socket_receive_async shall be called with on_receive_complete_context as context, ASYNC_SOCKET_RECEIVE_ERROR as result and 0 for bytes_received. ]*/
                if (
                    (io_result == ERROR_NETNAME_DELETED) ||
                    (io_result == ERROR_CONNECTION_ABORTED)
                    )
                {
                    LogInfo("Receive IO completed with error %lu", io_result);
                }
                else
                {
                    LogError("Receive IO completed with error %lu", io_result);
                }

                receive_result = ASYNC_SOCKET_RECEIVE_ERROR;
                bytes_received = 0;
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

    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_041: [ async_socket_close shall close the threadpool cleanup group by calling CloseThreadpoolCleanupGroup. ]*/
    CloseThreadpoolCleanupGroup(async_socket->tp_cleanup_group);

    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_042: [ async_socket_close shall destroy the thread pool environment created in async_socket_open_async. ]*/
    DestroyThreadpoolEnvironment(&async_socket->tp_environment);

    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_021: [ Then async_socket_close shall close the async socket, leaving it in a state where an async_socket_open_async can be performed. ]*/
    (void)InterlockedExchange(&async_socket->state, (LONG)ASYNC_SOCKET_WIN32_STATE_CLOSED);
    WakeByAddressSingle((PVOID)&async_socket->state);
}

ASYNC_SOCKET_HANDLE async_socket_create(EXECUTION_ENGINE_HANDLE execution_engine, SOCKET_HANDLE socket_handle)
{
    ASYNC_SOCKET_HANDLE result;
    SOCKET win32_socket = (SOCKET)socket_handle;

    if (
        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_002: [ If execution_engine is NULL, async_socket_create shall fail and return NULL. ]*/
        (execution_engine == NULL) ||
        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_034: [ If socket_handle is INVALID_SOCKET, async_socket_create shall fail and return NULL. ]*/
        (win32_socket == INVALID_SOCKET))
    {
        LogError("EXECUTION_ENGINE_HANDLE execution_engine=%p, SOCKET_HANDLE socket_handle=%p",
            execution_engine, (void*)win32_socket);
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
            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_035: [ Otherwise, async_socket_open_async shall obtain the PTP_POOL from the execution engine passed to async_socket_create by calling execution_engine_win32_get_threadpool. ]*/
            result->pool = execution_engine_win32_get_threadpool(execution_engine);
            result->socket_handle = socket_handle;

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
        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_093: [ While async_socket is OPENING or CLOSING, async_socket_destroy shall wait for the open to complete either succesfully or with error. ]*/
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
        }
        while (1);

        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_005: [ Otherwise, async_socket_destroy shall free all resources associated with async_socket. ]*/
        free(async_socket);
    }
}

int async_socket_open_async(ASYNC_SOCKET_HANDLE async_socket, ON_ASYNC_SOCKET_OPEN_COMPLETE on_open_complete, void* on_open_complete_context)
{
    int result;

    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_009: [ on_open_complete_context shall be allowed to be NULL. ]*/

    if (
        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_007: [ If async_socket is NULL, async_socket_open_async shall fail and return a non-zero value. ]*/
        (async_socket == NULL) ||
        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_008: [ If on_open_complete is NULL, async_socket_open_async shall fail and return a non-zero value. ]*/
        (on_open_complete == NULL))
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
            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_016: [ Otherwise async_socket_open_async shall initialize a thread pool environment by calling InitializeThreadpoolEnvironment. ]*/
            InitializeThreadpoolEnvironment(&async_socket->tp_environment);

            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_036: [ async_socket_open_async shall set the thread pool for the environment to the pool obtained from the execution engine by calling SetThreadpoolCallbackPool. ]*/
            SetThreadpoolCallbackPool(&async_socket->tp_environment, async_socket->pool);

            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_037: [ async_socket_open_async shall create a threadpool cleanup group by calling CreateThreadpoolCleanupGroup. ]*/
            async_socket->tp_cleanup_group = CreateThreadpoolCleanupGroup();
            if (async_socket->tp_cleanup_group == NULL)
            {
                /* Codes_SRS_ASYNC_SOCKET_WIN32_01_039: [ If any error occurs, async_socket_open_async shall fail and return a non-zero value. ]*/
                LogLastError("CreateThreadpoolCleanupGroup failed");
                result = MU_FAILURE;
            }
            else
            {
                /* Codes_SRS_ASYNC_SOCKET_WIN32_01_058: [ async_socket_open_async shall create a threadpool IO by calling CreateThreadpoolIo and passing socket_handle, the callback environment to it and on_io_complete as callback. ]*/
                async_socket->tp_io = CreateThreadpoolIo(async_socket->socket_handle, on_io_complete, NULL, &async_socket->tp_environment);
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

                CloseThreadpoolCleanupGroup(async_socket->tp_cleanup_group);
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
        if (InterlockedCompareExchange(&async_socket->state, (LONG)ASYNC_SOCKET_WIN32_STATE_CLOSING, (LONG)ASYNC_SOCKET_WIN32_STATE_OPEN) != (LONG)ASYNC_SOCKET_WIN32_STATE_OPEN)
        {
            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_022: [ If async_socket is not OPEN, async_socket_close shall return. ]*/
            LogWarning("Not open");
        }
        else
        {
            internal_close(async_socket);
        }
    }
}

int async_socket_send_async(ASYNC_SOCKET_HANDLE async_socket, const ASYNC_SOCKET_BUFFER* buffers, uint32_t buffer_count, ON_ASYNC_SOCKET_SEND_COMPLETE on_send_complete, void* on_send_complete_context)
{
    int result;

    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_027: [ on_send_complete_context shall be allowed to be NULL. ]*/

    if (
        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_024: [ If async_socket is NULL, async_socket_send_async shall fail and return a non-zero value. ]*/
        (async_socket == NULL) ||
        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_025: [ If buffers is NULL, async_socket_send_async shall fail and return a non-zero value. ]*/
        (buffers == NULL) ||
        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_085: [ If buffer_count is 0, async_socket_send_async shall fail and return a non-zero value. ]*/
        (buffer_count == 0) ||
        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_026: [ If on_send_complete is NULL, async_socket_send_async shall fail and return a non-zero value. ]*/
        (on_send_complete == NULL)
        )
    {
        LogError("Invalid arguments: ASYNC_SOCKET_HANDLE async_socket=%p, const ASYNC_SOCKET_BUFFER* payload=%p, uint32_t buffer_count=%" PRIu32 ", ON_ASYNC_SOCKET_SEND_COMPLETE on_send_complete=%p, void*, on_send_complete_context=%p",
            async_socket, buffers, buffer_count, on_send_complete, on_send_complete_context);
        result = MU_FAILURE;
    }
    else
    {
        // limit memory needed to UINT32_MAX
        if (buffer_count > (UINT32_MAX - sizeof(ASYNC_SOCKET_IO_CONTEXT)) / sizeof(WSABUF))
        {
            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_103: [ If the amount of memory needed to allocate the context and the WSABUF items is exceeding UINT32_MAX, async_socket_send_async shall fail and return a non-zero value. ]*/
            LogError("Buffer count too big: %" PRIu32, buffer_count);
            result = MU_FAILURE;
        }
        else
        {
            uint32_t i;
            uint32_t total_buffer_bytes = 0;

            for (i = 0; i < buffer_count; i++)
            {
                if (
                    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_089: [ If any of the buffers in payload has buffer set to NULL, async_socket_send_async shall fail and return a non-zero value. ]*/
                    (buffers[i].buffer == NULL) ||
                    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_090: [ If any of the buffers in payload has length set to 0, async_socket_send_async shall fail and return a non-zero value. ]*/
                    (buffers[i].length == 0)
                    )
                {
                    LogError("Invalid buffer %" PRIu32 ": buffer=%p, length = %" PRIu32, i, buffers[i].buffer, buffers[i].length);
                    break;
                }

                if (total_buffer_bytes + buffers[i].length < total_buffer_bytes)
                {
                    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_101: [ If the sum of buffer lengths for all the buffers in payload is greater than UINT32_MAX, async_socket_send_async shall fail and return a non-zero value. ]*/
                    LogError("Overflow in total buffer length computation");
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
                result = MU_FAILURE;
            }
            else
            {
                (void)InterlockedIncrement(&async_socket->pending_api_calls);

                if (InterlockedAdd(&async_socket->state, 0) != (LONG)ASYNC_SOCKET_WIN32_STATE_OPEN)
                {
                    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_097: [ If async_socket is not OPEN, async_socket_send_async shall fail and return a non-zero value. ]*/
                    LogWarning("Not open");
                    result = MU_FAILURE;
                }
                else
                {
                    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_028: [ Otherwise async_socket_send_async shall create a context for the send where the payload, on_send_complete and on_send_complete_context shall be stored. ]*/
                    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_050: [ The context shall also allocate enough memory to keep an array of buffer_count WSABUF items. ]*/
                    ASYNC_SOCKET_IO_CONTEXT* send_context = malloc(sizeof(ASYNC_SOCKET_IO_CONTEXT) + (sizeof(WSABUF) * buffer_count));
                    if (send_context == NULL)
                    {
                        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_029: [ If any error occurs, async_socket_send_async shall fail and return a non-zero value. ]*/
                        LogError("malloc failed");
                        result = MU_FAILURE;
                    }
                    else
                    {
                        send_context->total_buffer_bytes = total_buffer_bytes;

                        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_056: [ async_socket_send_async shall set the WSABUF items to point to the memory/length of the buffers in payload. ]*/
                        for (i = 0; i < buffer_count; i++)
                        {
                            send_context->wsa_buffers[i].buf = buffers[i].buffer;
                            send_context->wsa_buffers[i].len = buffers[i].length;
                        }

                        (void)memset(&send_context->overlapped, 0, sizeof(send_context->overlapped));

                        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_057: [ An event to be used for the OVERLAPPED structure passed to WSASend shall be created and stored in the context. ]*/
                        send_context->overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
                        if (send_context->overlapped.hEvent == NULL)
                        {
                            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_029: [ If any error occurs, async_socket_send_async shall fail and return a non-zero value. ]*/
                            LogLastError("CreateEvent failed");
                            result = MU_FAILURE;
                        }
                        else
                        {
                            int wsa_send_result;
                            int wsa_last_error;

                            send_context->io_type = ASYNC_SOCKET_IO_TYPE_SEND;
                            send_context->io.send.on_send_complete = on_send_complete;
                            send_context->io.send.on_send_complete_context = on_send_complete_context;

                            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_060: [ An asynchronous IO shall be started by calling StartThreadpoolIo. ]*/
                            StartThreadpoolIo(async_socket->tp_io);

                            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_061: [ The WSABUF array associated with the context shall be sent by calling WSASend and passing to it the OVERLAPPED structure with the event that was just created, dwFlags set to 0, lpNumberOfBytesSent set to NULL and lpCompletionRoutine set to NULL. ]*/
                            wsa_send_result = WSASend((SOCKET)async_socket->socket_handle, send_context->wsa_buffers, buffer_count, NULL, 0, &send_context->overlapped, NULL);

                            if ((wsa_send_result != 0) && (wsa_send_result != SOCKET_ERROR))
                            {
                                /* Codes_SRS_ASYNC_SOCKET_WIN32_01_106: [ If WSASend fails with any other error, async_socket_send_async shall call CancelThreadpoolIo and return a non-zero value. ]*/
                                LogLastError("WSARecv failed with %d", wsa_send_result);
                                CancelThreadpoolIo(async_socket->tp_io);

                                result = MU_FAILURE;
                            }
                            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_053: [ If WSAGetLastError returns WSA_IO_PENDING, it shall be not treated as an error. ]*/
                            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_062: [ If WSASend fails, async_socket_send_async shall call WSAGetLastError. ]*/
                            else if ((wsa_send_result == SOCKET_ERROR) && ((wsa_last_error = WSAGetLastError()) != WSA_IO_PENDING))
                            {
                                /* Codes_SRS_ASYNC_SOCKET_WIN32_01_029: [ If any error occurs, async_socket_send_async shall fail and return a non-zero value. ]*/
                                LogLastError("WSASend failed with %d, WSAGetLastError returned %lu", wsa_send_result, (unsigned long)wsa_last_error);

                                /* Codes_SRS_ASYNC_SOCKET_WIN32_01_100: [ If WSAGetLastError returns any other error, async_socket_send_async shall call CancelThreadpoolIo. ]*/
                                CancelThreadpoolIo(async_socket->tp_io);

                                result = MU_FAILURE;
                            }
                            else
                            {
                                (void)InterlockedDecrement(&async_socket->pending_api_calls);
                                WakeByAddressSingle((PVOID)&async_socket->pending_api_calls);

                                /* Codes_SRS_ASYNC_SOCKET_WIN32_01_045: [ On success, async_socket_send_async shall return 0. ]*/
                                result = 0;
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
        // limit memory needed to UINT32_MAX
        if (buffer_count > (UINT32_MAX - sizeof(ASYNC_SOCKET_IO_CONTEXT)) / sizeof(WSABUF))
        {
            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_104: [ If the amount of memory needed to allocate the context and the WSABUF items is exceeding UINT32_MAX, async_socket_receive_async shall fail and return a non-zero value. ]*/
            LogError("Buffer count too big: %" PRIu32, buffer_count);
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
                    LogError("Overflow in total buffer length computation");
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

                if (InterlockedAdd(&async_socket->state, 0) != (LONG)ASYNC_SOCKET_WIN32_STATE_OPEN)
                {
                    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_098: [ If async_socket is not OPEN, async_socket_receive_async shall fail and return a non-zero value. ]*/
                    LogWarning("Not open");
                    result = MU_FAILURE;
                }
                else
                {
                    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_077: [ Otherwise async_socket_receive_async shall create a context for the send where the payload, on_receive_complete and on_receive_complete_context shall be stored. ]*/
                    /* Codes_SRS_ASYNC_SOCKET_WIN32_01_078: [ The context shall also allocate enough memory to keep an array of buffer_count WSABUF items. ]*/
                    ASYNC_SOCKET_IO_CONTEXT* receive_context = malloc(sizeof(ASYNC_SOCKET_IO_CONTEXT) + (sizeof(WSABUF) * buffer_count));
                    if (receive_context == NULL)
                    {
                        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_084: [ If any error occurs, async_socket_receive_async shall fail and return a non-zero value. ]*/
                        LogError("malloc failed");
                        result = MU_FAILURE;
                    }
                    else
                    {
                        receive_context->total_buffer_bytes = total_buffer_bytes;

                        /* Codes_SRS_ASYNC_SOCKET_WIN32_01_079: [ async_socket_receive_async shall set the WSABUF items to point to the memory/length of the buffers in payload. ]*/
                        for (i = 0; i < buffer_count; i++)
                        {
                            receive_context->wsa_buffers[i].buf = payload[i].buffer;
                            receive_context->wsa_buffers[i].len = payload[i].length;
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
                            int wsa_receive_result;
                            int wsa_last_error;
                            DWORD flags = 0;

                            receive_context->io_type = ASYNC_SOCKET_IO_TYPE_RECEIVE;
                            receive_context->io.receive.on_receive_complete = on_receive_complete;
                            receive_context->io.receive.on_receive_complete_context = on_receive_complete_context;

                            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_081: [ An asynchronous IO shall be started by calling StartThreadpoolIo. ]*/
                            StartThreadpoolIo(async_socket->tp_io);

                            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_082: [ A receive shall be started for the WSABUF array associated with the context calling WSARecv and passing to it the OVERLAPPED structure with the event that was just created, dwFlags set to 0, lpNumberOfBytesSent set to NULL and lpCompletionRoutine set to NULL. ]*/
                            wsa_receive_result = WSARecv((SOCKET)async_socket->socket_handle, receive_context->wsa_buffers, buffer_count, NULL, &flags, &receive_context->overlapped, NULL);

                            if ((wsa_receive_result != 0) && (wsa_receive_result != SOCKET_ERROR))
                            {
                                /* Codes_SRS_ASYNC_SOCKET_WIN32_01_105: [ If WSARecv fails with any other error, async_socket_receive_async shall call CancelThreadpoolIo and return a non-zero value. ]*/
                                LogLastError("WSARecv failed with %d", wsa_receive_result);
                                CancelThreadpoolIo(async_socket->tp_io);

                                result = MU_FAILURE;
                            }
                            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_054: [ If WSARecv fails with SOCKET_ERROR, async_socket_receive_async shall call WSAGetLastError. ]*/
                            /* Codes_SRS_ASYNC_SOCKET_WIN32_01_055: [ If WSAGetLastError returns IO_PENDING, it shall be not treated as an error. ]*/
                            else if ((wsa_receive_result == SOCKET_ERROR) && ((wsa_last_error = WSAGetLastError()) != WSA_IO_PENDING))
                            {
                                /* Codes_SRS_ASYNC_SOCKET_WIN32_01_084: [ If any error occurs, async_socket_receive_async shall fail and return a non-zero value. ]*/
                                LogLastError("WSARecv failed with %d, WSAGetLastError returned %lu", wsa_receive_result, wsa_last_error);

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
    }

all_ok:
    return result;
}
