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
#include "c_pal/sync.h"
#include "c_pal/string_utils.h"
#include "c_pal/socket_handle.h"

#include "c_pal/socket_mgr.h"

#define SOCKET_MGR_STATE_VALUES \
    SOCKET_MGR_STATE_CLOSED, \
    SOCKET_MGR_STATE_CONNECTING, \
    SOCKET_MGR_STATE_CONNECTED, \
    SOCKET_MGR_STATE_CLOSING

MU_DEFINE_ENUM(SOCKET_MGR_STATE, SOCKET_MGR_STATE_VALUES)
MU_DEFINE_ENUM_STRINGS(SOCKET_MGR_STATE, SOCKET_MGR_STATE_VALUES)

#define SOCKET_IO_TYPE_VALUES \
    SOCKET_IO_TYPE_SEND, \
    SOCKET_IO_TYPE_RECEIVE

MU_DEFINE_ENUM(SOCKET_IO_TYPE, SOCKET_IO_TYPE_VALUES)
MU_DEFINE_ENUM_STRINGS(SOCKET_IO_TYPE, SOCKET_IO_TYPE_VALUES)

typedef struct SOCKET_MGR_TAG
{
    SOCKET_HANDLE socket;

    volatile_atomic int32_t state;
    volatile_atomic int32_t pending_api_calls;

    SOCKET_TYPE type;
} SOCKET_MGR;

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

SOCKET_MGR_HANDLE socket_mgr_create(SOCKET_TYPE type)
{
    SOCKET_MGR* result;
    result = malloc(sizeof(SOCKET_MGR));
    if (result == NULL)
    {
        LogError("failure allocating SOCKET_MGR: %zu", sizeof(SOCKET_MGR));
    }
    else
    {
        result->type = type;

        (void)interlocked_exchange(&result->state, SOCKET_MGR_STATE_CLOSED);
        (void)interlocked_exchange(&result->pending_api_calls, 0);
    }
    return result;
}

void socket_mgr_destroy(SOCKET_MGR_HANDLE socket_client)
{
    if (socket_client == NULL)
    {
        // Do nothing
    }
    else
    {
        free(socket_client);
    }
}

int socket_mgr_connect(SOCKET_MGR_HANDLE socket_client, const char* hostname, uint16_t port, uint32_t connection_timeout)
{
    int result;
    if (socket_client == NULL ||
        hostname == NULL)
    {
        LogError("Invalid arguments: SOCKET_MGR_HANDLE socket_client: %p, const char* hostname: %s, uint16_t port: %" PRIu16 ", uint32_t connection_timeout: %" PRIu32 "",
            socket_client, MU_P_OR_NULL(hostname), port, connection_timeout);
        result = MU_FAILURE;
    }
    else
    {
        int32_t current_state = interlocked_compare_exchange(&socket_client->state, SOCKET_MGR_STATE_CONNECTING, SOCKET_MGR_STATE_CLOSED);
        if (current_state != SOCKET_MGR_STATE_CLOSED)
        {
            LogError("Open called in state %" PRI_MU_ENUM "", MU_ENUM_VALUE(SOCKET_MGR_STATE, current_state));
            result = MU_FAILURE;
        }
        else
        {
            socket_client->socket = connect_to_client(hostname, port, connection_timeout);
            if (socket_client->socket == INVALID_SOCKET)
            {
                LogError("Failure conneting to client hostname: %s:%" PRIu16 "", hostname, port);
                result = MU_FAILURE;
                (void)interlocked_exchange(&socket_client->state, SOCKET_MGR_STATE_CLOSED);
                wake_by_address_single(&socket_client->state);
            }
            else
            {
                (void)interlocked_exchange(&socket_client->state, SOCKET_MGR_STATE_CONNECTED);
                wake_by_address_single(&socket_client->state);

                result = 0;
            }
        }
    }
    return result;
}

