// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "umock_c/umock_c_prod.h" // IWYU pragma: keep

#include "c_logging/logger.h"
#include "c_logging/log_context.h"
#include "c_logging/log_context_property_type_ascii_char_ptr.h"

#include "c_pal/gballoc_hl.h"        // IWYU pragma: keep
#include "c_pal/gballoc_hl_redirect.h" // IWYU pragma: keep

#include "c_pal/sm.h"
#include "c_pal/socket_handle.h"

#include "c_pal/socket_transport.h"

#define SOCKET_IO_TYPE_VALUES \
    SOCKET_IO_TYPE_SEND, \
    SOCKET_IO_TYPE_RECEIVE

MU_DEFINE_ENUM(SOCKET_IO_TYPE, SOCKET_IO_TYPE_VALUES)
MU_DEFINE_ENUM_STRINGS(SOCKET_IO_TYPE, SOCKET_IO_TYPE_VALUES)

MU_DEFINE_ENUM_STRINGS(SOCKET_TYPE, SOCKET_TYPE_VALUES)

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
    // Codes_SOCKET_TRANSPORT_LINUX_11_015: [ socket_transport_connect shall call socket with the params AF_INET, SOCK_STREAM and 0. ]
    SOCKET_HANDLE client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET)
    {
        LogErrorNo("Failure: socket create failure.");
    }
    else
    {
        char port_string[16];
        if (sprintf(port_string, "%u", port) < 0)
        {
            LogError("failed to copy port value %" PRIu16 "", port);
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
            }
            else
            {
                LogInfo("Connecting to %s:%" PRIu16 " Machine Name: %s, connection timeout: %" PRIu32 "", hostname, port, MU_P_OR_NULL(addrInfo->ai_canonname), connection_timeout);

                // Codes_SOCKET_TRANSPORT_LINUX_11_016: [ socket_transport_connect shall call connect to connect to the endpoint. ]
                if (connect(client_socket, addrInfo->ai_addr, addrInfo->ai_addrlen) != 0)
                {
                    LogErrorNo("Connection failure. Server: %s:%" PRIu16 ".", hostname, port);
                }
                // Codes_SOCKET_TRANSPORT_LINUX_11_017: [ socket_transport_connect shall set the socket to non-blocking by calling fcntl with O_NONBLOCK. ]
                else if (set_nonblocking(client_socket) != 0)
                {
                    LogError("Failure: nonblocking failure.");
                }
                else
                {
                    freeaddrinfo(addrInfo);
                    goto all_ok;
                }
                freeaddrinfo(addrInfo);
            }
        }
        close(client_socket);
        client_socket = INVALID_SOCKET;
    }
all_ok:
    return client_socket;
}

SOCKET_TRANSPORT_HANDLE socket_transport_create_client(void)
{
    SOCKET_TRANSPORT* result;

    // Codes_SOCKET_TRANSPORT_WIN32_11_002: [ socket_transport_create_client shall allocate a new SOCKET_TRANSPORT object. ]
    result = malloc(sizeof(SOCKET_TRANSPORT));
    if (result == NULL)
    {
        LogError("failure allocating SOCKET_TRANSPORT: %zu", sizeof(SOCKET_TRANSPORT));
    }
    else
    {
        // Codes_SOCKET_TRANSPORT_LINUX_11_003: [ socket_transport_create_client shall call sm_create to create a sm object with the type set to SOCKET_CLIENT. ]
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
        // Codes_SOCKET_TRANSPORT_WIN32_11_004: [ On any failure socket_transport_create_client shall return NULL. ]
        result = NULL;
    }
all_ok:
    // Codes_SOCKET_TRANSPORT_WIN32_11_005: [ On success socket_transport_create_client shall return SOCKET_TRANSPORT_HANDLE. ]
    return result;
}

