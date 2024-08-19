# socket transport Linux

## Overview

socket_transport_linux is the module that abstracts the socket transport for the linux platform

## References

- [socket transport requirements](../../interfaces/devdoc/socket_transport_requirements.md)

## Exposed API

```c
typedef struct SOCKET_TRANSPORT_TAG* SOCKET_TRANSPORT_HANDLE;

#define SOCKET_SEND_RESULT_VALUES \
    SOCKET_SEND_INVALID_ARG, \
    SOCKET_SEND_OK, \
    SOCKET_SEND_ERROR, \
    SOCKET_SEND_FAILED, \
    SOCKET_SEND_SHUTDOWN

MU_DEFINE_ENUM(SOCKET_SEND_RESULT, SOCKET_SEND_RESULT_VALUES)

#define SOCKET_RECEIVE_RESULT_VALUES \
    SOCKET_RECEIVE_INVALID_ARG, \
    SOCKET_RECEIVE_OK, \
    SOCKET_RECEIVE_WOULD_BLOCK, \
    SOCKET_RECEIVE_ERROR, \
    SOCKET_RECEIVE_SHUTDOWN

MU_DEFINE_ENUM(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_RESULT_VALUES)

#define SOCKET_ACCEPT_RESULT_VALUES \
    SOCKET_ACCEPT_OK, \
    SOCKET_ACCEPT_ERROR, \
    SOCKET_ACCEPT_NO_SOCKET

MU_DEFINE_ENUM(SOCKET_ACCEPT_RESULT, SOCKET_ACCEPT_RESULT_VALUES)

#define SOCKET_TYPE_VALUES \
    SOCKET_CLIENT, \
    SOCKET_BINDING

MU_DEFINE_ENUM(SOCKET_TYPE, SOCKET_TYPE_VALUES)

typedef struct SOCKET_BUFFER_TAG
{
    uint32_t length;
    void* buffer;
} SOCKET_BUFFER;

MOCKABLE_FUNCTION(, SOCKET_TRANSPORT_HANDLE, socket_transport_create_client);
MOCKABLE_FUNCTION(, SOCKET_TRANSPORT_HANDLE, socket_transport_create_server);
MOCKABLE_FUNCTION(, SOCKET_TRANSPORT_HANDLE, socket_transport_create_from_socket, SOCKET_HANDLE, socket_handle);
MOCKABLE_FUNCTION(, void, socket_transport_destroy, SOCKET_TRANSPORT_HANDLE, socket_transport);

MOCKABLE_FUNCTION(, int, socket_transport_connect, SOCKET_TRANSPORT_HANDLE, socket_transport, const char*, hostname, uint16_t, port, uint32_t, connection_timeout_ms);
MOCKABLE_FUNCTION(, int, socket_transport_listen, SOCKET_TRANSPORT_HANDLE, socket_transport, uint16_t, port);
MOCKABLE_FUNCTION(, void, socket_transport_disconnect, SOCKET_TRANSPORT_HANDLE, socket_transport);

MOCKABLE_FUNCTION(, SOCKET_ACCEPT_RESULT, socket_transport_accept, SOCKET_TRANSPORT_HANDLE, socket_transport, SOCKET_TRANSPORT_HANDLE*, accepted_socket);

MOCKABLE_FUNCTION(, SOCKET_SEND_RESULT, socket_transport_send, SOCKET_TRANSPORT_HANDLE, socket_transport, SOCKET_BUFFER*, payload, uint32_t, buffer_count, uint32_t*, bytes_sent, uint32_t, flags, void*, data);
MOCKABLE_FUNCTION(, SOCKET_RECEIVE_RESULT, socket_transport_receive, SOCKET_TRANSPORT_HANDLE, socket_transport, SOCKET_BUFFER*, payload, uint32_t, buffer_count, uint32_t*, bytes_recv, uint32_t, flags, void*, data);

MOCKABLE_FUNCTION(, SOCKET_HANDLE, socket_transport_get_underlying_socket, SOCKET_TRANSPORT_HANDLE, socket_transport);
MOCKABLE_FUNCTION(, bool, socket_transport_is_valid_socket, SOCKET_TRANSPORT_HANDLE, socket_transport_handle);
```

