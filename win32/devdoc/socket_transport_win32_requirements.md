# socket transport Win32

## Overview

socket_transport_win32 is the module that abstracts the socket transport for the windows platform

## References

- [socket transport requirements](../../interfaces/devdoc/socket_transport_requirements.md)

## Exposed API

```c
typedef struct SOCKET_TRANSPORT_TAG* SOCKET_TRANSPORT_HANDLE;

#define SOCKET_SEND_RESULT_VALUES \
    SOCKET_SEND_OK, \
    SOCKET_SEND_INVALID_ARG, \
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

#define SOCKET_ACCEPT_RESULT_VALUES \
    SOCKET_ACCEPT_OK, \
    SOCKET_ACCEPT_ERROR, \
    SOCKET_ACCEPT_NO_CONNECTION

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

**SOCKET_TRANSPORT_WIN32_09_002: [** `socket_transport_create_client` shall allocate a new `SOCKET_TRANSPORT` object. **]**

**SOCKET_TRANSPORT_WIN32_09_003: [** `socket_transport_create_client` shall call `sm_create` to create a sm object with the type set to SOCKET_CLIENT. **]**

**SOCKET_TRANSPORT_WIN32_09_004: [** On any failure `socket_transport_create_client` shall return `NULL`. **]**

**SOCKET_TRANSPORT_WIN32_09_005: [** On success `socket_transport_create_client` shall return `SOCKET_TRANSPORT_HANDLE`. **]**

### socket_transport_create_server

```c
MOCKABLE_FUNCTION(, SOCKET_TRANSPORT_HANDLE, socket_transport_create_server);
```

`socket_transport_create_server` creates a server socket transport.

**SOCKET_TRANSPORT_WIN32_09_087: [** `socket_transport_create_server` shall allocate a new `SOCKET_TRANSPORT` object. **]**

**SOCKET_TRANSPORT_WIN32_09_088: [** `socket_transport_create_server` shall call `sm_create` to create a sm object with the type set to SOCKET_BINDING. **]**

**SOCKET_TRANSPORT_WIN32_09_089: [** On any failure `socket_transport_create_server` shall return `NULL`. **]**

**SOCKET_TRANSPORT_WIN32_09_090: [** On success `socket_transport_create_server` shall return `SOCKET_TRANSPORT_HANDLE`. **]**

### socket_transport_create_from_socket

```c
MOCKABLE_FUNCTION(, SOCKET_TRANSPORT_HANDLE, socket_transport_create_from_socket, SOCKET_HANDLE, socket_handle);
```

`socket_transport_create_from_socket` creates a client socket transport from a given socket handle.

**SOCKET_TRANSPORT_WIN32_09_096: [** If socket_handle is an INVALID_SOCKET, `socket_transport_create_from_socket` shall fail and return `NULL`. **]**

**SOCKET_TRANSPORT_WIN32_09_097: [** `socket_transport_create_from_socket` shall allocate a new SOCKET_TRANSPORT object. **]**

**SOCKET_TRANSPORT_WIN32_09_098: [** `socket_transport_create_from_socket` shall call sm_create to create a sm_object with the type set to SOCKET_CLIENT. **]**

**SOCKET_TRANSPORT_WIN32_09_014: [** If `sm_open_begin` does not return `SM_EXEC_GRANTED`, `socket_transport_create_from_socket` shall fail and return `NULL`. **]**

**SOCKET_TRANSPORT_WIN32_09_099: [** `socket_transport_create_from_socket` shall assign the socket_handle to the new allocated socket transport. **]**

**SOCKET_TRANSPORT_WIN32_09_100: [** On any failure `socket_transport_create_from_socket` shall return `NULL`. **]**

**SOCKET_TRANSPORT_WIN32_09_101: [** On success `socket_transport_create_from_socket` shall return SOCKET_TRANSPORT_HANDLE. **]**


### socket_transport_destroy

```c
MOCKABLE_FUNCTION(, void, socket_transport_destroy, SOCKET_TRANSPORT_HANDLE, socket_transport);
```

`socket_transport_destroy` destroys all data stored in the `SOCKET_TRANSPORT_HANDLE` object.

**SOCKET_TRANSPORT_WIN32_09_006: [** If `socket_transport` is `NULL` `socket_transport_destroy` shall return. **]**

**SOCKET_TRANSPORT_WIN32_09_007: [** `socket_transport_destroy` shall call `sm_destroy` to destroy the sm object. **]**

**SOCKET_TRANSPORT_WIN32_09_008: [** `socket_transport_destroy` shall free the `SOCKET_TRANSPORT_HANDLE` object. **]**

### socket_transport_connect

```c
MOCKABLE_FUNCTION(, int, socket_transport_connect, SOCKET_TRANSPORT_HANDLE, socket_transport, const char*, hostname, uint16_t, port, uint32_t, connection_timeout_ms);
```

`socket_transport_connect` shall connect to a specified endpoint.

**SOCKET_TRANSPORT_WIN32_09_009: [** If `socket_transport` is `NULL`, `socket_transport_connect` shall fail and return a non-zero value. **]**

**SOCKET_TRANSPORT_WIN32_09_010: [** If `hostname` is `NULL`, `socket_transport_connect` shall fail and return a non-zero value. **]**

**SOCKET_TRANSPORT_WIN32_09_011: [** If `port` is `0`, `socket_transport_connect` shall fail and return a non-zero value. **]**

**SOCKET_TRANSPORT_WIN32_09_012: [** If the `socket_transport` type is not `SOCKET_CLIENT`, `socket_transport_connect` shall fail and return a non-zero value. **]**

**SOCKET_TRANSPORT_WIN32_09_013: [** `socket_transport_connect` shall call `sm_open_begin` to begin the open. **]**

**SOCKET_TRANSPORT_WIN32_09_014: [** If `sm_open_begin` does not return `SM_EXEC_GRANTED`, `socket_transport_connect` shall fail and return a non-zero value. **]**

**SOCKET_TRANSPORT_WIN32_09_015: [** `socket_transport_connect` shall call `socket` with the params `AF_INET`, `SOCK_STREAM` and `IPPROTO_TCP`. **]**

**SOCKET_TRANSPORT_WIN32_09_016: [** `socket_transport_connect` shall call `connect_to_endpoint` with the `connection_timeout_ms` to connect to the endpoint. **]**

**SOCKET_TRANSPORT_WIN32_09_017: [** If successful `socket_transport_connect` shall call `sm_open_end` with `true`. **]**

**SOCKET_TRANSPORT_WIN32_09_018: [** If any failure is encountered, `socket_transport_connect` shall call `sm_open_end` with `false`, fail and return a non-zero value. **]**

### connect_to_endpoint

```c
static int connect_to_endpoint(SOCKET client_socket, const ADDRINFO* addrInfo, uint32_t connection_timeout_ms)
```

`connect_to_endpoint` shall call the `connect` API to connect to the endpoint.

**SOCKET_TRANSPORT_WIN32_09_019: [** `connect_to_endpoint` shall call `connect` using `connection_timeout_ms` as a timeout for the connection. **]**

**SOCKET_TRANSPORT_WIN32_09_020: [** If the `connect` call fails, `connect_to_endpoint` shall check to `WSAGetLastError` for `WSAEWOULDBLOCK`. **]**

**SOCKET_TRANSPORT_WIN32_09_021: [** On `WSAEWOULDBLOCK` `connect_to_endpoint` shall call the `select` API with the `connection_timeout_ms` and check the return value: **]**

- **SOCKET_TRANSPORT_WIN32_09_022: [** If the return is `SOCKET_ERROR`, this indicates a failure and `connect_to_endpoint` shall fail. **]**

- **SOCKET_TRANSPORT_WIN32_09_023: [** If the return value is 0, this indicates a timeout and `connect_to_endpoint` shall fail. **]**

- **SOCKET_TRANSPORT_WIN32_09_024: [** Any other value this indicates a possible success and `connect_to_endpoint` shall test if the socket is writable by calling `FD_ISSET`. **]**

**SOCKET_TRANSPORT_WIN32_09_025: [** If the socket is writable `connect_to_endpoint` shall succeed and return a 0 value. **]**

**SOCKET_TRANSPORT_WIN32_09_026: [** If any error is encountered `connect_to_endpoint` shall return a non-zero value. **]**

### socket_transport_disconnect

```c
MOCKABLE_FUNCTION(, void, socket_transport_disconnect, SOCKET_TRANSPORT_HANDLE, socket_transport);
```

`socket_transport_disconnect` shall disconnect a connected socked from its endpoint.

**SOCKET_TRANSPORT_WIN32_09_027: [** If `socket_transport` is `NULL`, `socket_transport_disconnect` shall fail and return. **]**

**SOCKET_TRANSPORT_WIN32_09_028: [** `socket_transport_disconnect` shall call `sm_close_begin` to begin the closing process. **]**

**SOCKET_TRANSPORT_WIN32_09_029: [** If `sm_close_begin` does not return `SM_EXEC_GRANTED`, `socket_transport_disconnect` shall fail and return. **]**

**SOCKET_TRANSPORT_WIN32_09_083: [** If `shutdown` does not return 0 on a socket that is not a binding socket, the socket is not valid therefore `socket_transport_disconnect` shall not call `close` **]**

**SOCKET_TRANSPORT_WIN32_09_030: [** `socket_transport_disconnect` shall call `closesocket` to disconnect the connected socket. **]**

**SOCKET_TRANSPORT_WIN32_09_031: [** `socket_transport_disconnect` shall call `sm_close_end`. **]**

### socket_transport_send

```c
MOCKABLE_FUNCTION(, SOCKET_SEND_RESULT, socket_transport_send, SOCKET_TRANSPORT_HANDLE, socket_transport, SOCKET_BUFFER*, payload, uint32_t, buffer_count, uint32_t*, bytes_written, uint32_t, flags, void*, overlapped_data);
```

**SOCKET_TRANSPORT_WIN32_09_032: [** If `socket_transport` is `NULL`, `socket_transport_send` shall fail and return `SOCKET_SEND_INVALID_ARG`. **]**

**SOCKET_TRANSPORT_WIN32_09_033: [** If `payload` is `NULL`, `socket_transport_send` shall fail and return `SOCKET_SEND_INVALID_ARG`. **]**

**SOCKET_TRANSPORT_WIN32_09_034: [** If `buffer_count` is `0`, `socket_transport_send` shall fail and return `SOCKET_SEND_INVALID_ARG`. **]**

**SOCKET_TRANSPORT_WIN32_09_035: [** `socket_transport_send` shall call `sm_exec_begin`. **]**

**SOCKET_TRANSPORT_WIN32_09_036: [** If `sm_exec_begin` does not return `SM_EXEC_GRANTED`, `socket_transport_send` shall fail and return `SOCKET_SEND_ERROR`. **]**

**SOCKET_TRANSPORT_WIN32_09_037: [** `socket_transport_send` shall call `WSASend` to send data with `flags` and the `overlapped_data`. **]**

**SOCKET_TRANSPORT_WIN32_09_038: [** If `WSASend` returns 0, `socket_transport_send` shall store the bytes written in `bytes_written` (if non-NULL) and return `SOCKET_SEND_OK`. **]**

**SOCKET_TRANSPORT_WIN32_09_039: [** Otherwise `socket_transport_send` shall return `SOCKET_SEND_FAILED`. **]**

**SOCKET_TRANSPORT_WIN32_09_040: [** `socket_transport_send` shall call `sm_exec_end`. **]**

### socket_transport_receive

```c
MOCKABLE_FUNCTION(, SOCKET_RECEIVE_RESULT, socket_transport_receive, SOCKET_TRANSPORT_HANDLE, socket_transport, SOCKET_BUFFER*, payload, uint32_t, buffer_count, uint32_t*, bytes_recv, uint32_t, flags, void*, data);
```

**SOCKET_TRANSPORT_WIN32_09_041: [** If `socket_transport` is `NULL`, `socket_transport_receive` shall fail and return `SOCKET_RECEIVE_INVALID_ARG`. **]**

**SOCKET_TRANSPORT_WIN32_09_042: [** If `payload` is `NULL`, `socket_transport_receive` shall fail and return `SOCKET_RECEIVE_INVALID_ARG`. **]**

**SOCKET_TRANSPORT_WIN32_09_043: [** If `buffer_count` is `0`, `socket_transport_receive` shall fail and return `SOCKET_RECEIVE_INVALID_ARG`. **]**

**SOCKET_TRANSPORT_WIN32_09_044: [** `socket_transport_receive` shall call `sm_exec_begin`. **]**

**SOCKET_TRANSPORT_WIN32_09_045: [** If `sm_exec_begin` does not return `SM_EXEC_GRANTED`, `socket_transport_receive` shall fail and return `SOCKET_RECEIVE_ERROR`. **]**

**SOCKET_TRANSPORT_WIN32_09_046: [** `socket_transport_receive` shall call `WSARecv` with the `payload`, `flags` and the `data` which is used as overlapped object. **]**

**SOCKET_TRANSPORT_WIN32_09_047: [** If `WSARecv` return 0, `socket_transport_receive` shall do the following: **]**

- **SOCKET_TRANSPORT_WIN32_09_048: [** If `bytes_recv` is not `NULL`, `socket_transport_receive` shall copy the number of bytes into `bytes_recv`. **]**

- **SOCKET_TRANSPORT_WIN32_09_049: [** `socket_transport_receive` shall return `SOCKET_RECEIVE_OK`. **]**

**SOCKET_TRANSPORT_WIN32_09_050: [** If `WSARecv` returns an non-zero value, `socket_transport_receive` shall do the following: **]**

- **SOCKET_TRANSPORT_WIN32_09_051: [** If `WSAGetLastError` returns `WSA_IO_PENDING`, and `bytes_recv` is not `NULL`, `socket_transport_receive` shall set `bytes_recv` to 0. **]**

  - **SOCKET_TRANSPORT_WIN32_09_052: [** `socket_transport_receive` shall return `SOCKET_RECEIVE_WOULD_BLOCK`. **]**

- **SOCKET_TRANSPORT_WIN32_09_053: [** If `WSAGetLastError` does not returns `WSA_IO_PENDING` `socket_transport_receive` shall return `SOCKET_RECEIVE_ERROR`. **]**

**SOCKET_TRANSPORT_WIN32_09_054: [** `socket_transport_receive` shall call `sm_exec_end`. **]**

### socket_transport_listen

```c
MOCKABLE_FUNCTION(, int, socket_transport_listen, SOCKET_TRANSPORT_HANDLE, socket_transport, uint16_t, port);
```

`socket_transport_listen` is to listen for incoming connections.

**SOCKET_TRANSPORT_WIN32_09_055: [** If `socket_transport` is `NULL`, `socket_transport_listen` shall fail and return a non-zero value. **]**

**SOCKET_TRANSPORT_WIN32_09_056: [** If `port` is `0`, `socket_transport_listen` shall fail and return a non-zero value. **]**

**SOCKET_TRANSPORT_WIN32_09_057: [** If the transport type is not `SOCKET_BINDING`, `socket_transport_listen` shall fail and return a non-zero value. **]**

**SOCKET_TRANSPORT_WIN32_09_058: [** `socket_transport_listen` shall call `sm_open_begin` to begin the open. **]**

**SOCKET_TRANSPORT_WIN32_09_059: [** If `sm_open_begin` does not return `SM_EXEC_GRANTED`, `socket_transport_listen` shall fail and return a non-zero value. **]**

**SOCKET_TRANSPORT_WIN32_09_060: [** `socket_transport_listen` shall call `socket` with the params `AF_INET`, `SOCK_STREAM` and `IPPROTO_TCP`. **]**

**SOCKET_TRANSPORT_WIN32_09_061: [** `socket_transport_listen` shall bind to the socket by calling `bind`. **]**

**SOCKET_TRANSPORT_WIN32_09_065: [** `sock_transport_listen` shall set listening socket in non-blocking mode by calling `ioctlsocket`. **]**

**SOCKET_TRANSPORT_WIN32_09_062: [** `socket_transport_listen` shall start listening to incoming connection by calling `listen`. **]**

**SOCKET_TRANSPORT_WIN32_09_063: [** If successful `socket_transport_listen` shall call `sm_open_end` with `true`. **]**

**SOCKET_TRANSPORT_WIN32_09_066: [** `socket_transport_listen` shall call `closesocket`. **]**

**SOCKET_TRANSPORT_WIN32_09_064: [** If any failure is encountered, `socket_transport_listen` shall call `sm_open_end` with `false`, fail and return a non-zero value. **]**

### socket_transport_accept

```c
MOCKABLE_FUNCTION(, SOCKET_ACCEPT_RESULT, socket_transport_accept, SOCKET_TRANSPORT_HANDLE, socket_transport, SOCKET_TRANSPORT_HANDLE*, accepted_socket);
```

`socket_transport_accept` accepts the incoming connections.

**SOCKET_TRANSPORT_WIN32_09_067: [** If `socket_transport` is `NULL`, `socket_transport_accept` shall fail and return `SOCKET_ACCEPT_ERROR`. **]**

**SOCKET_TRANSPORT_WIN32_09_068: [** If the transport type is not `SOCKET_BINDING`, `socket_transport_accept` shall fail and return `SOCKET_ACCEPT_ERROR`. **]**

**SOCKET_TRANSPORT_WIN32_09_069: [** `socket_transport_accept` shall call `sm_exec_begin`. **]**

**SOCKET_TRANSPORT_WIN32_09_070: [** If `sm_exec_begin` does not return `SM_EXEC_GRANTED`, `socket_transport_accept` shall fail and return `SOCKET_ACCEPT_ERROR`. **]**

**SOCKET_TRANSPORT_WIN32_09_071: [** `socket_transport_accept` shall call `select` determine if the socket is ready to be read passing a timeout of 10 milliseconds. **]**

**SOCKET_TRANSPORT_WIN32_09_091: [** If `select` returns zero, socket_transport_accept shall set accepted_socket to `NULL` and return `SOCKET_ACCEPT_NO_CONNECTION`. **]**

**SOCKET_TRANSPORT_WIN32_09_072: [** `socket_transport_accept` shall call `accept` to accept the incoming socket connection. **]**

**SOCKET_TRANSPORT_WIN32_09_073: [** If `accept` returns an INVALID_SOCKET, `socket_transport_accept` shall fail and return `SOCKET_ACCEPT_ERROR`. **]**

**SOCKET_TRANSPORT_WIN32_09_074: [** `socket_transport_accept` shall allocate a `SOCKET_TRANSPORT` for the incoming connection and call `sm_create` and `sm_open` on the connection. **]**

**SOCKET_TRANSPORT_WIN32_09_084: [** If `malloc` fails, `socket_transport_accept` shall fail and return `SOCKET_ACCEPT_ERROR`. **]**

**SOCKET_TRANSPORT_WIN32_09_085: [** If `sm_create` fails, `socket_transport_accept` shall close the incoming socket, fail, and return `SOCKET_ACCEPT_ERROR`. **]**

**SOCKET_TRANSPORT_WIN32_09_086: [** If `sm_open_begin` fails, `socket_transport_accept` shall close the incoming socket, fail, and return `SOCKET_ACCEPT_ERROR` **]**

**SOCKET_TRANSPORT_WIN32_09_075: [** If successful `socket_transport_accept` shall return `SOCKET_ACCEPT_OK`. **]**

**SOCKET_TRANSPORT_WIN32_09_076: [** If any failure is encountered, `socket_transport_accept` shall fail and return `SOCKET_ACCEPT_ERROR`. **]**

**SOCKET_TRANSPORT_WIN32_09_077: [** `socket_transport_accept` shall call `sm_exec_end`. **]**

### socket_transport_get_underlying_socket

```c
MOCKABLE_FUNCTION(, SOCKET_HANDLE, socket_transport_get_underlying_socket, SOCKET_TRANSPORT_HANDLE, socket_transport);
```

`socket_transport_get_underlying_socket` returns the underlying socket.

**SOCKET_TRANSPORT_WIN32_09_078: [** If `socket_transport` is `NULL`, `socket_transport_get_underlying_socket` shall fail and return `INVALID_SOCKET`. **]**

**SOCKET_TRANSPORT_WIN32_09_079: [** `socket_transport_get_underlying_socket` shall call `sm_exec_begin`. **]**

**SOCKET_TRANSPORT_WIN32_09_080: [** If `sm_exec_begin` does not return `SM_EXEC_GRANTED`, `socket_transport_get_underlying_socket` shall fail and return `INVALID_SOCKET`. **]**

**SOCKET_TRANSPORT_WIN32_09_081: [** `socket_transport_get_underlying_socket` shall return the SOCKET_TRANSPORT socket value. **]**

**SOCKET_TRANSPORT_WIN32_09_082: [** `socket_transport_get_underlying_socket` shall call `sm_exec_end`. **]**

### socket_transport_is_valid_socket

```c
MOCKABLE_FUNCTION(, bool, socket_transport_is_valid_socket, SOCKET_TRANSPORT_HANDLE, socket_transport_handle);
```

**SOCKET_TRANSPORT_WIN32_09_092: [** `socket_transport_is_valid_socket` checks that the internal socket is valid. **]**

**SOCKET_TRANSPORT_WIN32_09_093: [** If `socket_transport_handle` is `NULL`, `socket_transport_is_valid_socket` shall fail and return false. **]**

**SOCKET_TRANSPORT_WIN32_09_094: [** If the socket inside `socket_transport_handle` is an `INVALID_SOCKET`, `socket_transport_is_valid_socket` shall fail and return false. **]**

**SOCKET_TRANSPORT_WIN32_09_095: [** On success, `socket_transport_is_valid_socket` shall return true. **]**
