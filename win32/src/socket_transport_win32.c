// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>
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

#include "c_pal/socket_transport.h"

#define SOCKET_IO_TYPE_VALUES \
    SOCKET_IO_TYPE_SEND, \
    SOCKET_IO_TYPE_RECEIVE

MU_DEFINE_ENUM(SOCKET_IO_TYPE, SOCKET_IO_TYPE_VALUES)
MU_DEFINE_ENUM_STRINGS(SOCKET_IO_TYPE, SOCKET_IO_TYPE_VALUES)

MU_DEFINE_ENUM_STRINGS(SOCKET_SEND_RESULT, SOCKET_SEND_RESULT_VALUES)
MU_DEFINE_ENUM_STRINGS(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_RESULT_VALUES)
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
    if (connect(client_socket, addrInfo->ai_addr, (int)addrInfo->ai_addrlen) != 0)
    {
        if (WSAGetLastError() == WSAEWOULDBLOCK)
        {
            fd_set write_fds;
            FD_ZERO(&write_fds);
            FD_SET(client_socket, &write_fds);

            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = connection_timeout_ms * 100;

            int ret = select(0, NULL, &write_fds, NULL, &tv);
            if (ret == SOCKET_ERROR)
            {
                LogError("Failure connecting error: %d", WSAGetLastError());
                result = MU_FAILURE;
            }
            else if (ret == 0)
            {
                LogError("Failure timeout (%" PRId32 " us) attempting to connect", connection_timeout_ms);
                result = MU_FAILURE;
            }
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
        else
        {
            LogLastError("Winsock connect failure");
            result = MU_FAILURE;
        }
    }
    else
    {
        result = 0;
    }
    return result;
}

