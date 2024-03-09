# `async_socket` requirements

## Overview

`async_socket` is an interface for an asynchronous socket API on the underlying platform.

## Design

The `async_socket` takes ownership of the provided `SOCKET_HANDLE` and is responsible for closing it. For that reason, it is not possible to re-open the `async_socket` after closing it. This is due to the fact that the socket is created by some external event, such as accepting an incoming connection and closing the underlying socket is not reversible.

## Exposed API

```c
typedef struct ASYNC_SOCKET_TAG* ASYNC_SOCKET_HANDLE;

#define ASYNC_SOCKET_OPEN_RESULT_VALUES \
    ASYNC_SOCKET_OPEN_OK, \
    ASYNC_SOCKET_OPEN_ERROR

MU_DEFINE_ENUM(ASYNC_SOCKET_OPEN_RESULT, ASYNC_SOCKET_OPEN_RESULT_VALUES)

#define ASYNC_SOCKET_SEND_SYNC_RESULT_VALUES \
    ASYNC_SOCKET_SEND_SYNC_OK, \
    ASYNC_SOCKET_SEND_SYNC_ERROR, \
    ASYNC_SOCKET_SEND_SYNC_NOT_OPEN

MU_DEFINE_ENUM(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_RESULT_VALUES)

#define ASYNC_SOCKET_SEND_RESULT_VALUES \
    ASYNC_SOCKET_SEND_OK, \
    ASYNC_SOCKET_SEND_ERROR, \
    ASYNC_SOCKET_SEND_ABANDONED

MU_DEFINE_ENUM(ASYNC_SOCKET_SEND_RESULT, ASYNC_SOCKET_SEND_RESULT_VALUES)

#define ASYNC_SOCKET_RECEIVE_RESULT_VALUES \
    ASYNC_SOCKET_RECEIVE_OK, \
    ASYNC_SOCKET_RECEIVE_ERROR, \
    ASYNC_SOCKET_RECEIVE_ABANDONED

MU_DEFINE_ENUM(ASYNC_SOCKET_RECEIVE_RESULT, ASYNC_SOCKET_RECEIVE_RESULT_VALUES)

#define ASYNC_SOCKET_NOTIFY_IO_TYPE_VALUES \
    ASYNC_SOCKET_NOTIFY_IO_TYPE_IN, \
    ASYNC_SOCKET_NOTIFY_IO_TYPE_OUT

MU_DEFINE_ENUM(ASYNC_SOCKET_NOTIFY_IO_TYPE, ASYNC_SOCKET_NOTIFY_IO_TYPE_VALUES)

#define ASYNC_SOCKET_NOTIFY_IO_RESULT_VALUES \
    ASYNC_SOCKET_NOTIFY_IO_RESULT_IN, \
    ASYNC_SOCKET_NOTIFY_IO_RESULT_OUT, \
    ASYNC_SOCKET_NOTIFY_IO_RESULT_ABANDONED, \
    ASYNC_SOCKET_NOTIFY_IO_RESULT_ERROR

MU_DEFINE_ENUM(ASYNC_SOCKET_NOTIFY_IO_RESULT, ASYNC_SOCKET_NOTIFY_IO_RESULT_VALUES)

typedef void (*ON_ASYNC_SOCKET_OPEN_COMPLETE)(void* context, ASYNC_SOCKET_OPEN_RESULT open_result);
typedef void (*ON_ASYNC_SOCKET_SEND_COMPLETE)(void* context, ASYNC_SOCKET_SEND_RESULT send_result);
typedef void (*ON_ASYNC_SOCKET_RECEIVE_COMPLETE)(void* context, ASYNC_SOCKET_RECEIVE_RESULT receive_result, uint32_t bytes_received);
typedef void (*ON_ASYNC_SOCKET_NOTIFY_IO_COMPLETE)(void* context, ASYNC_SOCKET_NOTIFY_IO_RESULT notify_io_result);
typedef int (*ON_ASYNC_SOCKET_SEND)(void* context, ASYNC_SOCKET_HANDLE async_socket, const void* buf, size_t len);
typedef int (*ON_ASYNC_SOCKET_RECV)(void* context, ASYNC_SOCKET_HANDLE async_socket, void* buf, size_t len);

typedef struct ASYNC_SOCKET_BUFFER_TAG
{
    void* buffer;
    uint32_t length;
} ASYNC_SOCKET_BUFFER;

MOCKABLE_FUNCTION(, ASYNC_SOCKET_HANDLE, async_socket_create, EXECUTION_ENGINE_HANDLE, execution_engine, SOCKET_HANDLE, socket_handle);
MOCKABLE_FUNCTION(, ASYNC_SOCKET_HANDLE, async_socket_create_with_transport, EXECUTION_ENGINE_HANDLE, execution_engine, SOCKET_HANDLE, socket_handle, ON_ASYNC_SOCKET_SEND, on_send, void*, on_send_context, ON_ASYNC_SOCKET_RECV, on_recv, void*, on_recv_context);
MOCKABLE_FUNCTION(, void, async_socket_destroy, ASYNC_SOCKET_HANDLE, async_socket);

MOCKABLE_FUNCTION(, int, async_socket_open_async, ASYNC_SOCKET_HANDLE, async_socket, ON_ASYNC_SOCKET_OPEN_COMPLETE, on_open_complete, void*, on_open_complete_context);
MOCKABLE_FUNCTION(, void, async_socket_close, ASYNC_SOCKET_HANDLE, async_socket);
MOCKABLE_FUNCTION(, ASYNC_SOCKET_SEND_SYNC_RESULT, async_socket_send_async, ASYNC_SOCKET_HANDLE, async_socket, const ASYNC_SOCKET_BUFFER*, payload, uint32_t, buffer_count, ON_ASYNC_SOCKET_SEND_COMPLETE, on_send_complete, void*, on_send_complete_context);
MOCKABLE_FUNCTION(, int, async_socket_receive_async, ASYNC_SOCKET_HANDLE, async_socket, ASYNC_SOCKET_BUFFER*, payload, uint32_t, buffer_count, ON_ASYNC_SOCKET_RECEIVE_COMPLETE, on_receive_complete, void*, on_receive_complete_context);
MOCKABLE_FUNCTION(, int, async_socket_notify_io_async, ASYNC_SOCKET_HANDLE, async_socket, ASYNC_SOCKET_NOTIFY_IO_TYPE, io_type, ON_ASYNC_SOCKET_NOTIFY_IO_COMPLETE, on_notify_io_complete, void*, on_notify_io_complete_context);
```