### socket_transport_create_client

```c
MOCKABLE_FUNCTION(, SOCKET_TRANSPORT_HANDLE, socket_transport_create_client);
```

`socket_transport_create_client` creates a client socket transport.

**SOCKET_TRANSPORT_LINUX_11_002: [** `socket_transport_create_client` shall allocate a new `SOCKET_TRANSPORT` object. **]**

**SOCKET_TRANSPORT_LINUX_11_003: [** `socket_transport_create_client` shall call `sm_create` to create a sm object with the type set to SOCKET_CLIENT. **]**

**SOCKET_TRANSPORT_LINUX_11_004: [** On any failure `socket_transport_create_client` shall return `NULL`. **]**

**SOCKET_TRANSPORT_LINUX_11_005: [** On success `socket_transport_create_client` shall return `SOCKET_TRANSPORT_HANDLE`. **]**

### socket_transport_create_server

```c
MOCKABLE_FUNCTION(, SOCKET_TRANSPORT_HANDLE, socket_transport_create_server);
```

`socket_transport_create_server` creates a server socket transport.

**SOCKET_TRANSPORT_LINUX_11_079: [** `socket_transport_create_server` shall allocate a new `SOCKET_TRANSPORT` object. **]**

**SOCKET_TRANSPORT_LINUX_11_080: [** `socket_transport_create_server` shall call `sm_create` to create a sm object with the type set to SOCKET_BINDING. **]**

**SOCKET_TRANSPORT_LINUX_11_081: [** On any failure `socket_transport_create_server` shall return `NULL`. **]**

**SOCKET_TRANSPORT_LINUX_11_082: [** On success `socket_transport_create_server` shall return `SOCKET_TRANSPORT_HANDLE`. **]**

### socket_transport_create_from_socket

```c
MOCKABLE_FUNCTION(, SOCKET_TRANSPORT_HANDLE, socket_transport_create_from_socket, SOCKET_HANDLE, socket_handle);
```

`socket_transport_create_from_socket` creates a client socket transport from a given socket handle.

**SOCKET_TRANSPORT_LINUX_11_086: [** If socket_handle is an INVALID_SOCKET, `socket_transport_create_from_socket` shall fail and return `NULL`. **]**

**SOCKET_TRANSPORT_LINUX_11_087: [** `socket_transport_create_from_socket` shall allocate a new SOCKET_TRANSPORT object. **]**

**SOCKET_TRANSPORT_LINUX_11_088: [** `socket_transport_create_from_socket` shall call sm_create to create a sm_object with the type set to SOCKET_CLIENT. **]**

**SOCKET_TRANSPORT_LINUX_11_097: [** If `sm_open_begin` does not return `SM_EXEC_GRANTED`, `socket_transport_create_from_socket` shall fail and return `NULL`. **]**

**SOCKET_TRANSPORT_LINUX_11_096: [** `socket_transport_create_from_socket` shall assign the socket_handle to the new allocated socket transport. **]**

**SOCKET_TRANSPORT_LINUX_11_090: [** On any failure `socket_transport_create_from_socket` shall return `NULL`. **]**

**SOCKET_TRANSPORT_LINUX_11_091: [** On success `socket_transport_create_from_socket` shall return SOCKET_TRANSPORT_HANDLE. **]**

### socket_transport_destroy

```c
MOCKABLE_FUNCTION(, void, socket_transport_destroy, SOCKET_TRANSPORT_HANDLE, socket_transport);
```

`socket_transport_destroy` destroys all data stored in the `SOCKET_TRANSPORT_HANDLE` object.

**SOCKET_TRANSPORT_LINUX_11_006: [** If `socket_transport` is `NULL` `socket_transport_destroy` shall return. **]**

**SOCKET_TRANSPORT_LINUX_11_007: [** `socket_transport_destroy` shall call `sm_destroy` to destroy the sm object. **]**

