// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>
#include <inttypes.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>

#include "macro_utils/macro_utils.h"

#include "umock_c/umock_c_prod.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/interlocked.h"
#include "c_pal/sm.h"
#include "c_pal/sync.h"
#include "c_pal/string_utils.h"
#include "c_pal/socket_handle.h"

#include "c_pal/socket_transport.h"

#define SOCKET_IO_TYPE_VALUES \
    SOCKET_IO_TYPE_SEND, \
    SOCKET_IO_TYPE_RECEIVE

MU_DEFINE_ENUM(SOCKET_IO_TYPE, SOCKET_IO_TYPE_VALUES)
MU_DEFINE_ENUM_STRINGS(SOCKET_IO_TYPE, SOCKET_IO_TYPE_VALUES)

typedef struct SOCKET_TRANSPORT_TAG
{
    SOCKET_HANDLE socket;
    SM_HANDLE sm;
    SOCKET_TYPE type;
} SOCKET_TRANSPORT;

static int set_nonblocking(SOCKET_HANDLE socket)
{
    int result;
    int opts = fcntl(socket, F_GETFL);
    if (opts < 0)
    {
        LogErrorNo("Failure getting socket option.");
        result = MU_FAILURE;
    }
    else
    {
        if ((opts = fcntl(socket, F_SETFL, opts|O_NONBLOCK)) < 0)
        {
            LogErrorNo("Failure setting socket option.");
            result = MU_FAILURE;
        }
        else
        {
            result = 0;
        }
    }
    return result;
}