SOCKET_TRANSPORT_HANDLE socket_transport_create_server(void)
{
    SOCKET_TRANSPORT* result;

    // Codes_SOCKET_TRANSPORT_LINUX_11_079: [ socket_transport_create shall allocate a new SOCKET_TRANSPORT object. ]
    result = malloc(sizeof(SOCKET_TRANSPORT));
    if (result == NULL)
    {
        LogError("failure allocating SOCKET_TRANSPORT: %zu", sizeof(SOCKET_TRANSPORT));
    }
    else
    {
        // Codes_SOCKET_TRANSPORT_LINUX_11_080: [ socket_transport_create_server shall call sm_create to create a sm object with the type set to SOCKET_BINDING. ]
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
        // Codes_SOCKET_TRANSPORT_WIN32_11_081: [ On any failure socket_transport_create shall return NULL. ]
        result = NULL;
    }
all_ok:
    // Codes_SOCKET_TRANSPORT_WIN32_11_082: [ On success socket_transport_create shall return SOCKET_TRANSPORT_HANDLE. ]
    return result;
}

void socket_transport_destroy(SOCKET_TRANSPORT_HANDLE socket_transport)
{
    // Codes_SOCKET_TRANSPORT_LINUX_11_006: [ If socket_transport is NULL socket_transport_destroy shall return. ]
    if (socket_transport == NULL)
    {
        // Do nothing
    }
    else
    {
        // Codes_SOCKET_TRANSPORT_LINUX_11_007: [ socket_transport_destroy shall call sm_destroy to destroy the sm object. ]
        sm_destroy(socket_transport->sm);
        // Codes_SOCKET_TRANSPORT_LINUX_11_008: [ socket_transport_destroy shall free the SOCKET_TRANSPORT_HANDLE object. ]
        free(socket_transport);
    }
}

