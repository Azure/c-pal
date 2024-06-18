# socket transport Win32

## Overview

socket_transport_win32 is the module that abstracts the socket transport for the windows platform

The `SOCKET_TYPE` determines which API are applicable for this instance of socket_transport.  The following chart shows which API's are allowed under which type:

| API                                    | Allowable Socket Type
|----------------------------------------|----------------------
| socket_transport_create                | BOTH
| socket_transport_destroy               | BOTH
| socket_transport_connect               | SOCKET_CLIENT
| socket_transport_listen                | SOCKET_SERVER
| socket_transport_disconnect            | BOTH
| socket_transport_accept                | SOCKET_SERVER
| socket_transport_send                  | BOTH
| socket_transport_receive               | BOTH
| socket_transport_get_underlying_socket | BOTH

## Exposed API

```c
typedef struct SOCKET_TRANSPORT_TAG* SOCKET_TRANSPORT_HANDLE;

#define SOCKET_SEND_RESULT_VALUES \
    SOCKET_SEND_OK, \
    SOCKET_SEND_ERROR, \
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

`socket_transport_create` shall ensure `type` is either `SOCKET_CLIENT`, or `SOCKET_SERVER`.

`socket_transport_create` shall allocate a new `SOCKET_TRANSPORT` object.

`socket_transport_create` shall call `sm_create` to create a sm object.

On any failure `socket_transport_create` shall return `NULL`.

On success `socket_transport_create` shall return `SOCKET_TRANSPORT_HANDLE`.

### socket_transport_destroy

```c
MOCKABLE_FUNCTION(, void, socket_transport_destroy, SOCKET_TRANSPORT_HANDLE, socket_transport);
```

`socket_transport_destroy` destroys all data stored in the `SOCKET_TRANSPORT_HANDLE` object.

If `socket_transport` is `NULL` `socket_transport_destroy` shall return.

`socket_transport_destroy` shall call `sm_destroy` to destroy the sm object.

`socket_transport_destroy` shall free the `SOCKET_TRANSPORT_HANDLE` object.

### socket_transport_connect

```c
MOCKABLE_FUNCTION(, int, socket_transport_connect, SOCKET_TRANSPORT_HANDLE, socket_transport, const char*, hostname, uint16_t, port, uint32_t, connection_timeout_ms);
```

`socket_transport_connect` shall connect to a specified endpoint.

If `socket_transport` is `NULL`, `socket_transport_connect` shall fail and return a non-zero value.

If `hostname` is `NULL`, `socket_transport_connect` shall fail and return a non-zero value.

If `port` is `0`, `socket_transport_connect` shall fail and return a non-zero value.

If the `socket_transport` type is not `SOCKET_CLIENT`, `socket_transport_connect` shall fail and return a non-zero value.

`socket_transport_connect` shall call `sm_open_begin` to begin the open.

If `sm_open_begin` does not return `SM_EXEC_GRANTED`, `socket_transport_connect` shall fail and return a non-zero value.

`socket_transport_connect` shall call `socket` with the params `AF_INET`, `SOCK_STREAM` and `IPPROTO_TCP`.

`socket_transport_connect` shall call `connect_to_endpoint` with the `connection_timeout_ms` to connect to the endpoint.

If successful `socket_transport_connect` shall call `sm_open_end` with `true`.

If any failure is encountered, `socket_transport_connect` shall call `sm_open_end` with `false`, fail and return a non-zero value.

### connect_to_endpoint

```c
static int connect_to_endpoint(SOCKET client_socket, const ADDRINFO* addrInfo, uint32_t connection_timeout_ms)
```

`connect_to_endpoint` shall call the `connect` API to connect to the endpoint.

`connect_to_endpoint` shall call `connect` using `connection_timeout_ms` as a timeout for the connection.

If the `connect` call fails, `connect_to_endpoint` shall check to `WSAGetLastError` for `WSAEWOULDBLOCK`.

On `WSAEWOULDBLOCK` `connect_to_endpoint` shall call the `select` API with the `connection_timeout_ms` and check the return value:

- If the return is `SOCKET_ERROR`, this indicates a failure and `connect_to_endpoint` shall fail.

- If the return value is 0, this indicates a timeout and `connect_to_endpoint` shall fail.

- Any other value this indicates a possible success and and `connect_to_endpoint` shall test if the socket is writable by calling `FD_ISSET`.

If the socket is writable `connect_to_endpoint` shall succeed and return a 0 value.

If any error is encountered `connect_to_endpoint` shall return a non-zero value.

### socket_transport_disconnect

```c
MOCKABLE_FUNCTION(, void, socket_transport_disconnect, SOCKET_TRANSPORT_HANDLE, socket_transport);
```

`socket_transport_disconnect` shall disconnect a connected socked from its endpoint.

If `socket_transport` is `NULL`, `socket_transport_disconnect` shall fail and return.

`socket_transport_disconnect` shall call `sm_close_begin` to begin the closing process.

If `sm_close_begin` does not return `SM_EXEC_GRANTED`, `socket_transport_disconnect` shall fail and return.

`socket_transport_disconnect` shall call `closesocket` to disconnect the connected socket.

`socket_transport_disconnect` shall call `sm_close_end`.

### socket_transport_send

```c
MOCKABLE_FUNCTION(, SOCKET_SEND_RESULT, socket_transport_send, SOCKET_TRANSPORT_HANDLE, socket_transport, SOCKET_BUFFER*, payload, uint32_t, buffer_count, uint32_t*, bytes_written, uint32_t, flags, void*, overlapped_data);
```

If `socket_transport` is `NULL`, `socket_transport_send` shall fail and return `SOCKET_SEND_ERROR`.

If `payload` is `NULL`, `socket_transport_send` shall fail and return `SOCKET_SEND_ERROR`.

If `buffer_count` is `0`, `socket_transport_send` shall fail and return `SOCKET_SEND_ERROR`.

`socket_transport_send` shall call `sm_exec_begin`.

If `sm_exec_begin` does not return `SM_EXEC_GRANTED`, `socket_transport_send` shall fail and return `SOCKET_SEND_ERROR`.

`socket_transport_send` shall call `WSASend` to send data with `flags` and the `overlapped_data`.

If `WSASend` returns 0, `socket_transport_send` shall store the bytes written in `bytes_written` (if non-NULL) and return `SOCKET_SEND_OK`.

Otherwise `socket_transport_send` shall return `SOCKET_SEND_FAILED`.

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

`socket_transport_receive` shall call `WSARecv` with the `payload`, `flags` and the `data` which is used as overlapped object.

If `WSARecv` return 0, `socket_transport_receive` shall do the following:

- If `bytes_recv` is not `NULL`, `socket_transport_receive` shall copy the number of bytes into `bytes_recv`.

- `socket_transport_receive` shall return `SOCKET_RECEIVE_OK`.

If `WSARecv` returns an non-zero value, `socket_transport_receive` shall do the following:

- If `WSAGetLastError` returns `WSA_IO_PENDING`, and `bytes_recv` is not `NULL`, `socket_transport_receive` shall set `bytes_recv` to 0.

  - `socket_transport_receive` shall return `SOCKET_RECEIVE_WOULD_BLOCK`.

- If `WSAGetLastError` does not returns `WSA_IO_PENDING` `socket_transport_receive` shall return `SOCKET_RECEIVE_ERROR`.

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

If `socket_transport` is `NULL`, `socket_transport_accept` shall fail and return a non-zero value.

If the transport type is not `SOCKET_SERVER`, `socket_transport_accept` shall fail and return a non-zero value.

`socket_transport_accept` shall call `sm_exec_begin`.

If `sm_exec_begin` does not return `SM_EXEC_GRANTED`, `socket_transport_accept` shall fail and return `SOCKET_SEND_ERROR`.

`socket_transport_accept` shall call `select` determine if the socket is ready to be read passing a timeout of 10 milliseconds.

`socket_transport_accept` shall call `accept` to accept the incoming socket connection.

`socket_transport_accept` shall allocate a `SOCKET_TRANSPORT` for the incoming connection and call `sm_create` and `sm_open` on the connection.

If successful `socket_transport_accept` shall return the allocated `SOCKET_TRANSPORT`.

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