static SOCKET_HANDLE connect_to_client(const char* hostname, uint16_t port, uint32_t connection_timeout)
{
    SOCKET_HANDLE result;
    SOCKET_HANDLE client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET)
    {
        LogErrorNo("Failure: socket create failure.");
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
            struct addrinfo addr_hint = { 0 };
            struct addrinfo* addrInfo = NULL;

            addr_hint.ai_family = AF_INET;
            addr_hint.ai_socktype = SOCK_STREAM;
            addr_hint.ai_protocol = 0;
            addr_hint.ai_flags = AI_CANONNAME;

            if (getaddrinfo(hostname, port_string, &addr_hint, &addrInfo) != 0)
            {
                LogErrorNo("Failure: getaddrinfo(hostname=%s, portString=%s, &addr_hint=%p, &addrInfo=%p)", hostname, port_string, &addr_hint, &addrInfo);
                result = INVALID_SOCKET;
            }
            else
            {
                LogInfo("Connecting to %s:%" PRIu16 " Machine Name: %s, connection timeout: %" PRIu32 "", hostname, port, MU_P_OR_NULL(addrInfo->ai_canonname), connection_timeout);

                if (connect(client_socket, addrInfo->ai_addr, addrInfo->ai_addrlen) != 0)
                {
                    LogErrorNo("Connection failure. Server: %s:%" PRIu16 ".", hostname, port);
                    result = INVALID_SOCKET;
                }
                else if (set_nonblocking(client_socket) != 0)
                {
                    LogError("Failure: nonblocking failure.");
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
        close(client_socket);
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

void socket_transport_destroy(SOCKET_TRANSPORT_HANDLE socket_client)
{
    if (socket_client == NULL)
    {
        // Do nothing
    }
    else
    {
        sm_destroy(socket_transport->sm);
        free(socket_client);
    }
}

int socket_transport_connect(SOCKET_TRANSPORT_HANDLE socket_client, const char* hostname, uint16_t port, uint32_t connection_timeout)
{
    int result;
    if (socket_client == NULL ||
        hostname == NULL)
    {
        LogError("Invalid arguments: SOCKET_TRANSPORT_HANDLE socket_client: %p, const char* hostname: %s, uint16_t port: %" PRIu16 ", uint32_t connection_timeout: %" PRIu32 "",
            socket_client, MU_P_OR_NULL(hostname), port, connection_timeout);
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
            socket_client->socket = connect_to_client(hostname, port, connection_timeout);
            if (socket_client->socket == INVALID_SOCKET)
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
    return result;
}

void socket_transport_disconnect(SOCKET_TRANSPORT_HANDLE socket_client)
{
    if (socket_client == NULL)
    {
        LogError("Invalid arguments: SOCKET_TRANSPORT_HANDLE socket_client: %p",
            socket_client);
    }
    else
    {
        SM_RESULT close_result = sm_close_begin(socket_transport->sm);
        if (close_result == SM_EXEC_GRANTED)
        {
            close(socket_client->socket);
            sm_close_end(socket_transport->sm);
        }
        else
        {
            LogError("sm_close_begin failed with %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, close_result));
        }
    }
}

SOCKET_SEND_RESULT socket_transport_send(SOCKET_TRANSPORT_HANDLE socket_client, SOCKET_BUFFER* payload, uint32_t buffer_count, uint32_t* bytes_written, uint32_t flags, void* data)
{
    SOCKET_SEND_RESULT result;
    if (socket_client == NULL ||
        payload == NULL)
    {
        LogError("Invalid arguments: SOCKET_TRANSPORT_HANDLE socket_client: %p, const SOCKET_BUFFER* payload: %p, void* data: %p",
            socket_client, payload, data);
        result = SOCKET_SEND_ERROR;
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
            uint32_t total_send_size = 0;
            for (uint32_t index = 0; index < buffer_count; index++)
            {
                ssize_t data_sent = 0;
                do
                {
                    ssize_t send_size = send(socket_client->socket, payload[index].buffer, payload[index].length, flags);
                    if (send_size < 0)
                    {
                        // Log below where we get the errno
                        result = SOCKET_SEND_ERROR;
                        break;
                    }
                    else
                    {
                        result = SOCKET_SEND_OK;
                        data_sent += send_size;
                    }
                } while (data_sent < payload[index].length);

                if (result == SOCKET_SEND_OK)
                {
    #ifdef ENABLE_SOCKET_LOGGING
                    LogVerbose("Send completed synchronously at %lf", timer_global_get_elapsed_us());
    #endif
                    total_send_size += data_sent;
                }
                else
                {
                    if (errno == ECONNRESET)
                    {
                        // todo: maybe we need to return a known value
                        result = SOCKET_SEND_SHUTDOWN;
                        LogError("A reset on the send socket has been encountered");
                    }
                    else
                    {
                        result = SOCKET_SEND_ERROR;
                        LogErrorNo("Failure sending socket data: buffer: %p, length: %" PRIu32 "", payload->buffer, payload->length);
                    }
                }
            }
            if (bytes_written != NULL)
            {
                *bytes_written = total_send_size;
            }
            sm_exec_end(socket_transport->sm);
        }
    }
    return result;
}

SOCKET_RECEIVE_RESULT socket_transport_receive(SOCKET_TRANSPORT_HANDLE socket_client, SOCKET_BUFFER* payload, uint32_t buffer_count, uint32_t* bytes_recv, uint32_t flags, void* data)
{
    SOCKET_RECEIVE_RESULT result;
    if (socket_client == NULL ||
        payload == NULL)
    {
        LogError("Invalid arguments: SOCKET_TRANSPORT_HANDLE socket_client=%p, const SOCKET_BUFFER* payload=%p, uint32_t flags=%" PRIu32 ", void*, data=%p",
            socket_client, payload, flags, data);
        result = SOCKET_RECEIVE_ERROR;
    }
    else
    {
        SM_RESULT sm_result = sm_exec_begin(socket_transport->sm);
        if (sm_result != SM_EXEC_GRANTED)
        {
            LogError("sm_exec_begin failed : %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, sm_result));
            result = SOCKET_RECEIVE_ERROR;
        }
        else
        {
            uint32_t total_recv_size = 0;
            for (uint32_t index = 0; index < buffer_count; index++)
            {
                ssize_t recv_size = recv(socket_client->socket, payload[index].buffer, payload[index].length, 0);
                if (recv_size < 0)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        result = SOCKET_RECEIVE_WOULD_BLOCK;
                        break;
                    }
                    else
                    {
                        total_recv_size = 0;
                        recv_size = 0;
                        if (errno == ECONNRESET)
                        {
                            result = SOCKET_RECEIVE_SHUTDOWN;
                            LogError("A reset on the recv socket has been encountered");
                        }
                        else
                        {
                            result = SOCKET_RECEIVE_ERROR;
                            LogErrorNo("failure recv data");
                        }
                        break;
                    }
                }
                else if (recv_size == 0)
                {
                    LogError("Socket received 0 bytes, assuming socket is closed");
                    result = SOCKET_RECEIVE_SHUTDOWN;
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
                        result = SOCKET_RECEIVE_ERROR;
                    }
                    else
                    {
                        total_recv_size += recv_size;
                        if (recv_size <= payload[index].length)
                        {
#ifdef ENABLE_SOCKET_LOGGING
                            LogVerbose("Asynchronous receive of %" PRIu32 " bytes completed at %lf", bytes_received, timer_global_get_elapsed_us());
#endif
                            result = SOCKET_RECEIVE_OK;
                        }
                        else
                        {
                            // keep recieving data
                        }
                    }
                }
            }

            if (result == 0)
            {
                // Success
                if (bytes_recv != NULL)
                {
                    *bytes_recv = total_recv_size;
                }
            }
            sm_exec_end(socket_transport->sm);
        }
    }
    return result;
}

int socket_transport_listen(SOCKET_TRANSPORT_HANDLE socket_client, uint16_t port)
{
    int result;
    if (socket_client == NULL)
    {
        LogError("Invalid arguments: SOCKET_TRANSPORT_HANDLE socket_client: %p",
            socket_client);
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
            socket_client->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (socket_client->socket == INVALID_SOCKET)
            {
                LogErrorNo("Could not create socket");
                result = MU_FAILURE;
            }
            else
            {
                struct sockaddr_in service;

                service.sin_family = AF_INET;
                service.sin_addr.s_addr = htonl(INADDR_ANY);
                service.sin_port = htons(port);

                if (bind(socket_client->socket, (struct sockaddr*)&service, sizeof(service)) != 0)
                {
                    LogErrorNo("Could not bind socket, port=%" PRIu16 "", port);
                    result = MU_FAILURE;
                }
                else if (set_nonblocking(socket_client->socket) != 0)
                {
                    LogErrorNo("Could not set listening socket in non-blocking mode");
                    result = MU_FAILURE;
                }
                else
                {
                    if (listen(socket_client->socket, SOMAXCONN) != 0)
                    {
                        LogErrorNo("Could not start listening for connections");
                        result = MU_FAILURE;
                    }
                    else
                    {
                        sm_open_end(socket_transport->sm, true);
                        result = 0;
                        goto all_ok;
                    }
                }
                close(socket_client->socket);
                sm_open_end(socket_transport->sm, false);
            }
        }
    }
all_ok:
    return result;
}

SOCKET_TRANSPORT_HANDLE socket_transport_accept(SOCKET_TRANSPORT_HANDLE socket_client)
{
    SOCKET_TRANSPORT* result;
    if (socket_client == NULL)
    {
        LogError("Invalid arguments: SOCKET_TRANSPORT_HANDLE socket_client: %p", socket_client);
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
            struct sockaddr_in cli_addr;
            socklen_t client_len = sizeof(cli_addr);

            SOCKET_HANDLE accepted_socket;
            accepted_socket = accept(socket_client->socket, (struct sockaddr*)&cli_addr, &client_len);
            if (accepted_socket == INVALID_SOCKET)
            {
                LogErrorNo("Failure accepting socket");
                result = NULL;
            }
            else
            {
                if (set_nonblocking(accepted_socket) != 0)
                {
                    LogError("Failure: setting socket to nonblocking.");
                    result = NULL;
                }
                else
                {
                    char hostname_addr[256];
                    (void)inet_ntop(AF_INET, (const void*)&cli_addr.sin_addr, hostname_addr, sizeof(hostname_addr));
                    LogError("Socket connected (%" PRI_SOCKET ") from %s:%d", accepted_socket, hostname_addr, cli_addr.sin_port);

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
                        }
                        else
                        {
                            SM_RESULT open_result = sm_open_begin(result->sm);
                            if (open_result == SM_EXEC_GRANTED)
                            {
                                result->type = SOCKET_SERVER;
                                result->socket = accepted_socket;
                                sm_open_end(result->sm, true);
                                goto all_ok;
                            }
                            else
                            {
                                LogError("sm_open_begin failed with %" PRI_MU_ENUM " in accept, closing incoming socket.", MU_ENUM_VALUE(SM_RESULT, open_result));
                            }
                            sm_destroy(result->sm);
                        }
                        free(result);
                        result = NULL;
                    }
                }
                close(accepted_socket);
            }
        }
        result = NULL;
    }
all_ok:
    return result;
}

SOCKET_HANDLE socket_transport_get_underlying_socket(SOCKET_TRANSPORT_HANDLE socket_handle)
{
    SOCKET_HANDLE result;
    if (socket_handle == NULL)
    {
        LogError("Invalid arguments: SOCKET_TRANSPORT_HANDLE socket_handle: %p",
            socket_handle);
        result = INVALID_SOCKET;
    }
    else
    {
        result = socket_handle->socket;
    }
    return result;
}