**SOCKET_TRANSPORT_LINUX_11_008: [** `socket_transport_destroy` shall free the `SOCKET_TRANSPORT_HANDLE` object. **]**

### socket_transport_connect

```c
MOCKABLE_FUNCTION(, int, socket_transport_connect, SOCKET_TRANSPORT_HANDLE, socket_transport, const char*, hostname, uint16_t, port, uint32_t, connection_timeout_ms);
```

`socket_transport_connect` shall connect to a specified endpoint.

**SOCKET_TRANSPORT_LINUX_11_009: [** If `socket_transport` is `NULL`, `socket_transport_connect` shall fail and return a non-zero value. **]**

**SOCKET_TRANSPORT_LINUX_11_010: [** If `hostname` is `NULL`, `socket_transport_connect` shall fail and return a non-zero value. **]**

**SOCKET_TRANSPORT_LINUX_11_011: [** If `port` is `0`, `socket_transport_connect` shall fail and return a non-zero value. **]**

**SOCKET_TRANSPORT_LINUX_11_012: [** If the `socket_transport` is not `SOCKET_CLIENT`, `socket_transport_connect` shall fail and return a non-zero value. **]**

**SOCKET_TRANSPORT_LINUX_11_013: [** `socket_transport_connect` shall call `sm_open_begin` to begin the open. **]**

**SOCKET_TRANSPORT_LINUX_11_014: [** If `sm_open_begin` does not return `SM_EXEC_GRANTED`, `socket_transport_connect` shall fail and return a non-zero value. **]**

**SOCKET_TRANSPORT_LINUX_11_015: [** `socket_transport_connect` shall call `socket` with the params `AF_INET`, `SOCK_STREAM` and `0`. **]**

**SOCKET_TRANSPORT_LINUX_11_016: [** `socket_transport_connect` shall call `connect` to connect to the endpoint. **]**

**SOCKET_TRANSPORT_LINUX_11_017: [** `socket_transport_connect` shall set the socket to non-blocking by calling `fcntl` with `O_NONBLOCK`. **]**

**SOCKET_TRANSPORT_LINUX_11_018: [** If successful `socket_transport_connect` shall call `sm_open_end` with `true`. **]**

**SOCKET_TRANSPORT_LINUX_11_019: [** If any failure is encountered, `socket_transport_connect` shall call `sm_open_end` with `false`, fail and return a non-zero value. **]**

### socket_transport_disconnect

```c
MOCKABLE_FUNCTION(, void, socket_transport_disconnect, SOCKET_TRANSPORT_HANDLE, socket_transport);
```

`socket_transport_disconnect` shall disconnect a connected socked from its endpoint.

**SOCKET_TRANSPORT_LINUX_11_020: [** If `socket_transport` is `NULL`, `socket_transport_disconnect` shall fail and return. **]**

**SOCKET_TRANSPORT_LINUX_11_021: [** `socket_transport_disconnect` shall call `sm_close_begin` to begin the closing process. **]**

**SOCKET_TRANSPORT_LINUX_11_022: [** If `sm_close_begin` does not return `SM_EXEC_GRANTED`, `socket_transport_disconnect` shall fail and return. **]**

**SOCKET_TRANSPORT_LINUX_11_025: [** `socket_transport_disconnect` shall call `shutdown` to stop both the transmit and reception of the connected socket. **]**

**SOCKET_TRANSPORT_LINUX_11_026: [** If `shutdown` does not return 0, the socket is not valid therefore `socket_transport_disconnect` shall not call 'close' **]**

**SOCKET_TRANSPORT_LINUX_11_023: [** `socket_transport_disconnect` shall call `close` to disconnect the connected socket. **]**

**SOCKET_TRANSPORT_LINUX_11_024: [** `socket_transport_disconnect` shall call `sm_close_end`. **]**

### socket_transport_send