static SOCKET connect_to_client(const char* hostname, uint16_t port, uint32_t connection_timeout)
{
    SOCKET result;
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

SOCKET_TRANSPORT_HANDLE socket_transport_create(SOCKET_TYPE type)
{
    SOCKET_TRANSPORT* result;
    result = malloc(sizeof(SOCKET_TRANSPORT));
    if (result == NULL)
    {
        LogError("failure allocating SOCKET_TRANSPORT: %zu", sizeof(SOCKET_TRANSPORT));
    }
    else
    {
        result->sm = sm_create("Socket_transport_win32");
        if (result->sm == NULL)
        {
            LogError("sm_create failed.");
        }
        else
        {
            result->type = type;
            goto all_ok;
        }
        free(result);
        result = NULL;
    }
all_ok:
    return result;
}

void socket_transport_destroy(SOCKET_TRANSPORT_HANDLE socket_transport)
{
    if (socket_transport == NULL)
    {
        // Do nothing
    }
    else
    {
        sm_destroy(socket_transport->sm);
        free(socket_transport);
    }
}

int socket_transport_connect(SOCKET_TRANSPORT_HANDLE socket_transport, const char* hostname, uint16_t port, uint32_t connection_timeout)
{
    int result;
    if (socket_transport == NULL ||
        hostname == NULL ||
        port == 0)
    {
        LogError("Invalid arguments: SOCKET_TRANSPORT_HANDLE socket_transport: %p, const char* hostname: %s, uint16_t port: %" PRIu16 ", uint32_t connection_timeout: %" PRIu32 "",
            socket_transport, MU_P_OR_NULL(hostname), port, connection_timeout);
        result = MU_FAILURE;
    }
    else
    {
        if (socket_transport->type != SOCKET_CLIENT)
        {
            LogError("Invalid socket type for this API expected: SOCKET_CLIENT, actual: %" PRI_MU_ENUM, MU_ENUM_VALUE(SOCKET_TYPE, socket_transport->type));
            result = MU_FAILURE;
        }
        else
        {
            SM_RESULT open_result = sm_open_begin(socket_transport->sm);
            if (open_result != SM_EXEC_GRANTED)
            {
                LogError("sm_open_begin failed with %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, open_result));
                result = MU_FAILURE;
            }
            else
            {
                socket_transport->socket = connect_to_client(hostname, port, connection_timeout);
                if (socket_transport->socket == INVALID_SOCKET)
                {
                    LogError("Failure conneting to client hostname: %s:%" PRIu16 "", hostname, port);
                    result = MU_FAILURE;
                    sm_open_end(socket_transport->sm, false);
                }
                else
                {
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
    if (socket_transport == NULL)
    {
        LogError("Invalid arguments: SOCKET_TRANSPORT_HANDLE socket_transport: %p",
            socket_transport);
    }
    else
    {
        SM_RESULT close_result = sm_close_begin(socket_transport->sm);
        if (close_result == SM_EXEC_GRANTED)
        {
            if (closesocket(socket_transport->socket) != 0)
            {
                LogLastError("Failure in closesocket");
            }
            sm_close_end(socket_transport->sm);
        }
        else
        {
            LogError("sm_close_begin failed with %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, close_result));
        }
    }
}

SOCKET_SEND_RESULT socket_transport_send(SOCKET_TRANSPORT_HANDLE socket_transport, const SOCKET_BUFFER* payload, uint32_t buffer_count, uint32_t* bytes_sent, uint32_t flags, void* overlapped_data)
{
    SOCKET_SEND_RESULT result;
    if (socket_transport == NULL ||
        payload == NULL ||
        buffer_count == 0)
    {
        LogError("Invalid arguments: SOCKET_TRANSPORT_HANDLE socket_transport: %p, const SOCKET_BUFFER* payload: %p, void* overlapped_data: %p",
            socket_transport, payload, overlapped_data);
        result = SOCKET_SEND_ERROR;
    }
    else
    {
        SM_RESULT sm_result = sm_exec_begin(socket_transport->sm);
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
            send_result = WSASend(socket_transport->socket, (LPWSABUF)payload, buffer_count, num_bytes_param, flags, (LPWSAOVERLAPPED)overlapped_data, NULL);
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
            else
            {
                LogLastError("Failure sending socket data: buffer: %p, length: %" PRIu32 "", payload->buffer, payload->length);
                result = SOCKET_SEND_FAILED;
            }
            sm_exec_end(socket_transport->sm);
        }
    }
    return result;
}

SOCKET_RECEIVE_RESULT socket_transport_receive(SOCKET_TRANSPORT_HANDLE socket_transport, SOCKET_BUFFER* payload, uint32_t buffer_count, uint32_t* bytes_recv, uint32_t flags, void* data)
{
    SOCKET_RECEIVE_RESULT result;
    if (socket_transport == NULL ||
        payload == NULL)
    {
        LogError("Invalid arguments: SOCKET_TRANSPORT_HANDLE socket_transport=%p, const SOCKET_BUFFER* payload=%p, uint32_t flags=%" PRIu32 ", void*, data=%p",
            socket_transport, payload, flags, data);
        result = SOCKET_RECEIVE_ERROR;
    }
    else
    {
        SM_RESULT sm_result = sm_exec_begin(socket_transport->sm);
        if (sm_result != SM_EXEC_GRANTED)
        {
            LogError("sm_exec_begin failed : %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, sm_result));
            result = MU_FAILURE;
        }
        else
        {
            DWORD num_bytes = 0;
            LPDWORD num_bytes_param = NULL;
            if (data == NULL)
            {
                num_bytes_param = &num_bytes;
            }

            int recv_result = WSARecv(socket_transport->socket, (LPWSABUF)payload, buffer_count, num_bytes_param, (LPDWORD)&flags, (LPWSAOVERLAPPED)data, NULL);
            if (recv_result == 0)
            {
                // Success
                if (bytes_recv != NULL)
                {
                    *bytes_recv = num_bytes;
                }
                result = SOCKET_RECEIVE_OK;
            }
            else
            {
                int wsa_last_error = WSAGetLastError();
                if (WSA_IO_PENDING == wsa_last_error)
                {
                    if (bytes_recv != NULL)
                    {
                        *bytes_recv = 0;
                    }
                    result = SOCKET_RECEIVE_WOULD_BLOCK;
                }
                else
                {
                    LogLastError("WSARecv failed with %d, WSAGetLastError returned %lu", recv_result, wsa_last_error);
                    result = SOCKET_RECEIVE_ERROR;
                }
            }
            sm_exec_end(socket_transport->sm);
        }
    }
    return result;
}

int socket_transport_listen(SOCKET_TRANSPORT_HANDLE socket_transport, uint16_t port)
{
    int result;
    if (socket_transport == NULL ||
        port == 0)
    {
        LogError("Invalid arguments: SOCKET_TRANSPORT_HANDLE socket_transport: %p, uint16_t port: %" PRIu16 "",
            socket_transport, port);
        result = MU_FAILURE;
    }
    else
    {
        if (socket_transport->type != SOCKET_SERVER)
        {
            LogError("Invalid socket type for this API expected: SOCKET_SERVER, actual: %" PRI_MU_ENUM, MU_ENUM_VALUE(SOCKET_TYPE, socket_transport->type));
            result = MU_FAILURE;
        }
        else
        {
            SM_RESULT open_result = sm_open_begin(socket_transport->sm);
            if (open_result != SM_EXEC_GRANTED)
            {
                LogError("sm_open_begin failed with %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, open_result));
                result = MU_FAILURE;
            }
            else
            {
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

                    if (bind(socket_transport->socket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR)
                    {
                        LogLastError("Could not bind socket, port=%" PRIu16 "", port);
                        result = MU_FAILURE;
                    }
                    else if (ioctlsocket(socket_transport->socket, FIONBIO, &iMode) != 0)
                    {
                        LogLastError("Could not set listening socket in non-blocking mode");
                        result = MU_FAILURE;
                    }
                    else
                    {
                        if (listen(socket_transport->socket, SOMAXCONN) == SOCKET_ERROR)
                        {
                            LogLastError("Could not start listening for connections");
                            result = MU_FAILURE;
                        }
                        else
                        {
                            sm_open_end(socket_transport->sm, true);
                            result = 0;
                            goto all_ok;
                        }
                    }
                    closesocket(socket_transport->socket);
                    sm_open_end(socket_transport->sm, false);
                }
            }
        }
    }
all_ok:
    return result;
}

SOCKET_TRANSPORT_HANDLE socket_transport_accept(SOCKET_TRANSPORT_HANDLE socket_transport)
{
    SOCKET_TRANSPORT* result;
    if (socket_transport == NULL)
    {
        LogError("Invalid arguments: SOCKET_TRANSPORT_HANDLE socket_transport: %p",
            socket_transport);
        result = NULL;
    }
    else
    {
        if (socket_transport->type != SOCKET_SERVER)
        {
            LogError("Invalid socket type for this API expected: SOCKET_SERVER, actual: %" PRI_MU_ENUM, MU_ENUM_VALUE(SOCKET_TYPE, socket_transport->type));
            result = NULL;
        }
        else
        {
            SM_RESULT sm_result = sm_exec_begin(socket_transport->sm);
            if (sm_result != SM_EXEC_GRANTED)
            {
                LogError("sm_exec_begin failed : %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, sm_result));
                result = NULL;
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

                select_result = select(0, &read_fds, NULL, NULL, &timeout);
                if (select_result == SOCKET_ERROR)
                {
                    LogLastError("Error waiting for socket connections");
                    is_error = true;
                    result = NULL;
                }
                else if (select_result > 0)
                {
                    struct sockaddr_in cli_addr;
                    socklen_t client_len = sizeof(cli_addr);

                    SOCKET accepted_socket = accept(socket_transport->socket, (struct sockaddr*)&cli_addr, &client_len);
                    if (accepted_socket == INVALID_SOCKET)
                    {
                        if (WSAGetLastError() != WSAEWOULDBLOCK)
                        {
                            LogLastError("Error accepting socket");
                            is_error = true;
                        }
                        result = NULL;
                    }
                    else
                    {
                        char hostname_addr[256];
                        (void)inet_ntop(AF_INET, (const void*)&cli_addr.sin_addr, hostname_addr, sizeof(hostname_addr));
                        LogError("Socket connected (%" PRI_SOCKET ") from %s:%d", (void*)accepted_socket, hostname_addr, cli_addr.sin_port);

                        // Create the socket handle
                        result = malloc(sizeof(SOCKET_TRANSPORT));
                        if (result == NULL)
                        {
                            LogError("failure allocating SOCKET_TRANSPORT: %zu", sizeof(SOCKET_TRANSPORT));
                        }
                        else
                        {
                            result->sm = sm_create("Socket_transport_win32");
                            if (result->sm == NULL)
                            {
                                LogError("Failed calling sm_create in accept, closing incoming socket.");
                                closesocket(accepted_socket);
                                free(result);
                                result = NULL;
                            }
                            else
                            {
                                SM_RESULT open_result = sm_open_begin(result->sm);
                                if (open_result == SM_EXEC_GRANTED)
                                {
                                    result->type = SOCKET_SERVER;
                                    result->socket = accepted_socket;
                                    sm_open_end(result->sm, true);
                                }
                                else
                                {
                                    LogError("sm_open_begin failed with %" PRI_MU_ENUM " in accept, closing incoming socket.", MU_ENUM_VALUE(SM_RESULT, open_result));
                                    closesocket(accepted_socket);
                                    sm_destroy(result->sm);
                                    free(result);
                                    result = NULL;
                                }
                            }
                        }
                    }
                }
                else
                {
                    LogLastError("Failure accepting socket connection");
                    is_error = true;
                    result = NULL;
                }
                sm_exec_end(socket_transport->sm);
            }
        }
    }
    return result;
}

SOCKET_HANDLE socket_transport_get_underlying_socket(SOCKET_TRANSPORT_HANDLE socket_transport)
{
    SOCKET_HANDLE result;
    if (socket_transport == NULL)
    {
        LogError("Invalid arguments: SOCKET_TRANSPORT_HANDLE socket_transport: %p",
            socket_transport);
        result = (SOCKET_HANDLE)INVALID_SOCKET;
    }
    else
    {
        SM_RESULT sm_result = sm_exec_begin(socket_transport->sm);
        if (sm_result != SM_EXEC_GRANTED)
        {
            LogError("sm_exec_begin failed : %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, sm_result));
            result = (SOCKET_HANDLE)INVALID_SOCKET;
        }
        else
        {
            result = (SOCKET_HANDLE)socket_transport->socket;
            sm_exec_end(socket_transport->sm);
        }
    }
    return result;
}
