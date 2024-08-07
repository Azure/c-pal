// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#include "winsock2.h"
#include "ws2tcpip.h"
#include "windows.h"

#include "macro_utils/macro_utils.h"

#include "umock_c/umock_c_prod.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/execution_engine.h"
#include "c_pal/execution_engine_win32.h"
#include "c_pal/interlocked.h"
#include "c_pal/sm.h"
#include "c_pal/sync.h"
#include "c_pal/string_utils.h"
#include "c_pal/timer.h"

#include "c_pal/socket_transport.h"

#define SOCKET_IO_TYPE_VALUES \
    SOCKET_IO_TYPE_SEND, \
    SOCKET_IO_TYPE_RECEIVE

MU_DEFINE_ENUM(SOCKET_IO_TYPE, SOCKET_IO_TYPE_VALUES)
MU_DEFINE_ENUM_STRINGS(SOCKET_IO_TYPE, SOCKET_IO_TYPE_VALUES)

MU_DEFINE_ENUM_STRINGS(SOCKET_SEND_RESULT, SOCKET_SEND_RESULT_VALUES)
MU_DEFINE_ENUM_STRINGS(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_RESULT_VALUES)
MU_DEFINE_ENUM_STRINGS(SOCKET_ACCEPT_RESULT, SOCKET_ACCEPT_RESULT_VALUES)
MU_DEFINE_ENUM_STRINGS(SOCKET_TYPE, SOCKET_TYPE_VALUES)

typedef struct SOCKET_TRANSPORT_TAG
{
    SOCKET socket;
    SM_HANDLE sm;
    SOCKET_TYPE type;
} SOCKET_TRANSPORT;

static int connect_to_endpoint(SOCKET client_socket, const ADDRINFO* addrInfo, uint32_t connection_timeout_ms)
{
    int result;

    // Codes_SOCKET_TRANSPORT_WIN32_09_019: [ connect_to_endpoint shall call connect using connection_timeout_ms as a timeout for the connection. ]
    if (connect(client_socket, addrInfo->ai_addr, (int)addrInfo->ai_addrlen) != 0)
    {
        // Codes_ SOCKET_TRANSPORT_WIN32_09_020: [ If the connect call fails, connect_to_endpoint shall check to WSAGetLastError for WSAEWOULDBLOCK.
        if (WSAGetLastError() == WSAEWOULDBLOCK)
        {
            fd_set write_fds;
            FD_ZERO(&write_fds);
            FD_SET(client_socket, &write_fds);

            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = connection_timeout_ms * 100;

            // Codes_SOCKET_TRANSPORT_WIN32_09_021: [ On WSAEWOULDBLOCK connect_to_endpoint shall call the select API with the connection_timeout_ms and check the return value: ]
            int ret = select(0, NULL, &write_fds, NULL, &tv);

            // Codes_SOCKET_TRANSPORT_WIN32_09_022: [ If the return is SOCKET_ERROR, this indicates a failure and connect_to_endpoint shall fail. ]
            if (ret == SOCKET_ERROR)
            {
                LogError("Failure connecting error: %d", WSAGetLastError());
                result = MU_FAILURE;
            }

            // Codes_SOCKET_TRANSPORT_WIN32_09_023: [ If the return value is 0, this indicates a timeout and connect_to_endpoint shall fail. ]
            else if (ret == 0)
            {
                LogError("Failure timeout (%" PRId32 " us) attempting to connect", connection_timeout_ms);
                result = MU_FAILURE;
            }

            // Codes_SOCKET_TRANSPORT_WIN32_09_024: [ Any other value this indicates a possible success and connect_to_endpoint shall test if the socket is writable by calling FD_ISSET. ]
            else
            {
                if (FD_ISSET(client_socket, &write_fds))
                {
                    result = 0;
                }
                else
                {
                    LogError("Failure connection is not writable: %d", WSAGetLastError());
                    result = MU_FAILURE;
                }
            }
        }
        // Codes_SOCKET_TRANSPORT_WIN32_09_026: [ If any error is encountered connect_to_endpoint shall return a non-zero value. ]
        else
        {
            LogLastError("Winsock connect failure");
            result = MU_FAILURE;
        }
    }
    // Codes_SOCKET_TRANSPORT_WIN32_09_025: [ If the socket is writable connect_to_endpoint shall succeed and return a 0 value. ]
    else
    {
        result = 0;
    }
    return result;
}