```c
MOCKABLE_FUNCTION(, SOCKET_SEND_RESULT, socket_transport_send, SOCKET_TRANSPORT_HANDLE, socket_transport, SOCKET_BUFFER*, payload, uint32_t, buffer_count, uint32_t*, bytes_sent, uint32_t, flags, void*, data);
```

**SOCKET_TRANSPORT_LINUX_11_027: [** If `socket_transport` is `NULL`, `socket_transport_send` shall fail and return `SOCKET_SEND_INVALID_ARG`. **]**

**SOCKET_TRANSPORT_LINUX_11_028: [** If `payload` is `NULL`, `socket_transport_send` shall fail and return `SOCKET_SEND_INVALID_ARG`. **]**

**SOCKET_TRANSPORT_LINUX_11_029: [** If `buffer_count` is `0`, `socket_transport_send` shall fail and return `SOCKET_SEND_INVALID_ARG`. **]**

**SOCKET_TRANSPORT_LINUX_11_030: [** `socket_transport_send` shall call `sm_exec_begin`. **]**

**SOCKET_TRANSPORT_LINUX_11_031: [** If `sm_exec_begin` does not return `SM_EXEC_GRANTED`, `socket_transport_send` shall fail and return `SOCKET_SEND_ERROR`. **]**

**SOCKET_TRANSPORT_LINUX_11_032: [** For each buffer count in payload `socket_transport_send` shall call `send` to send data with `flags` as a parameter. **]**

**SOCKET_TRANSPORT_LINUX_11_033: [** If `send` returns a value less then 0, `socket_transport_send` shall stop sending and return `SOCKET_SEND_FAILED`. **]**

- **SOCKET_TRANSPORT_LINUX_11_034: [** If the errno is equal to `ECONNRESET`, `socket_transport_send` shall return `SOCKET_SEND_SHUTDOWN`. **]**

**SOCKET_TRANSPORT_LINUX_11_035: [** Otherwise `socket_transport_send` shall continue calling send until the `SOCKET_BUFFER` length is reached. **]**

**SOCKET_TRANSPORT_LINUX_11_036: [** If `bytes_sent` is not `NULL`, `socket_transport_send` shall set `bytes_sent` the total bytes sent. **]**

**SOCKET_TRANSPORT_LINUX_11_037: [** `socket_transport_send` shall call `sm_exec_end`. **]**

### socket_transport_receive

```c
MOCKABLE_FUNCTION(, SOCKET_RECEIVE_RESULT, socket_transport_receive, SOCKET_TRANSPORT_HANDLE, socket_transport, SOCKET_BUFFER*, payload, uint32_t, buffer_count, uint32_t*, bytes_recv, uint32_t, flags, void*, data);
```

**SOCKET_TRANSPORT_LINUX_11_038: [** If `socket_transport` is `NULL`, `socket_transport_receive` shall fail and return `SOCKET_RECEIVE_INVALID_ARG`. **]**

**SOCKET_TRANSPORT_LINUX_11_039: [** If `payload` is `NULL`, `socket_transport_receive` shall fail and return `SOCKET_RECEIVE_INVALID_ARG`. **]**

**SOCKET_TRANSPORT_LINUX_11_040: [** If `buffer_count` is `0`, `socket_transport_receive` shall fail and return `SOCKET_RECEIVE_INVALID_ARG`. **]**

**SOCKET_TRANSPORT_LINUX_11_041: [** `socket_transport_receive` shall call `sm_exec_begin`. **]**

**SOCKET_TRANSPORT_LINUX_11_042: [** If `sm_exec_begin` does not return `SM_EXEC_GRANTED`, `socket_transport_receive` shall fail and return `SOCKET_RECEIVE_ERROR`. **]**

**SOCKET_TRANSPORT_LINUX_11_043: [** For each buffer count in payload `socket_transport_receive` shall call `recv` with the `flags` parameter. **]**

**SOCKET_TRANSPORT_LINUX_11_044: [** If `recv` a value less then 0, `socket_transport_receive` shall do the following: **]**

- **SOCKET_TRANSPORT_LINUX_11_045: [** If `errno` is `EAGAIN` or `EWOULDBLOCK`, `socket_transport_receive` shall break out of loop and return `SOCKET_RECEIVE_WOULD_BLOCK`. **]**