int socket_transport_connect(SOCKET_TRANSPORT_HANDLE socket_transport, const char* hostname, uint16_t port, uint32_t connection_timeout)
{
    int result;
    // Codes_SOCKET_TRANSPORT_LINUX_11_009: [ If socket_transport is NULL, socket_transport_connect shall fail and return a non-zero value. ]
    if (socket_transport == NULL ||
        // Codes_SOCKET_TRANSPORT_LINUX_11_010: [ If hostname is NULL, socket_transport_connect shall fail and return a non-zero value. ]
        hostname == NULL ||
        // Codes_SOCKET_TRANSPORT_LINUX_11_011: [ If port is 0, socket_transport_connect shall fail and return a non-zero value. ]
        port == 0)
    {
        LogError("Invalid arguments: SOCKET_TRANSPORT_HANDLE socket_transport: %p, const char* hostname: %s, uint16_t port: %" PRIu16 ", uint32_t connection_timeout: %" PRIu32 "",
            socket_transport, MU_P_OR_NULL(hostname), port, connection_timeout);
        result = MU_FAILURE;
    }
    else
    {
        // Codes_SOCKET_TRANSPORT_LINUX_11_012: [ If the socket_transport is not SOCKET_CLIENT, socket_transport_connect shall fail and return a non-zero value. ]
        if (socket_transport->type != SOCKET_CLIENT)
        {
            LogError("Invalid socket type for this API expected: SOCKET_CLIENT, actual: %" PRI_MU_ENUM, MU_ENUM_VALUE(SOCKET_TYPE, socket_transport->type));
            result = MU_FAILURE;
        }
        else
        {
            // Codes_SOCKET_TRANSPORT_LINUX_11_013: [ socket_transport_connect shall call sm_open_begin to begin the open. ]
            SM_RESULT open_result = sm_open_begin(socket_transport->sm);
            if (open_result != SM_EXEC_GRANTED)
            {
                // Codes_SOCKET_TRANSPORT_LINUX_11_014: [ If sm_open_begin does not return SM_EXEC_GRANTED, socket_transport_connect shall fail and return a non-zero value. ]
                LogError("sm_open_begin failed with %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, open_result));
                result = MU_FAILURE;
            }
            else
            {
                socket_transport->socket = connect_to_client(hostname, port, connection_timeout);
                if (socket_transport->socket == INVALID_SOCKET)
                {
                    // Codes_SOCKET_TRANSPORT_LINUX_11_019: [ If any failure is encountered, socket_transport_connect shall call sm_open_end with false, fail and return a non-zero value. ]
                    LogError("Failure conneting to client hostname: %s:%" PRIu16 "", hostname, port);
                    result = MU_FAILURE;
                    sm_open_end(socket_transport->sm, false);
                }
                else
                {
                    // Codes_SOCKET_TRANSPORT_LINUX_11_018: [ If successful socket_transport_connect shall call sm_open_end with true. ]
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
    // Codes_SOCKET_TRANSPORT_LINUX_11_020: [ If socket_transport is NULL, socket_transport_disconnect shall fail and return. ]
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
            // Codes_SOCKET_TRANSPORT_LINUX_11_025: [ socket_transport_disconnect shall call shutdown to stop both the transmit and reception of the connected socket. ]
            if (shutdown(socket_transport->socket, 2) != 0)
            {
                // Codes_SOCKET_TRANSPORT_LINUX_11_026: [ If shutdown does not return 0, the socket is not valid therefore socket_transport_disconnect shall not call 'close' ]
                LogErrorNo("shutdown failed on socket: %" PRI_SOCKET "", socket_transport->socket);
            }
            else
            {
                // Codes_SOCKET_TRANSPORT_LINUX_11_023: [ socket_transport_disconnect shall call close to disconnect the connected socket. ]
                if (close(socket_transport->socket) != 0)
                {
                    LogErrorNo("Close failure on socket: %" PRI_SOCKET "", socket_transport->socket);
                }
            }
            // Codes_SOCKET_TRANSPORT_LINUX_11_024: [ socket_transport_disconnect shall call sm_close_end. ]
            sm_close_end(socket_transport->sm);
        }
        else
        {
            LogError("sm_close_begin failed with %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, close_result));
        }
    }
}

SOCKET_SEND_RESULT socket_transport_send(SOCKET_TRANSPORT_HANDLE socket_transport, const SOCKET_BUFFER* payload, uint32_t buffer_count, uint32_t* bytes_sent, uint32_t flags, void* data)
{
    SOCKET_SEND_RESULT result;
    // Codes_SOCKET_TRANSPORT_LINUX_11_027: [ If socket_transport is NULL, socket_transport_send shall fail and return SOCKET_SEND_INVALID_ARG. ]
    if (socket_transport == NULL ||
        // Codes_SOCKET_TRANSPORT_LINUX_11_028: [ If payload is NULL, socket_transport_send shall fail and return SOCKET_SEND_INVALID_ARG. ]
        payload == NULL ||
        // Codes_SOCKET_TRANSPORT_LINUX_11_029: [ If buffer_count is 0, socket_transport_send shall fail and return SOCKET_SEND_INVALID_ARG. ]
        buffer_count == 0)
    {
        LogError("Invalid arguments: SOCKET_TRANSPORT_HANDLE socket_transport: %p, const SOCKET_BUFFER* payload: %p, uint32_T buffer_count: %" PRIu32 ", void* data: %p",
            socket_transport, payload, buffer_count, data);
        result = SOCKET_SEND_INVALID_ARG;
    }
    else
    {
        // Codes_SOCKET_TRANSPORT_LINUX_11_030: [ socket_transport_send shall call sm_exec_begin. ]
        SM_RESULT sm_result = sm_exec_begin(socket_transport->sm);
        if (sm_result != SM_EXEC_GRANTED)
        {
            // Codes_SOCKET_TRANSPORT_LINUX_11_031: [ If sm_exec_begin does not return SM_EXEC_GRANTED, socket_transport_send shall fail and return SOCKET_SEND_ERROR. ]
            LogError("sm_exec_begin failed : %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, sm_result));
            result = SOCKET_SEND_ERROR;
        }
        else
        {
            uint32_t total_send_size = 0;
            for (uint32_t index = 0; index < buffer_count; index++)
            {
                // Codes_SOCKET_TRANSPORT_LINUX_11_032: [ For each buffer count in payload socket_transport_send shall call send to send data with flags as a parameter. ]
                ssize_t data_sent = 0;
                do
                {
                    ssize_t send_size = send(socket_transport->socket, payload[index].buffer, payload[index].length, flags);
                    if (send_size < 0)
                    {
                        // Codes_SOCKET_TRANSPORT_LINUX_11_033: [ If send returns a value less then 0, socket_transport_send shall stop sending and return SOCKET_SEND_FAILED. ]
                        LogErrorNo("Failure sending data index: %" PRIu32 ", payload.buffer: %p, payload.length: %" PRIu32 "", index, payload[index].buffer, payload[index].length);
                        result = SOCKET_SEND_FAILED;
                        break;
                    }
                    else
                    {
                        result = SOCKET_SEND_OK;
                        data_sent += send_size;
                    }
                    // Codes_SOCKET_TRANSPORT_LINUX_11_035: [ Otherwise socket_transport_send shall continue calling send until the SOCKET_BUFFER length is reached. ]
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
                        // Codes_SOCKET_TRANSPORT_LINUX_11_034: [ If the errno is equal to ECONNRESET, socket_transport_send shall return SOCKET_SEND_SHUTDOWN. ]
                        result = SOCKET_SEND_SHUTDOWN;
                        LogError("A reset on the send socket has been encountered");
                    }
                    else
                    {
                        // Send the result unchanged
                        LogErrorNo("Failure sending socket data: buffer: %p, length: %" PRIu32 "", payload->buffer, payload->length);
                    }
                }
            }
            if (bytes_sent != NULL)
            {
                *bytes_sent = total_send_size;
            }
            sm_exec_end(socket_transport->sm);
        }
    }
    return result;
}

SOCKET_RECEIVE_RESULT socket_transport_receive(SOCKET_TRANSPORT_HANDLE socket_transport, SOCKET_BUFFER* payload, uint32_t buffer_count, uint32_t* bytes_recv, uint32_t flags, void* data)
{
    SOCKET_RECEIVE_RESULT result;
    // Codes_SOCKET_TRANSPORT_LINUX_11_038: [ If socket_transport is NULL, socket_transport_receive shall fail and return SOCKET_RECEIVE_INVALID_ARG. ]
    if (socket_transport == NULL ||
        // Codes_SOCKET_TRANSPORT_LINUX_11_039: [ If payload is NULL, socket_transport_receive shall fail and return SOCKET_RECEIVE_INVALID_ARG. ]
        payload == NULL ||
        // Codes_SOCKET_TRANSPORT_LINUX_11_040: [ If buffer_count is 0, socket_transport_receive shall fail and return SOCKET_RECEIVE_INVALID_ARG. ]
        buffer_count == 0)
    {
        LogError("Invalid arguments: SOCKET_TRANSPORT_HANDLE socket_transport=%p, const SOCKET_BUFFER* payload=%p, uint32_t buffer_count: %" PRIu32 ", uint32_t flags=%" PRIu32 ", void*, data=%p",
            socket_transport, payload, buffer_count, flags, data);
        result = SOCKET_RECEIVE_INVALID_ARG;
    }
    else
    {
        // Codes_SOCKET_TRANSPORT_LINUX_11_041: [ socket_transport_receive shall call sm_exec_begin. ]
        SM_RESULT sm_result = sm_exec_begin(socket_transport->sm);
        if (sm_result != SM_EXEC_GRANTED)
        {
            // Codes_SOCKET_TRANSPORT_LINUX_11_042: [ If sm_exec_begin does not return SM_EXEC_GRANTED, socket_transport_receive shall fail and return SOCKET_RECEIVE_ERROR. ]
            LogError("sm_exec_begin failed : %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, sm_result));
            result = SOCKET_RECEIVE_ERROR;
        }
        else
        {
            uint32_t total_recv_size = 0;
            for (uint32_t index = 0; index < buffer_count; index++)
            {
                // Codes_SOCKET_TRANSPORT_LINUX_11_043: [ For each buffer count in payload socket_transport_receive shall call recv with the flags parameter. ]
                ssize_t recv_size = recv(socket_transport->socket, payload[index].buffer, payload[index].length, flags);
                if (recv_size < 0)
                {
                    // Codes_SOCKET_TRANSPORT_LINUX_11_044: [ If recv a value less then 0, socket_transport_receive shall do the following: ]
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        // Codes_SOCKET_TRANSPORT_LINUX_11_045: [ If errno is EAGAIN or EWOULDBLOCK, socket_transport_receive shall break out of loop and return SOCKET_RECEIVE_WOULD_BLOCK. ]
                        result = SOCKET_RECEIVE_WOULD_BLOCK;
                        break;
                    }
                    else
                    {
                        total_recv_size = 0;
                        recv_size = 0;
                        if (errno == ECONNRESET)
                        {
                            // Codes_SOCKET_TRANSPORT_LINUX_11_046: [ If errno is ECONNRESET, socket_transport_receive shall break out of the loop and return SOCKET_RECEIVE_SHUTDOWN. ]
                            result = SOCKET_RECEIVE_SHUTDOWN;
                            LogError("A reset on the recv socket has been encountered");
                        }
                        else
                        {
                            // Codes_SOCKET_TRANSPORT_LINUX_11_047: [ else socket_transport_receive shall break out of the looop and return SOCKET_RECEIVE_ERROR. ]
                            result = SOCKET_RECEIVE_ERROR;
                            LogErrorNo("failure recv data");
                        }
                        break;
                    }
                }
                else if (recv_size == 0)
                {
                    // Codes_SOCKET_TRANSPORT_LINUX_11_048: [ If recv returns a 0 value, socket_transport_receive shall break and return SOCKET_RECEIVE_SHUTDOWN. ]
                    LogError("Socket received 0 bytes, assuming socket is closed");
                    result = SOCKET_RECEIVE_SHUTDOWN;
                    break;
                }
                else
                {
                    // Codes_SOCKET_TRANSPORT_LINUX_11_049: [ Else socket_transport_receive shall do the following: ]
                    // Codes_SOCKET_TRANSPORT_LINUX_11_050: [ socket_transport_receive shall test that the total recv size will not overflow. ]
                    if (recv_size > UINT32_MAX ||
                        UINT32_MAX - total_recv_size < recv_size)
                    {
                        // Handle unlikely overflow
                        LogError("Overflow in computing receive size (total_recv_size=%" PRIu32 " + recv_size=%zi > UINT32_MAX=%" PRIu32 ")",
                            total_recv_size, recv_size, UINT32_MAX);
                        result = SOCKET_RECEIVE_ERROR;
                        break;
                    }
                    else
                    {
                        // Codes_SOCKET_TRANSPORT_LINUX_11_051: [ socket_transport_receive shall store the received byte size. ]
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
                            // keep receiving data
                        }
                    }
                }
            }

            if (result == SOCKET_RECEIVE_OK)
            {
                // Success
                if (bytes_recv != NULL)
                {
                    *bytes_recv = total_recv_size;
                }
            }
            // Codes_SOCKET_TRANSPORT_LINUX_11_053: [ socket_transport_receive shall call sm_exec_end. ]
            sm_exec_end(socket_transport->sm);
        }
    }
    return result;
}

int socket_transport_listen(SOCKET_TRANSPORT_HANDLE socket_transport, uint16_t port)
{
    int result;
    // Codes_SOCKET_TRANSPORT_LINUX_11_054: [ If socket_transport is NULL, socket_transport_listen shall fail and return a non-zero value. ]
    if (socket_transport == NULL ||
        // Codes_SOCKET_TRANSPORT_LINUX_11_055: [ If port is 0, socket_transport_listen shall fail and return a non-zero value. ]
        port == 0)
    {
        LogError("Invalid arguments: SOCKET_TRANSPORT_HANDLE socket_transport: %p, uint16_t port: %" PRIu16 "",
            socket_transport, port);
        result = MU_FAILURE;
    }
    else
    {
        // Codes_SOCKET_TRANSPORT_LINUX_11_056: [ If the transport type is not SOCKET_BINDING, socket_transport_listen shall fail and return a non-zero value. ]
        if (socket_transport->type != SOCKET_BINDING)
        {
            LogError("Invalid socket type for this API expected: SOCKET_BINDING, actual: %" PRI_MU_ENUM, MU_ENUM_VALUE(SOCKET_TYPE, socket_transport->type));
            result = MU_FAILURE;
        }
        else
        {
            // Codes_SOCKET_TRANSPORT_LINUX_11_057: [ socket_transport_listen shall call sm_open_begin to begin the open. ]
            SM_RESULT open_result = sm_open_begin(socket_transport->sm);
            if (open_result != SM_EXEC_GRANTED)
            {
                // Codes_SOCKET_TRANSPORT_LINUX_11_058: [ If sm_open_begin does not return SM_EXEC_GRANTED, socket_transport_listen shall fail and return a non-zero value. ]
                LogError("sm_open_begin failed with %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, open_result));
                result = MU_FAILURE;
            }
            else
            {
                // Codes_SOCKET_TRANSPORT_LINUX_11_059: [ socket_transport_listen shall call socket with the params AF_INET, SOCK_STREAM and IPPROTO_TCP. ]
                socket_transport->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                if (socket_transport->socket == INVALID_SOCKET)
                {
                    LogErrorNo("Could not create socket");
                    result = MU_FAILURE;
                }
                else
                {
                    struct sockaddr_in service;

                    const int enable = 1;
                    // Codes_SOCKET_TRANSPORT_LINUX_11_083: [ socket_transport_listen shall set the SO_REUSEADDR option on the socket. ]
                    (void)setsockopt(socket_transport->socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

                    service.sin_family = AF_INET;
                    service.sin_addr.s_addr = htonl(INADDR_ANY);
                    service.sin_port = htons(port);

                    // Codes_SOCKET_TRANSPORT_LINUX_11_060: [ socket_transport_listen shall bind to the socket by calling bind. ]
                    if (bind(socket_transport->socket, (struct sockaddr*)&service, sizeof(service)) != 0)
                    {
                        LogErrorNo("Could not bind socket, port=%" PRIu16 "", port);
                        result = MU_FAILURE;
                    }
                    else if (set_nonblocking(socket_transport->socket) != 0)
                    {
                        LogErrorNo("Could not set listening socket in non-blocking mode");
                        result = MU_FAILURE;
                    }
                    else
                    {
                        // Codes_SOCKET_TRANSPORT_LINUX_11_061: [ socket_transport_listen shall start listening to incoming connection by calling listen. ]
                        if (listen(socket_transport->socket, SOMAXCONN) != 0)
                        {
                            LogErrorNo("Could not start listening for connections");
                            result = MU_FAILURE;
                        }
                        else
                        {
                            // Codess_SOCKET_TRANSPORT_LINUX_11_062: [ If successful socket_transport_listen shall call sm_open_end with true. ]
                            sm_open_end(socket_transport->sm, true);
                            result = 0;
                            goto all_ok;
                        }
                    }
                    // Codes_SOCKET_TRANSPORT_LINUX_11_063: [ If any failure is encountered, socket_transport_listen shall call sm_open_end with false, fail and return a non-zero value. ]
                    close(socket_transport->socket);
                }
                sm_open_end(socket_transport->sm, false);
            }
        }
    }
all_ok:
    return result;
}

SOCKET_TRANSPORT_HANDLE socket_transport_accept(SOCKET_TRANSPORT_HANDLE socket_transport)
{
    SOCKET_TRANSPORT* result;
    // Codes_SOCKET_TRANSPORT_LINUX_11_069: [ If socket_transport is NULL, socket_transport_accept shall fail and return NULL. ]
    if (socket_transport == NULL)
    {
        LogError("Invalid arguments: SOCKET_TRANSPORT_HANDLE socket_transport: %p", socket_transport);
        result = NULL;
    }
    else
    {
        // Codes_SOCKET_TRANSPORT_LINUX_11_070: [ If the transport type is not SOCKET_BINDING, socket_transport_accept shall fail and return NULL. ]
        if (socket_transport->type != SOCKET_BINDING)
        {
            LogError("Invalid socket type for this API expected: SOCKET_BINDING, actual: %" PRI_MU_ENUM, MU_ENUM_VALUE(SOCKET_TYPE, socket_transport->type));
            result = NULL;
        }
        else
        {
            // Codes_SOCKET_TRANSPORT_LINUX_11_071: [ socket_transport_accept shall call sm_exec_begin. ]
            SM_RESULT sm_result = sm_exec_begin(socket_transport->sm);
            if (sm_result != SM_EXEC_GRANTED)
            {
                // Codes_SOCKET_TRANSPORT_LINUX_11_072: [ If sm_exec_begin does not return SM_EXEC_GRANTED, socket_transport_accept shall fail and return NULL. ]
                LogError("sm_exec_begin failed : %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, sm_result));
                result = NULL;
            }
            else
            {
                struct sockaddr_in cli_addr;
                socklen_t client_len = sizeof(cli_addr);

                SOCKET_HANDLE accepted_socket;
                // Codes_SOCKET_TRANSPORT_LINUX_11_073: [ socket_transport_accept shall call accept to accept the incoming socket connection. ]
                accepted_socket = accept(socket_transport->socket, (struct sockaddr*)&cli_addr, &client_len);
                if (accepted_socket == INVALID_SOCKET)
                {
                    LogErrorNo("Failure accepting socket");
                    result = NULL;
                }
                else
                {
                    // Codes_SOCKET_TRANSPORT_LINUX_11_074: [ socket_transport_accept shall set the incoming socket to non-blocking. ]
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
                        // Codes_SOCKET_TRANSPORT_LINUX_11_075: [ socket_transport_accept shall allocate a SOCKET_TRANSPORT for the incoming connection and call sm_create and sm_open on the connection. ]
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
                                    // Codes_SOCKET_TRANSPORT_LINUX_11_076: [ If successful socket_transport_accept shall return the allocated SOCKET_TRANSPORT of type SOCKET_DATA. ]
                                    result->type = SOCKET_CLIENT;
                                    result->socket = accepted_socket;
                                    sm_open_end(result->sm, true);
                                    sm_exec_end(socket_transport->sm);
                                    goto all_ok;
                                }
                                else
                                {
                                    LogError("sm_open_begin failed with %" PRI_MU_ENUM " in accept, closing incoming socket.", MU_ENUM_VALUE(SM_RESULT, open_result));
                                }
                                sm_destroy(result->sm);
                            }
                            // Codes_SOCKET_TRANSPORT_LINUX_11_077: [ If any failure is encountered, socket_transport_accept shall fail and return NULL. ]
                            free(result);
                            result = NULL;
                        }
                    }
                    close(accepted_socket);
                }
                // Codes_SOCKET_TRANSPORT_LINUX_11_078: [ socket_transport_accept shall call sm_exec_end. ]
                sm_exec_end(socket_transport->sm);
            }
        }
        result = NULL;
    }
all_ok:
    return result;
}

SOCKET_HANDLE socket_transport_get_underlying_socket(SOCKET_TRANSPORT_HANDLE socket_transport)
{
    SOCKET_HANDLE result;
    // Codes_SOCKET_TRANSPORT_LINUX_11_064: [ If socket_transport is NULL, socket_transport_get_underlying_socket shall fail and return INVALID_SOCKET. ]
    if (socket_transport == NULL)
    {
        LogError("Invalid arguments: SOCKET_TRANSPORT_HANDLE socket_transport: %p",
            socket_transport);
        result = INVALID_SOCKET;
    }
    else
    {
        // Codes_SOCKET_TRANSPORT_LINUX_11_065: [ socket_transport_get_underlying_socket shall call sm_exec_begin. ]
        SM_RESULT sm_result = sm_exec_begin(socket_transport->sm);
        if (sm_result != SM_EXEC_GRANTED)
        {
            // Codes_SOCKET_TRANSPORT_LINUX_11_066: [ If sm_exec_begin does not return SM_EXEC_GRANTED, socket_transport_get_underlying_socket shall fail and return INVALID_SOCKET. ]
            LogError("sm_exec_begin failed : %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, sm_result));
            result = (SOCKET_HANDLE)INVALID_SOCKET;
        }
        else
        {
            // Codes_SOCKET_TRANSPORT_LINUX_11_067: [ socket_transport_get_underlying_socket shall return the SOCKET_HANDLE socket value. ]
            result = (SOCKET_HANDLE)socket_transport->socket;
            // Codes_SOCKET_TRANSPORT_LINUX_11_068: [ socket_transport_get_underlying_socket shall call sm_exec_end. ]
            sm_exec_end(socket_transport->sm);
        }
    }
    return result;
}

int socket_transport_local_address(SOCKET_TRANSPORT_HANDLE socket_transport, char hostname[MAX_GET_HOST_NAME_LEN], LOCAL_ADDRESS** local_address_list, uint32_t* address_count)
{
    int result;
    if (socket_transport == NULL ||
        hostname == NULL ||
        (local_address_list != NULL && address_count == NULL)
    )
    {
        LogError("Invalid arguments: SOCKET_TRANSPORT_HANDLE socket_transport: %p, char* hostname: %p, char* local_address_list[MAX_LOCAL_ADDRESS_LEN]: %p, uint32_t* address_count: %p",
            socket_transport, hostname, local_address_list, address_count);
        result = MU_FAILURE;
    }
    else
    {
        if (gethostname(hostname, MAX_GET_HOST_NAME_LEN) == SOCKET_ERROR)
        {
            LogLastError("Unable to get hostname");
            result = MU_FAILURE;
        }
        else
        {
            if (local_address_list != NULL)
            {
                struct hostent* host_info = gethostbyname(hostname);
                if (host_info == NULL)
                {
                    LogLastError("Failure in call to gethostbyname %s", hostname);
                    result = MU_FAILURE;
                }
                else
                {
                    if (host_info->h_addrtype == AF_INET)
                    {
                        uint32_t total_count = 0;
                        uint32_t address_index = 0;
                        struct in_addr addr;

                        // Count the address returned
                        while (host_info->h_addr_list[total_count] != 0)
                        {
                            total_count++;
                        }

                        // Allocate the array
                        LOCAL_ADDRESS* temp_list = malloc_2(sizeof(LOCAL_ADDRESS), total_count);
                        if (temp_list == NULL)
                        {
                            LogError("failure in malloc_2(total_count: %" PRIu32 ", MAX_LOCAL_ADDRESS_LEN: %d)", total_count, MAX_LOCAL_ADDRESS_LEN);
                            result = MU_FAILURE;
                        }
                        else
                        {
                            while (host_info->h_addr_list[address_index] != 0)
                            {
                                temp_list[address_index].address_type = ADDRESS_INET;

                                addr.s_addr = *(u_long*)host_info->h_addr_list[address_index];
                                inet_ntop(AF_INET, &(addr.s_addr), temp_list[address_index].address, MAX_LOCAL_ADDRESS_LEN);

                                LogInfo("IPv4 Address #: %s", temp_list[address_index].address);
                                address_index++;
                            }
                            *address_count = total_count;
                            *local_address_list = temp_list;
                            result = 0;
                        }
                    }
                    else if (host_info->h_addrtype == AF_INET6)
                    {
                        address_count = 0;
                        result = 0;
                    }
                    else if (host_info->h_addrtype == AF_NETBIOS)
                    {
                        address_count = 0;
                        result = 0;
                    }
                    else
                    {
                        LogError("Unknown address type h_addrtype: %d", host_info->h_addrtype);
                        result = MU_FAILURE;
                    }
                }
            }
            else
            {
                result = 0;
            }
        }
    }
    return result;
}