static SOCKET connect_to_client(const char* hostname, uint16_t port, uint32_t connection_timeout)
{
    SOCKET result;
    // Codes_SOCKET_TRANSPORT_WIN32_09_015: [ socket_transport_connect shall call socket with the params AF_INET, SOCK_STREAM and IPPROTO_TCP. ]
    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket == INVALID_SOCKET)
    {
        LogLastError("Failure: socket create failure.");
        result = INVALID_SOCKET;
    }
    else
    {
        char port_string[16];
        if (sprintf(port_string, "%u", port) < 0)
        {
            LogError("failed to copy port value %" PRIu16 "", port);
            result = INVALID_SOCKET;
        }
        else
        {
            ADDRINFO addr_hint = { 0 };
            ADDRINFO* addrInfo = NULL;

            addr_hint.ai_family = AF_INET;
            addr_hint.ai_socktype = SOCK_STREAM;
            addr_hint.ai_protocol = 0;
            addr_hint.ai_flags = AI_CANONNAME;

            if (getaddrinfo(hostname, port_string, &addr_hint, &addrInfo) != 0)
            {
                LogLastError("Failure: getaddrinfo(hostname=%s, portString=%s, &addr_hint=%p, &addrInfo=%p)", hostname, port_string, &addr_hint, &addrInfo);
                result = INVALID_SOCKET;
            }
            else
            {
                LogInfo("Connecting to %s:%" PRIu16 " Machine Name: %s, connection timeout: %" PRIu32 "", hostname, port, MU_P_OR_NULL(addrInfo->ai_canonname), connection_timeout);

                u_long iMode = 1;
                if (ioctlsocket(client_socket, FIONBIO, &iMode) != 0)
                {
                    LogLastError("Failure: ioctlsocket failure.");
                    result = INVALID_SOCKET;
                }
                // Codes_SOCKET_TRANSPORT_WIN32_09_016: [ socket_transport_connect shall call connect_to_endpoint with the connection_timeout_ms to connect to the endpoint. ]
                else if (connect_to_endpoint(client_socket, addrInfo, connection_timeout) != 0)
                {
                    LogLastError("Connection failure. Server: %s:%" PRIu16 " Machine Name: %s.", hostname, port, MU_P_OR_NULL(addrInfo->ai_canonname));
                    result = INVALID_SOCKET;
                }
                else
                {
                    freeaddrinfo(addrInfo);
                    result = client_socket;
                    goto all_ok;
                }
                freeaddrinfo(addrInfo);
            }
        }
        closesocket(client_socket);
    }
all_ok:
    return result;
}

SOCKET_TRANSPORT_HANDLE socket_transport_create_client()
{
    SOCKET_TRANSPORT* result;

    // Codes_SOCKET_TRANSPORT_WIN32_09_002: [ socket_transport_create_client shall allocate a new SOCKET_TRANSPORT object. ]
    result = malloc(sizeof(SOCKET_TRANSPORT));
    if (result == NULL)
    {
        LogError("failure allocating SOCKET_TRANSPORT: %zu", sizeof(SOCKET_TRANSPORT));
    }
    else
    {
        // Codes_SOCKET_TRANSPORT_WIN32_09_003: [ socket_transport_create_client shall call sm_create to create a sm object with the type set to SOCKET_CLIENT. ]
        result->sm = sm_create("Socket_transport_win32");
        if (result->sm == NULL)
        {
            LogError("sm_create failed.");
        }
        else
        {
            result->type = SOCKET_CLIENT;
            goto all_ok;
        }
        free(result);
        // Codes_SOCKET_TRANSPORT_WIN32_09_004: [ On any failure socket_transport_create shall return NULL. ]
        result = NULL;
    }
all_ok:
    // Codes_SOCKET_TRANSPORT_WIN32_09_005: [ On success socket_transport_create shall return SOCKET_TRANSPORT_HANDLE. ]
    return result;
}

SOCKET_TRANSPORT_HANDLE socket_transport_create_server()
{
    SOCKET_TRANSPORT* result;

    // Codes_SOCKET_TRANSPORT_WIN32_09_087: [ socket_transport_create_server shall allocate a new SOCKET_TRANSPORT object. ]
    result = malloc(sizeof(SOCKET_TRANSPORT));
    if (result == NULL)
    {
        LogError("failure allocating SOCKET_TRANSPORT: %zu", sizeof(SOCKET_TRANSPORT));
    }
    else
    {
        // Codes_SOCKET_TRANSPORT_WIN32_09_088: [ socket_transport_create_server shall call sm_create to create a sm object with the type set to SOCKET_BINDING. ]
        result->sm = sm_create("Socket_transport_win32");
        if (result->sm == NULL)
        {
            LogError("sm_create failed.");
        }
        else
        {
            result->type = SOCKET_BINDING;
            goto all_ok;
        }
        free(result);
        // Codes_SOCKET_TRANSPORT_WIN32_09_089: [ On any failure socket_transport_create_server shall return NULL. ]
        result = NULL;
    }
all_ok:
    // Codes_SOCKET_TRANSPORT_WIN32_09_090: [ On success socket_transport_create_server shall return SOCKET_TRANSPORT_HANDLE. ]
    return result;
}

