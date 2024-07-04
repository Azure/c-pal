# socket transport

## Overview

socket_transport is the module that abstracts the socket transport.

The `SOCKET_TYPE` determines which API are applicable for this instance of socket_transport.  The following chart shows which API's are allowed under which type:

| API                                    | Allowable Socket Type
|----------------------------------------|----------------------
| socket_transport_create_client         | SOCKET_CLIENT
| socket_transport_create_server         | SOCKET_BINDING
| socket_transport_destroy               | BOTH
| socket_transport_connect               | SOCKET_CLIENT
| socket_transport_listen                | SOCKET_BINDING
| socket_transport_disconnect            | BOTH
| socket_transport_accept                | SOCKET_BINDING
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
    SOCKET_BINDING

MU_DEFINE_ENUM(SOCKET_TYPE, SOCKET_TYPE_VALUES)

typedef struct SOCKET_BUFFER_TAG
{
    uint32_t length;
    void* buffer;
} SOCKET_BUFFER;

MOCKABLE_FUNCTION(, SOCKET_TRANSPORT_HANDLE, socket_transport_create_client);
MOCKABLE_FUNCTION(, SOCKET_TRANSPORT_HANDLE, socket_transport_create_server);
MOCKABLE_FUNCTION(, void, socket_transport_destroy, SOCKET_TRANSPORT_HANDLE, socket_transport);

MOCKABLE_FUNCTION(, int, socket_transport_connect, SOCKET_TRANSPORT_HANDLE, socket_transport, const char*, hostname, uint16_t, port, uint32_t, connection_timeout_ms);
MOCKABLE_FUNCTION(, int, socket_transport_listen, SOCKET_TRANSPORT_HANDLE, socket_transport, uint16_t, port);
MOCKABLE_FUNCTION(, void, socket_transport_disconnect, SOCKET_TRANSPORT_HANDLE, socket_transport);

MOCKABLE_FUNCTION(, SOCKET_TRANSPORT_HANDLE, socket_transport_accept, SOCKET_TRANSPORT_HANDLE, socket_transport);

MOCKABLE_FUNCTION(, SOCKET_SEND_RESULT, socket_transport_send, SOCKET_TRANSPORT_HANDLE, socket_transport, SOCKET_BUFFER*, payload, uint32_t, buffer_count, uint32_t*, bytes_sent, uint32_t, flags, void*, data);
MOCKABLE_FUNCTION(, SOCKET_RECEIVE_RESULT, socket_transport_receive, SOCKET_TRANSPORT_HANDLE, socket_transport, SOCKET_BUFFER*, payload, uint32_t, buffer_count, uint32_t*, bytes_recv, uint32_t, flags, void*, data);

MOCKABLE_FUNCTION(, SOCKET_HANDLE, socket_transport_get_underlying_socket, SOCKET_TRANSPORT_HANDLE, socket_transport);
```

### socket_transport_create_client

```c
MOCKABLE_FUNCTION(, SOCKET_TRANSPORT_HANDLE, socket_transport_create_client);
```

`socket_transport_create_client` creates a client socket transport.

### socket_transport_create_server

```c
MOCKABLE_FUNCTION(, SOCKET_TRANSPORT_HANDLE, socket_transport_create_server);
```

`socket_transport_create_server` creates a binding socket transport.

### socket_transport_destroy

```c
MOCKABLE_FUNCTION(, void, socket_transport_destroy, SOCKET_TRANSPORT_HANDLE, socket_transport);
```

`socket_transport_destroy` destroys all data stored in the `SOCKET_TRANSPORT_HANDLE` object.

### socket_transport_connect

```c
MOCKABLE_FUNCTION(, int, socket_transport_connect, SOCKET_TRANSPORT_HANDLE, socket_transport, const char*, hostname, uint16_t, port, uint32_t, connection_timeout_ms);
```

`socket_transport_connect` shall connect to a specified endpoint.

### socket_transport_disconnect

```c
MOCKABLE_FUNCTION(, void, socket_transport_disconnect, SOCKET_TRANSPORT_HANDLE, socket_transport);
```

`socket_transport_disconnect` shall disconnect a connected socked from its endpoint.

### socket_transport_send

```c
MOCKABLE_FUNCTION(, SOCKET_SEND_RESULT, socket_transport_send, SOCKET_TRANSPORT_HANDLE, socket_transport, SOCKET_BUFFER*, payload, uint32_t, buffer_count, uint32_t*, bytes_written, uint32_t, flags, void*, data);
```

`socket_transport_send` sends data to the connected endpoint.  The parameter `flags` is passed through to the send API.

### socket_transport_receive

```c
MOCKABLE_FUNCTION(, SOCKET_RECEIVE_RESULT, socket_transport_receive, SOCKET_TRANSPORT_HANDLE, socket_transport, SOCKET_BUFFER*, payload, uint32_t, buffer_count, uint32_t*, bytes_recv, uint32_t, flags, void*, data);
```

`socket_transport_receive` recieves data from a connected endpoint.  The parameter `flags` is passed through to the recieve API.


### socket_transport_listen

```c
MOCKABLE_FUNCTION(, int, socket_transport_listen, SOCKET_TRANSPORT_HANDLE, socket_transport, uint16_t, port);
```

`socket_transport_listen` is to listen for incoming connections.

### socket_transport_accept

```c
MOCKABLE_FUNCTION(, SOCKET_TRANSPORT_HANDLE, socket_transport_accept, SOCKET_TRANSPORT_HANDLE, socket_transport);
```

`socket_transport_accept` accepts the incoming connections.

### socket_transport_get_underlying_socket

```c
MOCKABLE_FUNCTION(, SOCKET_HANDLE, socket_transport_get_underlying_socket, SOCKET_TRANSPORT_HANDLE, socket_transport);
```

`socket_transport_get_underlying_socket` returns the underlying socket.