- **SOCKET_TRANSPORT_LINUX_11_046: [** If `errno` is `ECONNRESET`, `socket_transport_receive` shall break out of the loop and return `SOCKET_RECEIVE_SHUTDOWN`. **]**

- **SOCKET_TRANSPORT_LINUX_11_047: [** else `socket_transport_receive` shall break out of the looop and return `SOCKET_RECEIVE_ERROR`. **]**

**SOCKET_TRANSPORT_LINUX_11_048: [** If `recv` returns a `0` value, `socket_transport_receive` shall break and return `SOCKET_RECEIVE_SHUTDOWN`. **]**

**SOCKET_TRANSPORT_LINUX_11_049: [** Else `socket_transport_receive` shall do the following: **]**

- **SOCKET_TRANSPORT_LINUX_11_050: [** `socket_transport_receive` shall test that the total recv size will not overflow. **]**

- **SOCKET_TRANSPORT_LINUX_11_051: [** `socket_transport_receive` shall store the received byte size. **]**

**SOCKET_TRANSPORT_LINUX_11_052: [** If `bytes_recv` is not `NULL`, `socket_transport_send` shall set `bytes_recv` the total bytes received. **]**

**SOCKET_TRANSPORT_LINUX_11_053: [** `socket_transport_receive` shall call `sm_exec_end`. **]**

### socket_transport_listen

```c
MOCKABLE_FUNCTION(, int, socket_transport_listen, SOCKET_TRANSPORT_HANDLE, socket_transport, uint16_t, port);
```

`socket_transport_listen` is to listen for incoming connections.

**SOCKET_TRANSPORT_LINUX_11_054: [** If `socket_transport` is `NULL`, `socket_transport_listen` shall fail and return a non-zero value. **]**

**SOCKET_TRANSPORT_LINUX_11_055: [** If `port` is `0`, `socket_transport_listen` shall fail and return a non-zero value. **]**

**SOCKET_TRANSPORT_LINUX_11_056: [** If the transport type is not `SOCKET_BINDING`, `socket_transport_listen` shall fail and return a non-zero value. **]**

**SOCKET_TRANSPORT_LINUX_11_057: [** `socket_transport_listen` shall call `sm_open_begin` to begin the open. **]**

**SOCKET_TRANSPORT_LINUX_11_058: [** If `sm_open_begin` does not return `SM_EXEC_GRANTED`, `socket_transport_listen` shall fail and return a non-zero value. **]**

**SOCKET_TRANSPORT_LINUX_11_059: [** `socket_transport_listen` shall call `socket` with the params `AF_INET`, `SOCK_STREAM` and `IPPROTO_TCP`. **]**

**SOCKET_TRANSPORT_LINUX_11_083: [** `socket_transport_listen` shall set the `SO_REUSEADDR` option on the socket. **]**

**SOCKET_TRANSPORT_LINUX_11_060: [** `socket_transport_listen` shall bind to the socket by calling `bind`. **]**

**SOCKET_TRANSPORT_LINUX_11_061: [** `socket_transport_listen` shall start listening to incoming connection by calling `listen`. **]**

**SOCKET_TRANSPORT_LINUX_11_062: [** If successful `socket_transport_listen` shall call `sm_open_end` with `true`. **]**

**SOCKET_TRANSPORT_LINUX_11_063: [** If any failure is encountered, `socket_transport_listen` shall call `sm_open_end` with `false`, fail and return a non-zero value. **]**

### socket_transport_accept

```c
MOCKABLE_FUNCTION(, SOCKET_ACCEPT_RESULT, socket_transport_accept, SOCKET_TRANSPORT_HANDLE, socket_transport, SOCKET_TRANSPORT_HANDLE*, accepted_socket);
```

`socket_transport_accept` accepts the incoming connections.

**SOCKET_TRANSPORT_LINUX_11_069: [** If `socket_transport` is `NULL`, `socket_transport_accept` shall fail and return `SOCKET_ACCEPT_ERROR`. **]**