void socket_mgr_disconnect(SOCKET_MGR_HANDLE socket_client)
{
    if (socket_client == NULL)
    {
        LogError("Invalid arguments: SOCKET_MGR_HANDLE socket_client: %p",
            socket_client);
    }
    else
    {
        close(socket_client->socket);
    }
}

int socket_mgr_send(SOCKET_MGR_HANDLE socket_client, SOCKET_BUFFER* payload, uint32_t buffer_count, uint32_t* bytes_written, uint32_t flags, void* data)
{
    int result;
    if (socket_client == NULL ||
        payload == NULL)
    {
        LogError("Invalid arguments: SOCKET_MGR_HANDLE socket_client: %p, const SOCKET_BUFFER* payload: %p, void* data: %p",
            socket_client, payload, data);
        result = MU_FAILURE;
    }
    else
    {
        int32_t current_state = interlocked_add(&socket_client->state, 0);
        if (current_state != SOCKET_MGR_STATE_CONNECTED)
        {
            LogError("Socket client not open, current state is %" PRI_MU_ENUM "", MU_ENUM_VALUE(SOCKET_MGR_STATE, current_state));
            result = MU_FAILURE;
        }
        else
        {
            (void)interlocked_increment(&socket_client->pending_api_calls);

            for (uint32_t index = 0; index < buffer_count; index++)
            {
                ssize_t data_sent = 0;
                do
                {
                    ssize_t send_size = send(socket_client->socket, payload[index].buffer, payload[index].length, MSG_NOSIGNAL);
                    if (send_size < 0)
                    {
                        result = MU_FAILURE;
                        break;
                    }
                    else
                    {
                        result = 0;
                        data_sent += send_size;
                    }
                } while (data_sent < payload[index].length);

                if (result == 0)
                {
    #ifdef ENABLE_SOCKET_LOGGING
                    LogVerbose("Send completed synchronously at %lf", timer_global_get_elapsed_us());
    #endif
                    if (bytes_written != NULL)
                    {
                        *bytes_written = data_sent;
                    }
                }
                else
                {
                    if (errno == ECONNRESET)
                    {
                        // todo: maybe we need to return a known value
                        result = MU_FAILURE;
                        LogError("A reset on the send socket has been encountered");
                    }
                    else
                    {
                        result = MU_FAILURE;
                        LogErrorNo("Failure sending socket data: buffer: %p, length: %" PRIu32 "", payload->buffer, payload->length);
                    }
                }
            }
            (void)interlocked_decrement(&socket_client->pending_api_calls);
            wake_by_address_single(&socket_client->pending_api_calls);
        }
    }
    return result;
}

