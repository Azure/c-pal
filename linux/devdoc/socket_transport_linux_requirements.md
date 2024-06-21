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
    SOCKET_RECEIVE_OK, \
    SOCKET_RECEIVE_WOULD_BLOCK, \
    SOCKET_RECEIVE_ERROR, \
    SOCKET_RECEIVE_SHUTDOWN

MU_DEFINE_ENUM(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_RESULT_VALUES)

#define SOCKET_TYPE_VALUES \
    SOCKET_CLIENT, \
    SOCKET_SERVER

MU_DEFINE_ENUM(SOCKET_TYPE, SOCKET_TYPE_VALUES)

typedef struct SOCKET_BUFFER_TAG
{
    uint32_t length;
    void* buffer;
} SOCKET_BUFFER;

MOCKABLE_FUNCTION(, SOCKET_TRANSPORT_HANDLE, socket_transport_create, SOCKET_TYPE, type);
MOCKABLE_FUNCTION(, void, socket_transport_destroy, SOCKET_TRANSPORT_HANDLE, socket_transport);

MOCKABLE_FUNCTION(, int, socket_transport_connect, SOCKET_TRANSPORT_HANDLE, socket_transport, const char*, hostname, uint16_t, port, uint32_t, connection_timeout_ms);
MOCKABLE_FUNCTION(, int, socket_transport_listen, SOCKET_TRANSPORT_HANDLE, socket_transport, uint16_t, port);
MOCKABLE_FUNCTION(, void, socket_transport_disconnect, SOCKET_TRANSPORT_HANDLE, socket_transport);

MOCKABLE_FUNCTION(, SOCKET_TRANSPORT_HANDLE, socket_transport_accept, SOCKET_TRANSPORT_HANDLE, socket_transport);

MOCKABLE_FUNCTION(, SOCKET_SEND_RESULT, socket_transport_send, SOCKET_TRANSPORT_HANDLE, socket_transport, SOCKET_BUFFER*, payload, uint32_t, buffer_count, uint32_t*, bytes_sent, uint32_t, flags, void*, data);
MOCKABLE_FUNCTION(, SOCKET_RECEIVE_RESULT, socket_transport_receive, SOCKET_TRANSPORT_HANDLE, socket_transport, SOCKET_BUFFER*, payload, uint32_t, buffer_count, uint32_t*, bytes_recv, uint32_t, flags, void*, data);