**SOCKET_TRANSPORT_LINUX_11_070: [** If the transport type is not `SOCKET_BINDING`, `socket_transport_accept` shall fail and return `SOCKET_ACCEPT_ERROR`. **]**

**SOCKET_TRANSPORT_LINUX_11_071: [** `socket_transport_accept` shall call `sm_exec_begin`. **]**

**SOCKET_TRANSPORT_LINUX_11_072: [** If `sm_exec_begin` does not return `SM_EXEC_GRANTED`, `socket_transport_accept` shall fail and return `SOCKET_ACCEPT_ERROR`. **]**

**SOCKET_TRANSPORT_LINUX_11_073: [** `socket_transport_accept` shall call `accept` to accept the incoming socket connection. **]**

**SOCKET_TRANSPORT_LINUX_11_084: [** If `errno` is `EAGAIN` or `EWOULDBLOCK`, socket_transport_accept shall return `SOCKET_ACCEPT_NO_CONNECTION`. **]**

**SOCKET_TRANSPORT_LINUX_11_074: [** `socket_transport_accept` shall set the incoming socket to non-blocking. **]**

**SOCKET_TRANSPORT_LINUX_11_075: [** `socket_transport_accept` shall allocate a `SOCKET_TRANSPORT` for the incoming connection and call `sm_create` and `sm_open` on the connection. **]**

**SOCKET_TRANSPORT_LINUX_11_076: [** If successful `socket_transport_accept` shall assign accepted_socket to be the allocated incoming `SOCKET_TRANSPORT` and return `SOCKET_ACCEPT_OK`. **]**

**SOCKET_TRANSPORT_LINUX_11_077: [** If any failure is encountered, `socket_transport_accept` shall fail and return `SOCKET_ACCEPT_ERROR`. **]**

**SOCKET_TRANSPORT_LINUX_11_078: [** `socket_transport_accept` shall call `sm_exec_end`. **]**

### socket_transport_get_underlying_socket

```c
MOCKABLE_FUNCTION(, SOCKET_HANDLE, socket_transport_get_underlying_socket, SOCKET_TRANSPORT_HANDLE, socket_transport);
```

`socket_transport_get_underlying_socket` returns the underlying socket.

**SOCKET_TRANSPORT_LINUX_11_064: [** If `socket_transport` is `NULL`, `socket_transport_get_underlying_socket` shall fail and return `INVALID_SOCKET`. **]**

**SOCKET_TRANSPORT_LINUX_11_065: [** `socket_transport_get_underlying_socket` shall call `sm_exec_begin`. **]**

**SOCKET_TRANSPORT_LINUX_11_066: [** If `sm_exec_begin` does not return `SM_EXEC_GRANTED`, `socket_transport_get_underlying_socket` shall fail and return `INVALID_SOCKET`. **]**

**SOCKET_TRANSPORT_LINUX_11_067: [** `socket_transport_get_underlying_socket` shall return the `SOCKET_HANDLE` socket value. **]**

**SOCKET_TRANSPORT_LINUX_11_068: [** `socket_transport_get_underlying_socket` shall call `sm_exec_end`. **]**

### socket_transport_is_valid_socket

```c
MOCKABLE_FUNCTION(, bool, socket_transport_is_valid_socket, SOCKET_TRANSPORT_HANDLE, socket_transport_handle);
```

`socket_transport_is_valid_socket` checks that the internal socket is valid.

**SOCKET_TRANSPORT_LINUX_11_093: [** If `socket_transport_handle` is `NULL`, `socket_transport_is_valid_socket` shall fail and return `false`. **]**

**SOCKET_TRANSPORT_LINUX_11_094: [** If the socket inside `socket_transport_handle` is an `INVALID_SOCKET`, `socket_transport_is_valid_socket` shall fail and return `false`. **]**

**SOCKET_TRANSPORT_LINUX_11_095: [** On success, `socket_transport_is_valid_socket` shall return `true`. **]**