### ON_ASYNC_SOCKET_SEND

```c
typedef int (*ON_ASYNC_SOCKET_SEND)(void* context, ASYNC_SOCKET_HANDLE async_socket, const void* buf, size_t len);
```

The `ON_ASYNC_SOCKET_SEND` callback is called to handle sending data on the socket. It must return the size of data that was successfully written to the socket, or -1 in case of error.

When this function fails with -1, an error code may be set using the platform-specific error code mechanism (such as `errno` on Linux).

### ON_ASYNC_SOCKET_RECV

```c
typedef int (*ON_ASYNC_SOCKET_RECV)(void* context, ASYNC_SOCKET_HANDLE async_socket, void* buf, size_t len);
```

The `ON_ASYNC_SOCKET_RECV` callback is called to handle receiving data from the socket. It must return the size of data that was successfully read from the socket, or -1 in case of error.

When this function fails with -1, an error code may be set using the platform-specific error code mechanism (such as `errno` on Linux).

### async_socket_create_with_transport

```c
MOCKABLE_FUNCTION(, ASYNC_SOCKET_HANDLE, async_socket_create_with_transport, EXECUTION_ENGINE_HANDLE, execution_engine, SOCKET_HANDLE, socket_handle, ON_ASYNC_SOCKET_SEND, on_send, void*, on_send_context, ON_ASYNC_SOCKET_RECV, on_recv, void*, on_recv_context);
```

`async_socket_create_with_transport` allows overriding the send and receive functions for the socket. This may not be available on all platforms.

### async_socket_create

```c
MOCKABLE_FUNCTION(, ASYNC_SOCKET_HANDLE, async_socket_create, EXECUTION_ENGINE_HANDLE, execution_engine, SOCKET_HANDLE, socket_handle);
```

`async_socket_create` creates an async socket.

### async_socket_destroy

```c
MOCKABLE_FUNCTION(, void, async_socket_destroy, ASYNC_SOCKET_HANDLE, async_socket);
```

`async_socket_destroy` frees all resources associated with `async_socket`.

### async_socket_open_async

```c
MOCKABLE_FUNCTION(, int, async_socket_open_async, ASYNC_SOCKET_HANDLE, async_socket, ON_ASYNC_SOCKET_OPEN_COMPLETE, on_open_complete, void*, on_open_complete_context);
```

`async_socket_open_async` opens the async socket.

### async_socket_close

```c
MOCKABLE_FUNCTION(, void, async_socket_close, ASYNC_SOCKET_HANDLE, async_socket);
```

`async_socket_close` closes an open `async_socket`. Note that it is not possible to re-open a socket after closing it as it relies on underlying platform state which has been setup prior to the creating of this module.

### async_socket_send_async

```c
MOCKABLE_FUNCTION(, ASYNC_SOCKET_SEND_SYNC_RESULT, async_socket_send_async, ASYNC_SOCKET_HANDLE, async_socket, const ASYNC_SOCKET_BUFFER*, payload, uint32_t, buffer_count, ON_ASYNC_SOCKET_SEND_COMPLETE, on_send_complete, void*, on_send_complete_context);
```

`async_socket_send_async` sends a number of buffers asynchronously.

### async_socket_receive_async

```c
MOCKABLE_FUNCTION(, int, async_socket_receive_async, ASYNC_SOCKET_HANDLE, async_socket, ASYNC_SOCKET_BUFFER*, payload, uint32_t, buffer_count, ON_SOCKET_ASYNC_RECEIVE_COMPLETE, on_receive_complete, void*, on_receive_complete_context);
```

`async_socket_receive_async` receives in a number of buffers asynchronously.

### async_socket_notify_io_async

```c
MOCKABLE_FUNCTION(, int, async_socket_notify_io_async, ASYNC_SOCKET_HANDLE, async_socket, ASYNC_SOCKET_NOTIFY_IO_TYPE, io_type, ON_ASYNC_SOCKET_NOTIFY_IO_COMPLETE, on_notify_io_complete, void*, on_notify_io_complete_context);
```

`async_socket_notify_io_async` is used only for the Linux platform.