int socket_mgr_receive(SOCKET_MGR_HANDLE socket_client, SOCKET_BUFFER* payload, uint32_t buffer_count, uint32_t* bytes_recv, uint32_t flags, void* data)
{
    int result;
    if (socket_client == NULL ||
        payload == NULL)
    {
        LogError("Invalid arguments: SOCKET_MGR_HANDLE socket_client=%p, const SOCKET_BUFFER* payload=%p, uint32_t flags=%" PRIu32 ", void*, data=%p",
            socket_client, payload, flags, data);
        result = MU_FAILURE;
    }
    else
    {
        int32_t current_state = interlocked_add(&socket_client->state, 0);
        if (current_state != SOCKET_MGR_STATE_CONNECTED)
        {
            LogWarning("Socket client not open, current state is %" PRI_MU_ENUM "", MU_ENUM_VALUE(SOCKET_MGR_STATE, current_state));
            result = MU_FAILURE;
        }
        else
        {
            (void)interlocked_increment(&socket_client->pending_api_calls);

            uint32_t total_recv_size = 0;
            for (uint32_t index = 0; index < buffer_count; index++)
            {
                ssize_t recv_size = recv(socket_client->socket, payload[index].buffer, payload[index].length, 0);
                if (recv_size < 0)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        // Codes_SRS_ASYNC_SOCKET_LINUX_11_089: [ If errno is EAGAIN or EWOULDBLOCK, then no data is available and event_complete_callback will break out of the function. ]
                        result = 0;
                        break;
                    }
                    else
                    {
                        total_recv_size = 0;
                        recv_size = 0;
                        if (errno == ECONNRESET)
                        {
                            result = MU_FAILURE;
                            LogError("A reset on the recv socket has been encountered");
                        }
                        else
                        {
                            result = MU_FAILURE;
                            LogErrorNo("failure recv data");
                        }
                        break;
                    }
                }
                else if (recv_size == 0)
                {
                    LogError("Socket received 0 bytes, assuming socket is closed");
                    result = MU_FAILURE;
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
                        result = MU_FAILURE;
                    }
                    else
                    {
                        total_recv_size += recv_size;
                        if (recv_size <= payload[index].length)
                        {
#ifdef ENABLE_SOCKET_LOGGING
                            LogVerbose("Asynchronous receive of %" PRIu32 " bytes completed at %lf", bytes_received, timer_global_get_elapsed_us());
#endif
                            result = 0;
                            break;
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
            (void)interlocked_decrement(&socket_client->pending_api_calls);
            wake_by_address_single(&socket_client->pending_api_calls);
        }
    }
    return result;
}

int socket_mgr_listen(SOCKET_MGR_HANDLE socket_client, uint16_t port)
{
    int result;
    if (socket_client == NULL)
    {
        LogError("Invalid arguments: SOCKET_MGR_HANDLE socket_client: %p",
            socket_client);
        result = MU_FAILURE;
    }
    else
    {
        int32_t current_state = interlocked_compare_exchange(&socket_client->state, SOCKET_MGR_STATE_CONNECTING, SOCKET_MGR_STATE_CLOSED);
        if (current_state != SOCKET_MGR_STATE_CLOSED)
        {
            LogError("Open called in state %" PRI_MU_ENUM "", MU_ENUM_VALUE(SOCKET_MGR_STATE, current_state));
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
                        (void)interlocked_exchange(&socket_client->state, SOCKET_MGR_STATE_CONNECTED);
                        wake_by_address_single(&socket_client->state);
                        result = 0;
                        goto all_ok;
                    }
                }
                close(socket_client->socket);
                (void)interlocked_exchange(&socket_client->state, SOCKET_MGR_STATE_CLOSED);
                wake_by_address_single(&socket_client->state);
            }
        }
    }
all_ok:
    return result;
}

SOCKET_MGR_HANDLE socket_mgr_accept(SOCKET_MGR_HANDLE socket_client)
{
    SOCKET_MGR* result;
    if (socket_client == NULL)
    {
        LogError("Invalid arguments: SOCKET_MGR_HANDLE socket_client: %p", socket_client);
        result = NULL;
    }
    else
    {
        int32_t current_state = interlocked_add(&socket_client->state, 0);
        if (current_state != SOCKET_MGR_STATE_CONNECTED)
        {
            LogError("Socket client not open, current state is %" PRI_MU_ENUM "", MU_ENUM_VALUE(SOCKET_MGR_STATE, current_state));
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
                    LogError("Failure: nonblocking failure.");
                    result = NULL;
                }
                else
                {
                    char hostname_addr[256];
                    (void)inet_ntop(AF_INET, (const void*)&cli_addr.sin_addr, hostname_addr, sizeof(hostname_addr));
                    LogError("Socket connected (%" PRI_SOCKET ") from %s:%d", accepted_socket, hostname_addr, cli_addr.sin_port);

                    // Create the socket handle
                    result = malloc(sizeof(SOCKET_MGR));
                    if (result == NULL)
                    {
                        LogError("failure allocating SOCKET_MGR: %zu", sizeof(SOCKET_MGR));
                    }
                    else
                    {
                        (void)interlocked_exchange(&result->state, SOCKET_MGR_STATE_CONNECTED);
                        (void)interlocked_exchange(&result->pending_api_calls, 0);
                        result->socket = accepted_socket;
                        goto all_ok;
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