MOCKABLE_FUNCTION(, SOCKET_HANDLE, socket_transport_get_underlying_socket, SOCKET_TRANSPORT_HANDLE, socket_transport);
```

### socket_transport_create

```c
MOCKABLE_FUNCTION(, SOCKET_TRANSPORT_HANDLE, socket_transport_create, SOCKET_TYPE, type);
```

`socket_transport_create` creates a socket transport.

**SOCKET_TRANSPORT_LINUX_11_001: [** `socket_transport_create` shall ensure `type` is either `SOCKET_CLIENT`, or `SOCKET_SERVER`. **]**

**SOCKET_TRANSPORT_LINUX_11_002: [** `socket_transport_create` shall allocate a new `SOCKET_TRANSPORT` object. **]**

**SOCKET_TRANSPORT_LINUX_11_003: [** `socket_transport_create` shall call `sm_create` to create a sm object. **]**

**SOCKET_TRANSPORT_LINUX_11_004: [** On any failure `socket_transport_create` shall return `NULL`. **]**

**SOCKET_TRANSPORT_LINUX_11_005: [** On success `socket_transport_create` shall return `SOCKET_TRANSPORT_HANDLE`. **]**

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

`socket_transport_disconnect` shall call `sm_close_begin` to begin the closing process.

If `sm_close_begin` does not return `SM_EXEC_GRANTED`, `socket_transport_disconnect` shall fail and return.

`socket_transport_disconnect` shall call `shutdown` and `close` to disconnect the connected socket.

`socket_transport_disconnect` shall call `sm_close_end`.

### socket_transport_send

```c
MOCKABLE_FUNCTION(, SOCKET_SEND_RESULT, socket_transport_send, SOCKET_TRANSPORT_HANDLE, socket_transport, SOCKET_BUFFER*, payload, uint32_t, buffer_count, uint32_t*, bytes_sent, uint32_t, flags, void*, data);
```

If `socket_transport` is `NULL`, `socket_transport_send` shall fail and return `SOCKET_SEND_INVALID_ARG`.

If `payload` is `NULL`, `socket_transport_send` shall fail and return `SOCKET_SEND_INVALID_ARG`.

If `buffer_count` is `0`, `socket_transport_send` shall fail and return `SOCKET_SEND_INVALID_ARG`.

`socket_transport_send` shall call `sm_exec_begin`.

If `sm_exec_begin` does not return `SM_EXEC_GRANTED`, `socket_transport_send` shall fail and return `SOCKET_SEND_ERROR`.

For each buffer count in payload `socket_transport_send` shall call `send` to send data with `flags` as a parameter.

If `send` returns a value less then 0, `socket_transport_send` shall stop sending and return `SOCKET_SEND_FAILED`.

- If the errno is equal to `ECONNRESET`, `socket_transport_send` shall return `SOCKET_SEND_SHUTDOWN`.

Otherwise `socket_transport_send` shall continue calling send until the `SOCKET_BUFFER` length is reached.

If `bytes_sent` is not `NULL`, `socket_transport_send` shall set `bytes_sent` the total bytes sent.

`socket_transport_send` shall call `sm_exec_end`.

### socket_transport_receive

```c
MOCKABLE_FUNCTION(, SOCKET_RECEIVE_RESULT, socket_transport_receive, SOCKET_TRANSPORT_HANDLE, socket_transport, SOCKET_BUFFER*, payload, uint32_t, buffer_count, uint32_t*, bytes_recv, uint32_t, flags, void*, data);
```

If `socket_transport` is `NULL`, `socket_transport_receive` shall fail and return `SOCKET_RECEIVE_ERROR`.

If `payload` is `NULL`, `socket_transport_receive` shall fail and return `SOCKET_RECEIVE_ERROR`.

If `buffer_count` is `0`, `socket_transport_receive` shall fail and return `SOCKET_RECEIVE_ERROR`.

`socket_transport_receive` shall call `sm_exec_begin`.

If `sm_exec_begin` does not return `SM_EXEC_GRANTED`, `socket_transport_receive` shall fail and return `SOCKET_RECEIVE_ERROR`.

For each buffer count in payload `socket_transport_receive` shall call `recv` with the `flags` parameter.

If `recv` a value less then 0, `socket_transport_receive` shall do the following:

- If `errno` is `EAGAIN` or `EWOULDBLOCK`, `socket_transport_receive` shall break out of loop and return `SOCKET_RECEIVE_WOULD_BLOCK`.

- If `errno` is `ECONNRESET`, `socket_transport_receive` shall break out of the loop and return `SOCKET_RECEIVE_SHUTDOWN`.

- else `socket_transport_receive` shall break out of the looop and return `SOCKET_RECEIVE_ERROR`.

If `recv` returns a `0` value, `socket_transport_receive` shall break and return `SOCKET_RECEIVE_SHUTDOWN`.

Else `socket_transport_receive` shall do the following:

- `socket_transport_receive` shall test that the total recv size will not overflow.

- `socket_transport_receive` shall store the received byte size.

If `bytes_recv` is not `NULL`, `socket_transport_send` shall set `bytes_recv` the total bytes received.

`socket_transport_receive` shall call `sm_exec_end`.

### socket_transport_listen

```c
MOCKABLE_FUNCTION(, int, socket_transport_listen, SOCKET_TRANSPORT_HANDLE, socket_transport, uint16_t, port);
```

`socket_transport_listen` is to listen for incoming connections.

If `socket_transport` is `NULL`, `socket_transport_listen` shall fail and return a non-zero value.

If `port` is `0`, `socket_transport_listen` shall fail and return a non-zero value.

If the transport type is not `SOCKET_SERVER`, `socket_transport_listen` shall fail and return a non-zero value.

`socket_transport_listen` shall call `sm_open_begin` to begin the open.

If `sm_open_begin` does not return `SM_EXEC_GRANTED`, `socket_transport_listen` shall fail and return a non-zero value.

`socket_transport_listen` shall call `socket` with the params `AF_INET`, `SOCK_STREAM` and `IPPROTO_TCP`.

`socket_transport_listen` shall bind to the socket by calling `bind`.

`socket_transport_listen` shall start listening to incoming connection by calling `listen`.

If successful `socket_transport_listen` shall call `sm_open_end` with `true`.

If any failure is encountered, `socket_transport_listen` shall call `sm_open_end` with `false`, fail and return a non-zero value.

### socket_transport_accept

```c
MOCKABLE_FUNCTION(, SOCKET_TRANSPORT_HANDLE, socket_transport_accept, SOCKET_TRANSPORT_HANDLE, socket_transport);
```

`socket_transport_accept` accepts the incoming connections.

If `socket_transport` is `NULL`, `socket_transport_accept` shall fail and return `NULL`.

If the transport type is not `SOCKET_SERVER`, `socket_transport_accept` shall fail and return `NULL`.

`socket_transport_accept` shall call `sm_exec_begin`.

If `sm_exec_begin` does not return `SM_EXEC_GRANTED`, `socket_transport_accept` shall fail and return `NULL`.

`socket_transport_accept` shall call `accept` to accept the incoming socket connection.

`socket_transport_accept` shall set the incoming socket to non-blocking.

`socket_transport_accept` shall allocate a `SOCKET_TRANSPORT` for the incoming connection and call `sm_create` and `sm_open` on the connection.

If successful `socket_transport_accept` shall return the allocated `SOCKET_TRANSPORT` of type SOCKET_DATA.

If any failure is encountered, `socket_transport_accept` shall fail and return `NULL`.

`socket_transport_accept` shall call `sm_exec_end`.

### socket_transport_get_underlying_socket

```c
MOCKABLE_FUNCTION(, SOCKET_HANDLE, socket_transport_get_underlying_socket, SOCKET_TRANSPORT_HANDLE, socket_transport);
```

`socket_transport_get_underlying_socket` returns the underlying socket.

If `socket_transport` is `NULL`, `socket_transport_get_underlying_socket` shall fail and return `INVALID_SOCKET`.

`socket_transport_get_underlying_socket` shall call `sm_exec_begin`.

If `sm_exec_begin` does not return `SM_EXEC_GRANTED`, `socket_transport_get_underlying_socket` shall fail and return `INVALID_SOCKET`.

`socket_transport_get_underlying_socket` shall return the SOCKET_TRANSPORT socket value.

`socket_transport_get_underlying_socket` shall call `sm_exec_end`.