void socket_transport_destroy(SOCKET_TRANSPORT_HANDLE socket_transport)
{
    // Codes_SOCKET_TRANSPORT_WIN32_09_006: [ If socket_transport is NULL socket_transport_destroy shall return. ]
    if (socket_transport == NULL)
    {
        // Do nothing
    }
    else
    {
        // Codes_SOCKET_TRANSPORT_WIN32_09_007: [ socket_transport_destroy shall call sm_destroy to destroy the sm object. ]
        sm_destroy(socket_transport->sm);

        // Codes_SOCKET_TRANSPORT_WIN32_09_008: [ socket_transport_destroy shall free the SOCKET_TRANSPORT_HANDLE object. ]
        free(socket_transport);
    }
}


int socket_transport_connect(SOCKET_TRANSPORT_HANDLE socket_transport, const char* hostname, uint16_t port, uint32_t connection_timeout_ms)
{
    int result;

    // Codes_SOCKET_TRANSPORT_WIN32_09_009: [ If socket_transport is NULL, socket_transport_connect shall fail and return a non-zero value. ]
    if (socket_transport == NULL ||
        // Codes_SOCKET_TRANSPORT_WIN32_09_010: [If hostname is NULL, socket_transport_connect shall fail and return a non - zero value.]
        hostname == NULL ||
        // Codes_SOCKET_TRANSPORT_WIN32_09_011 : [If port is 0, socket_transport_connect shall fail and return a non - zero value.]
        port == 0)
    {
        LogError("Invalid arguments: SOCKET_TRANSPORT_HANDLE socket_transport: %p, const char* hostname: %s, uint16_t port: %" PRIu16 ", uint32_t connection_timeout: %" PRIu32 "",
            socket_transport, MU_P_OR_NULL(hostname), port, connection_timeout_ms);
        result = MU_FAILURE;
    }
    else
    {
        // Codes_SOCKET_TRANSPORT_WIN32_09_012: [ If the socket_transport type is not SOCKET_CLIENT, socket_transport_connect shall fail and return a non-zero value. ]
        if (socket_transport->type != SOCKET_CLIENT)
        {
            LogError("Invalid socket type for this API expected: SOCKET_CLIENT, actual: %" PRI_MU_ENUM "", MU_ENUM_VALUE(SOCKET_TYPE, socket_transport->type));
            result = MU_FAILURE;
        }
        else
        {
            // Codes_SOCKET_TRANSPORT_WIN32_09_013: [ socket_transport_connect shall call sm_open_begin to begin the open. ]
            SM_RESULT open_result = sm_open_begin(socket_transport->sm);

            // Codes_SOCKET_TRANSPORT_WIN32_09_014: [ If sm_open_begin does not return SM_EXEC_GRANTED, socket_transport_connect shall fail and return a non-zero value. ]
            if (open_result != SM_EXEC_GRANTED)
            {
                LogError("sm_open_begin failed with %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, open_result));
                result = MU_FAILURE;
            }
            else
            {
                socket_transport->socket = connect_to_client(hostname, port, connection_timeout_ms);

                if (socket_transport->socket == INVALID_SOCKET)
                {
                    LogError("Failure connecting to client hostname: %s:%" PRIu16 "", hostname, port);
                    result = MU_FAILURE;
                    // Codes_SOCKET_TRANSPORT_WIN32_09_018: [ If any failure is encountered, socket_transport_connect shall call sm_open_end with false, fail and return a non-zero value. ]
                    sm_open_end(socket_transport->sm, false);
                }
                else
                {
                    // Codes_SOCKET_TRANSPORT_WIN32_09_017: [ If successful socket_transport_connect shall call sm_open_end with true. ]
                    sm_open_end(socket_transport->sm, true);
                    result = 0;
                }
            }
        }
    }
    return result;
}

void socket_transport_disconnect(SOCKET_TRANSPORT_HANDLE socket_transport)
{
    // Codes_SOCKET_TRANSPORT_WIN32_09_027: [ If socket_transport is NULL, socket_transport_disconnect shall fail and return. ]
    if (socket_transport == NULL)
    {
        LogError("Invalid arguments: SOCKET_TRANSPORT_HANDLE socket_transport: %p",
            socket_transport);
    }
    else
    {
        // Codes_SOCKET_TRANSPORT_WIN32_09_028: [ socket_transport_disconnect shall call sm_close_begin to begin the closing process. ]
        SM_RESULT close_result = sm_close_begin(socket_transport->sm);
        if (close_result == SM_EXEC_GRANTED)
        {
            // Codes_SOCKET_TRANSPORT_WIN32_09_083: [ If shutdown does not return 0 on a socket that is not a binding socket, the socket is not valid therefore socket_transport_disconnect shall not call close ]
            if (socket_transport->type != SOCKET_BINDING && shutdown(socket_transport->socket, SD_BOTH) != 0)
            {
                LogLastError("shutdown failed on socket: %" PRI_SOCKET "", socket_transport->socket);
            }
            else
            {
                // Codes_SOCKET_TRANSPORT_WIN32_09_030: [ socket_transport_disconnect shall call closesocket to disconnect the connected socket. ]
                if (closesocket(socket_transport->socket) != 0)
                {
                    LogLastError("Failure in closesocket %" PRI_SOCKET "", socket_transport->socket);
                }
            }
            // Codes_SOCKET_TRANSPORT_WIN32_09_031: [ socket_transport_disconnect shall call sm_close_end. ]
            sm_close_end(socket_transport->sm);
        }

        // Codes_SOCKET_TRANSPORT_WIN32_09_029: [ If sm_close_begin does not return SM_EXEC_GRANTED, socket_transport_disconnect shall fail and return. ]
        else
        {
            LogError("sm_close_begin failed with %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, close_result));
        }
    }
}

SOCKET_SEND_RESULT socket_transport_send(SOCKET_TRANSPORT_HANDLE socket_transport, const SOCKET_BUFFER* payload, uint32_t buffer_count, uint32_t* bytes_sent, uint32_t flags, void* overlapped_data)
{
    SOCKET_SEND_RESULT result;
    // Codes_SOCKET_TRANSPORT_WIN32_09_032: [ If socket_transport is NULL, socket_transport_send shall fail and return SOCKET_SEND_INVALID_ARG. ]
    if (socket_transport == NULL ||
        // Codes_SOCKET_TRANSPORT_WIN32_09_033: [ If payload is NULL, socket_transport_send shall fail and return SOCKET_SEND_INVALID_ARG. ]
        payload == NULL ||
        // Codes_SOCKET_TRANSPORT_WIN32_09_034: [ If buffer_count is 0, socket_transport_send shall fail and return SOCKET_SEND_INVALID_ARG. ]
        buffer_count == 0)
    {
        LogError("Invalid arguments: SOCKET_TRANSPORT_HANDLE socket_transport: %p, const SOCKET_BUFFER* payload: %p, void* overlapped_data: %p",
            socket_transport, payload, overlapped_data);
        result = SOCKET_SEND_INVALID_ARG;
    }
    else
    {
        // Codes_SOCKET_TRANSPORT_WIN32_09_035: [ socket_transport_send shall call sm_exec_begin. ]
        SM_RESULT sm_result = sm_exec_begin(socket_transport->sm);

        // Codes_SOCKET_TRANSPORT_WIN32_09_036: [ If sm_exec_begin does not return SM_EXEC_GRANTED, socket_transport_send shall fail and return SOCKET_SEND_ERROR. ]
        if (sm_result != SM_EXEC_GRANTED)
        {
            LogError("sm_exec_begin failed : %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, sm_result));
            result = SOCKET_SEND_ERROR;
        }
        else
        {
            DWORD num_bytes = 0;
            LPDWORD num_bytes_param = NULL;
            // If the overlapped is NULL then set the num bytes params
            if (overlapped_data == NULL)
            {
                num_bytes_param = &num_bytes;
            }

            int send_result;

            // Codes_SOCKET_TRANSPORT_WIN32_09_037: [ socket_transport_send shall call WSASend to send data with flags and the overlapped_data. ]
            send_result = WSASend(socket_transport->socket, (LPWSABUF)payload, buffer_count, num_bytes_param, flags, (LPWSAOVERLAPPED)overlapped_data, NULL);

            // Codes_SOCKET_TRANSPORT_WIN32_09_038: [ If WSASend returns 0, socket_transport_send shall store the bytes written in bytes_written (if non-NULL) and return SOCKET_SEND_OK. ]
            if (send_result == 0)
            {
#ifdef ENABLE_SOCKET_LOGGING
                LogVerbose("Send completed synchronously at %lf", timer_global_get_elapsed_us());
#endif
                if (bytes_sent != NULL)
                {
                    *bytes_sent = num_bytes;
                }
                result = SOCKET_SEND_OK;
            }
            // Codes_SOCKET_TRANSPORT_WIN32_09_039: [ Otherwise socket_transport_send shall return SOCKET_SEND_FAILED. ]
            else
            {
                LogLastError("Failure sending socket data: buffer: %p, length: %" PRIu32 "", payload->buffer, payload->length);
                result = SOCKET_SEND_FAILED;
            }

            // Codes_SOCKET_TRANSPORT_WIN32_09_040: [ socket_transport_send shall call sm_exec_end. ]
            sm_exec_end(socket_transport->sm);
        }
    }
    return result;
}

SOCKET_RECEIVE_RESULT socket_transport_receive(SOCKET_TRANSPORT_HANDLE socket_transport, SOCKET_BUFFER* payload, uint32_t buffer_count, uint32_t* bytes_recv, uint32_t flags, void* data)
{
    SOCKET_RECEIVE_RESULT result;
    // Codes_SOCKET_TRANSPORT_WIN32_09_041: [ If socket_transport is NULL, socket_transport_receive shall fail and return SOCKET_RECEIVE_ERROR. ]
    if (socket_transport == NULL ||
        // Codes_SOCKET_TRANSPORT_WIN32_09_042: [ If payload is NULL, socket_transport_receive shall fail and return SOCKET_RECEIVE_ERROR. ]
        payload == NULL ||
        // Codes_SOCKET_TRANSPORT_WIN32_09_043: [ If buffer_count is 0, socket_transport_receive shall fail and return SOCKET_RECEIVE_ERROR. ]
        buffer_count == 0)
    {
        LogError("Invalid arguments: SOCKET_TRANSPORT_HANDLE socket_transport=%p, const SOCKET_BUFFER* payload=%p, uint32_t flags=%" PRIu32 ", void*, data=%p",
            socket_transport, payload, flags, data);
        result = SOCKET_RECEIVE_INVALID_ARG;
    }
    else
    {
        // Codes_SOCKET_TRANSPORT_WIN32_09_044: [ socket_transport_receive shall call sm_exec_begin. ]
        SM_RESULT sm_result = sm_exec_begin(socket_transport->sm);

        // Codes_SOCKET_TRANSPORT_WIN32_09_045: [ If sm_exec_begin does not return SM_EXEC_GRANTED, socket_transport_receive shall fail and return SOCKET_RECEIVE_ERROR. ]
        if (sm_result != SM_EXEC_GRANTED)
        {
            LogError("sm_exec_begin failed : %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, sm_result));
            result = SOCKET_RECEIVE_ERROR;
        }
        else
        {
            DWORD num_bytes = 0;
            LPDWORD num_bytes_param = NULL;
            if (data == NULL)
            {
                num_bytes_param = &num_bytes;
            }

            // Codes_SOCKET_TRANSPORT_WIN32_09_046: [ socket_transport_receive shall call WSARecv with the payload, flags and the data which is used as overlapped object. ]
            int recv_result = WSARecv(socket_transport->socket, (LPWSABUF)payload, buffer_count, num_bytes_param, (LPDWORD)&flags, (LPWSAOVERLAPPED)data, NULL);

            // Codes_SOCKET_TRANSPORT_WIN32_09_047: [ If WSARecv return 0, socket_transport_receive shall do the following: ]
            if (recv_result == 0)
            {
                // Codes_SOCKET_TRANSPORT_WIN32_09_048: [ If bytes_recv is not NULL, socket_transport_receive shall copy the number of bytes into bytes_recv. ]
                // Success
                if (bytes_recv != NULL)
                {
                    *bytes_recv = num_bytes;
                }

                // Codes_SOCKET_TRANSPORT_WIN32_09_049: [ socket_transport_receive shall return SOCKET_RECEIVE_OK. ]
                result = SOCKET_RECEIVE_OK;
            }
            // Codes_SOCKET_TRANSPORT_WIN32_09_050: [ If WSARecv returns an non-zero value, socket_transport_receive shall do the following: ]
            else
            {
                int wsa_last_error = WSAGetLastError();

                // Codes_SOCKET_TRANSPORT_WIN32_09_051: [ If WSAGetLastError returns WSA_IO_PENDING, and bytes_recv is not NULL, socket_transport_receive shall set bytes_recv to 0. ]
                if (WSA_IO_PENDING == wsa_last_error)
                {
                    if (bytes_recv != NULL)
                    {
                        *bytes_recv = 0;
                    }
                    // Codes_SOCKET_TRANSPORT_WIN32_09_052: [ socket_transport_receive shall return SOCKET_RECEIVE_WOULD_BLOCK. ]
                    result = SOCKET_RECEIVE_WOULD_BLOCK;
                }
                else
                {
                    // Codes_SOCKET_TRANSPORT_WIN32_09_053: [ If WSAGetLastError does not returns WSA_IO_PENDING socket_transport_receive shall return SOCKET_RECEIVE_ERROR. ]
                    LogLastError("WSARecv failed with %d, WSAGetLastError returned %lu", recv_result, wsa_last_error);
                    result = SOCKET_RECEIVE_ERROR;
                }
            }
            // Codes_SOCKET_TRANSPORT_WIN32_09_054: [ socket_transport_receive shall call sm_exec_end. ]
            sm_exec_end(socket_transport->sm);
        }
    }
    return result;
}

int socket_transport_listen(SOCKET_TRANSPORT_HANDLE socket_transport, uint16_t port)
{
    int result;

    // Codes_SOCKET_TRANSPORT_WIN32_09_055: [ If socket_transport is NULL, socket_transport_listen shall fail and return a non-zero value. ]
    // Codes_SOCKET_TRANSPORT_WIN32_09_056: [ If port is 0, socket_transport_listen shall fail and return a non-zero value. ]
    if (socket_transport == NULL ||
        port == 0)
    {
        LogError("Invalid arguments: SOCKET_TRANSPORT_HANDLE socket_transport: %p, uint16_t port: %" PRIu16 "",
            socket_transport, port);
        result = MU_FAILURE;
    }
    else
    {
        // Codes_SOCKET_TRANSPORT_WIN32_09_057: [ If the transport type is not SOCKET_BINDING, socket_transport_listen shall fail and return a non-zero value. ]
        if (socket_transport->type != SOCKET_BINDING)
        {
            LogError("Invalid socket type for this API expected: SOCKET_BINDING, actual: %" PRI_MU_ENUM, MU_ENUM_VALUE(SOCKET_TYPE, socket_transport->type));
            result = MU_FAILURE;
        }
        else
        {
            // Codes_SOCKET_TRANSPORT_WIN32_09_058: [ socket_transport_listen shall call sm_open_begin to begin the open. ]
            SM_RESULT open_result = sm_open_begin(socket_transport->sm);

            // Codes_SOCKET_TRANSPORT_WIN32_09_059: [ If sm_open_begin does not return SM_EXEC_GRANTED, socket_transport_listen shall fail and return a non-zero value. ]
            if (open_result != SM_EXEC_GRANTED)
            {
                LogError("sm_open_begin failed with %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, open_result));
                result = MU_FAILURE;
            }
            else
            {
                // Codes_SOCKET_TRANSPORT_WIN32_09_060: [ socket_transport_listen shall call socket with the params AF_INET, SOCK_STREAM and IPPROTO_TCP. ]
                socket_transport->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                if (socket_transport->socket == INVALID_SOCKET)
                {
                    LogLastError("Could not create socket");
                    result = MU_FAILURE;
                }
                else
                {
                    u_long iMode = 1;
                    struct sockaddr_in service;

                    service.sin_family = AF_INET;
                    service.sin_addr.s_addr = INADDR_ANY;
                    service.sin_port = htons(port);

                    // Codes_SOCKET_TRANSPORT_WIN32_09_061: [ socket_transport_listen shall bind to the socket by calling bind. ]
                    if (bind(socket_transport->socket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR)
                    {
                        LogLastError("Could not bind socket, port=%" PRIu16 "", port);
                        result = MU_FAILURE;
                    }

                    // Codes_SOCKET_TRANSPORT_WIN32_09_065: [ sock_transport_listen shall set listening socket in non-blocking mode by calling ioctlsocket. ]
                    else if (ioctlsocket(socket_transport->socket, FIONBIO, &iMode) != 0)
                    {
                        LogLastError("Could not set listening socket in non-blocking mode");
                        result = MU_FAILURE;
                    }
                    else
                    {
                        // Codes_SOCKET_TRANSPORT_WIN32_09_062: [ socket_transport_listen shall start listening to incoming connection by calling listen. ]
                        if (listen(socket_transport->socket, SOMAXCONN) == SOCKET_ERROR)
                        {
                            LogLastError("Could not start listening for connections");
                            result = MU_FAILURE;
                        }
                        else
                        {
                            // Codes_SOCKET_TRANSPORT_WIN32_09_063: [ If successful socket_transport_listen shall call sm_open_end with true. ]
                            sm_open_end(socket_transport->sm, true);
                            result = 0;
                            goto all_ok;
                        }
                    }
                    // Codes_SOCKET_TRANSPORT_WIN32_09_066: [ socket_transport_listen shall call closesocket. ]
                    (void)closesocket(socket_transport->socket);

                    // Codes_SOCKET_TRANSPORT_WIN32_09_064: [ If any failure is encountered, socket_transport_listen shall call sm_open_end with false, fail and return a non-zero value. ]
                    sm_open_end(socket_transport->sm, false);
                }
            }
        }
    }
all_ok:
    return result;
}

SOCKET_ACCEPT_RESULT socket_transport_accept(SOCKET_TRANSPORT_HANDLE socket_transport, SOCKET_TRANSPORT_HANDLE* accepted_socket)
{
    SOCKET_TRANSPORT* accept_result;
    SOCKET_ACCEPT_RESULT result;

    // Codes_SOCKET_TRANSPORT_WIN32_09_067: [ If socket_transport is NULL, socket_transport_accept shall fail and return SOCKET_ACCEPT_ERROR. ]
    if (socket_transport == NULL)
    {
        LogError("Invalid arguments: SOCKET_TRANSPORT_HANDLE socket_transport: %p",
            socket_transport);
        result = SOCKET_ACCEPT_ERROR;
    }
    else
    {
        // Codes_SOCKET_TRANSPORT_WIN32_09_068: [ If the transport type is not SOCKET_BINDING, socket_transport_accept shall fail and return SOCKET_ACCEPT_ERROR. ]
        if (socket_transport->type != SOCKET_BINDING)
        {
            LogError("Invalid socket type for this API expected: SOCKET_BINDING, actual: %" PRI_MU_ENUM, MU_ENUM_VALUE(SOCKET_TYPE, socket_transport->type));
            result = SOCKET_ACCEPT_ERROR;
        }
        else
        {
            // Codes_SOCKET_TRANSPORT_WIN32_09_069: [ socket_transport_accept shall call sm_exec_begin. ]
            SM_RESULT sm_result = sm_exec_begin(socket_transport->sm);

            // Codes_SOCKET_TRANSPORT_WIN32_09_070: [ If sm_exec_begin does not return SM_EXEC_GRANTED, socket_transport_accept shall fail and return SOCKET_ACCEPT_ERROR. ]
            if (sm_result != SM_EXEC_GRANTED)
            {
                LogError("sm_exec_begin failed : %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, sm_result));
                result = SOCKET_ACCEPT_ERROR;
            }
            else
            {
                fd_set read_fds;
                int select_result;
                struct timeval timeout;
                bool is_error = false;

                read_fds.fd_array[0] = socket_transport->socket;
                read_fds.fd_count = 1;
                timeout.tv_usec = 1000 * 100;
                timeout.tv_sec = 0;

                // Codes_SOCKET_TRANSPORT_WIN32_09_071: [ socket_transport_accept shall call select determine if the socket is ready to be read passing a timeout of 10 milliseconds. ]
                select_result = select(0, &read_fds, NULL, NULL, &timeout);
                if (select_result == SOCKET_ERROR)
                {
                    LogLastError("Error waiting for socket connections %", select_result);
                    is_error = true;
                    result = SOCKET_ACCEPT_ERROR;
                }
                else if (select_result > 0)
                {
                    struct sockaddr_in cli_addr;
                    socklen_t client_len = sizeof(cli_addr);

                    // Codes_SOCKET_TRANSPORT_WIN32_09_072: [ socket_transport_accept shall call accept to accept the incoming socket connection. ]
                    SOCKET accepting_socket = accept(socket_transport->socket, (struct sockaddr*)&cli_addr, &client_len);

                    // Codes_SOCKET_TRANSPORT_WIN32_09_073: [ If accept returns an INVALID_SOCKET, socket_transport_accept shall fail and return SOCKET_ACCEPT_ERROR. ]
                    if (accepting_socket == INVALID_SOCKET)
                    {
                        if (WSAGetLastError() != WSAEWOULDBLOCK)
                        {
                            LogLastError("Error in accepting socket %" PRI_SOCKET "", accepted_socket);
                            is_error = true;
                        }
                        result = SOCKET_ACCEPT_ERROR;
                    }
                    else
                    {
                        // Codes_SOCKET_TRANSPORT_WIN32_09_074: [ socket_transport_accept shall allocate a SOCKET_TRANSPORT for the incoming connection and call sm_create and sm_open on the connection. ]
                        char hostname_addr[256];
                        (void)inet_ntop(AF_INET, (const void*)&cli_addr.sin_addr, hostname_addr, sizeof(hostname_addr));

                        // Create the socket handle
                        // Codes_SOCKET_TRANSPORT_WIN32_09_084: [ If malloc fails, socket_transport_accept shall fail and return SOCKET_ACCEPT_ERROR. ]
                        accept_result = malloc(sizeof(SOCKET_TRANSPORT));
                        if (accept_result == NULL)
                        {
                            LogError("failure allocating SOCKET_TRANSPORT: %zu", sizeof(SOCKET_TRANSPORT));
                            result = SOCKET_ACCEPT_ERROR;
                        }
                        else
                        {
                            // Codes_SOCKET_TRANSPORT_WIN32_09_085: [ If sm_create fails, socket_transport_accept shall close the incoming socket, fail, and return SOCKET_ACCEPT_ERROR. ]
                            accept_result->sm = sm_create("Socket_transport_win32");
                            if (accept_result->sm == NULL)
                            {
                                LogError("Failed calling sm_create in accept, closing incoming socket.");
                                closesocket(accepting_socket);
                                free(accept_result);
                                result = SOCKET_ACCEPT_ERROR;
                            }
                            else
                            {
                                // Codes_SOCKET_TRANSPORT_WIN32_09_086: [ If sm_open_begin fails, socket_transport_accept shall close the incoming socket, fail, and return SOCKET_ACCEPT_ERROR ]
                                SM_RESULT open_result = sm_open_begin(accept_result->sm);
                                if (open_result == SM_EXEC_GRANTED)
                                {
                                    accept_result->type = SOCKET_CLIENT;
                                    accept_result->socket = accepting_socket;
                                    sm_open_end(accept_result->sm, true);
                                    *accepted_socket = accept_result;
                                    result = SOCKET_ACCEPT_OK;
                                }
                                else
                                {
                                    LogError("sm_open_begin failed with %" PRI_MU_ENUM " in accept, closing incoming socket.", MU_ENUM_VALUE(SM_RESULT, open_result));
                                    closesocket(accepting_socket);
                                    sm_destroy(accept_result->sm);
                                    free(accept_result);
                                    result = SOCKET_ACCEPT_ERROR;
                                }
                            }
                        }
                    }
                }
                // Codes_SOCKET_TRANSPORT_WIN32_09_091: [ If select returns zero, socket_transport_accept shall set accepted_socket to NULL and return SOCKET_ACCEPT_NO_CONNECTION. ]
                else if (select_result == 0)
                {
                    result = SOCKET_ACCEPT_NO_CONNECTION;
                }
                else
                {
                    // Codes_SOCKET_TRANSPORT_WIN32_09_076: [ If any failure is encountered, socket_transport_accept shall fail and return SOCKET_ACCEPT_ERROR. ]
                    LogLastError("Failure accepting socket connection");
                    is_error = true;
                    result = SOCKET_ACCEPT_ERROR;
                }
                // Codes_SOCKET_TRANSPORT_WIN32_09_077: [ socket_transport_accept shall call sm_exec_end. ]
                sm_exec_end(socket_transport->sm);
            }
        }
    }

    // Codes_SOCKET_TRANSPORT_WIN32_09_075: [ If successful socket_transport_accept shall return the allocated SOCKET_TRANSPORT. ]
    return result;
}

SOCKET_HANDLE socket_transport_get_underlying_socket(SOCKET_TRANSPORT_HANDLE socket_transport)
{
    SOCKET_HANDLE result;

    // Codes_SOCKET_TRANSPORT_WIN32_09_078: [ If socket_transport is NULL, socket_transport_get_underlying_socket shall fail and return INVALID_SOCKET. ]
    if (socket_transport == NULL)
    {
        LogError("Invalid arguments: SOCKET_TRANSPORT_HANDLE socket_transport: %p",
            socket_transport);
        result = (SOCKET_HANDLE)INVALID_SOCKET;
    }
    else
    {
        // Codes_SOCKET_TRANSPORT_WIN32_09_079: [ socket_transport_get_underlying_socket shall call sm_exec_begin. ]
        SM_RESULT sm_result = sm_exec_begin(socket_transport->sm);

        // Codes_SOCKET_TRANSPORT_WIN32_09_080: [ If sm_exec_begin does not return SM_EXEC_GRANTED, socket_transport_get_underlying_socket shall fail and return INVALID_SOCKET. ]
        if (sm_result != SM_EXEC_GRANTED)
        {
            LogError("sm_exec_begin failed : %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, sm_result));
            result = (SOCKET_HANDLE)INVALID_SOCKET;
        }
        else
        {
            // Codes_SOCKET_TRANSPORT_WIN32_09_081: [ socket_transport_get_underlying_socket shall return the SOCKET_TRANSPORT socket value. ]
            result = (SOCKET_HANDLE)socket_transport->socket;

            // Codes_SOCKET_TRANSPORT_WIN32_09_082: [ socket_transport_get_underlying_socket shall call sm_exec_end. ]
            sm_exec_end(socket_transport->sm);
        }
    }
    return result;
}
