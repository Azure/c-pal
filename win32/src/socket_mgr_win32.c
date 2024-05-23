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
#include "c_pal/sync.h"
#include "c_pal/string_utils.h"

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
    SOCKET socket;

    volatile_atomic int32_t state;
    volatile_atomic int32_t pending_api_calls;

    SOCKET_TYPE type;
} SOCKET_MGR;

static int connect_to_endpoint(SOCKET client_socket, const ADDRINFO* addrInfo, uint32_t timeout_usec)
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
            tv.tv_usec = timeout_usec;

            int ret = select(0, NULL, &write_fds, NULL, &tv);
            if (ret == SOCKET_ERROR)
            {
                LogError("Failure connecting error: %d", WSAGetLastError());
                result = MU_FAILURE;
            }
            else if (ret == 0)
            {
                LogError("Failure timeout (%" PRId32 " us) attempting to connect", timeout_usec);
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
        closesocket(socket_client->socket);
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

            DWORD num_bytes = 0;
            LPDWORD num_bytes_param = NULL;
            // If the overlapped is NULL then set the num bytes params
            if (data == NULL)
            {
                num_bytes_param = &num_bytes;
            }

            result = WSASend(socket_client->socket, (LPWSABUF)payload, buffer_count, num_bytes_param, flags, (LPWSAOVERLAPPED)data, NULL);
            if (result == 0)
            {
#ifdef ENABLE_SOCKET_LOGGING
                LogVerbose("Send completed synchronously at %lf", timer_global_get_elapsed_us());
#endif
                if (bytes_written != NULL)
                {
                    *bytes_written = num_bytes;
                }
            }
            else
            {
                LogLastError("Failure sending socket data: buffer: %p, length: %" PRIu32 "", payload->buffer, payload->length);
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

            DWORD num_bytes = 0;
            LPDWORD num_bytes_param = NULL;
            if (data == NULL)
            {
                num_bytes_param = &num_bytes;
            }

            result = WSARecv(socket_client->socket, (LPWSABUF)payload, buffer_count, num_bytes_param, (LPDWORD)&flags, (LPWSAOVERLAPPED)data, NULL);
            if (result == 0)
            {
                // Success
                if (bytes_recv != NULL)
                {
                    *bytes_recv = num_bytes;
                }
            }
            else
            {
                int wsa_last_error = WSAGetLastError();
                if (WSA_IO_PENDING == wsa_last_error)
                {
                    *bytes_recv = 0;
                    result = 0;
                }
                else
                {
                    LogLastError("WSARecv failed with %d, WSAGetLastError returned %lu", result, wsa_last_error);
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

                if (bind(socket_client->socket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR)
                {
                    LogLastError("Could not bind socket, port=%" PRIu16 "", port);
                    result = MU_FAILURE;
                }
                else if (ioctlsocket(socket_client->socket, FIONBIO, &iMode) != 0)
                {
                    LogLastError("Could not set listening socket in non-blocking mode");
                    result = MU_FAILURE;
                }
                else
                {
                    if (listen(socket_client->socket, SOMAXCONN) == SOCKET_ERROR)
                    {
                        LogLastError("Could not start listening for connections");
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
                closesocket(socket_client->socket);
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
        LogError("Invalid arguments: SOCKET_MGR_HANDLE socket_client: %p",
            socket_client);
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
            fd_set read_fds;
            int select_result;
            struct timeval timeout;
            bool is_error = false;

            read_fds.fd_array[0] = socket_client->socket;
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

                SOCKET accepted_socket = accept(socket_client->socket, (struct sockaddr*)&cli_addr, &client_len);
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
                    }
                }
            }
            else
            {
                LogLastError("Failure accepting socket connection");
                is_error = true;
                result = NULL;
            }
        }
    }
    return result;
}
