# net_comm_interface requirements

## Overview

The `net_comm_interface` module provides an interface for network communications for multiple platforms.

## Exposed API

```c
typedef void(*ON_BYTES_RECEIVED)(void* context, const unsigned char* buffer, size_t size);
typedef void(*ON_SEND_COMPLETE)(void* context, NET_SEND_RESULT send_result, size_t bytes_sent);
typedef void(*ON_OPEN_COMPLETE)(void* context, NET_OPEN_RESULT open_result);
typedef void(*ON_CLOSE_COMPLETE)(void* context);
typedef void(*ON_ERROR)(void* context, NET_ERROR_RESULT error_result);
typedef void(*ON_INCOMING_CONNECT)(void* context, const void* config);

typedef struct COMM_CALLBACK_INFO_TAG
{
    ON_OPEN_COMPLETE on_open_complete;
    void* on_open_complete_ctx;
    ON_CLOSE_COMPLETE on_close_complete;
    void* on_close_complete_ctx;
    ON_SEND_COMPLETE on_send_complete;
    void* on_send_complete_ctx;
    ON_BYTES_RECEIVED on_bytes_received;
    void* on_bytes_received_ctx;
    ON_ERROR on_error;
    void* on_error_ctx;
    ON_INCOMING_CONNECT on_incoming_conn;
    void* on_incoming_conn_ctx;
} COMM_CALLBACK_INFO;

typedef COMM_HANDLE(*COMM_CREATE)(const void* io_create_parameters, const COMM_CALLBACK_INFO* client_cb);
typedef void(*COMM_DESTROY)(COMM_HANDLE impl_handle);
typedef int(*COMM_OPEN)(COMM_HANDLE impl_handle);
typedef int(*COMM_CLOSE)(COMM_HANDLE impl_handle);
typedef int(*COMM_SEND)(COMM_HANDLE impl_handle, const void* buffer, size_t size);
typedef int(*COMM_SEND_NOTIFY)(COMM_HANDLE impl_handle, const void* buffer, uint32_t size, uint32_t flags, void* notify_info);
typedef int(*COMM_RECV)(COMM_HANDLE impl_handle);
typedef int(*COMM_RECV_NOTIFY)(COMM_HANDLE impl_handle, void* buffer, uint32_t size, uint32_t* flags, void* notify_info);
typedef SOCKET_HANDLE(*COMM_UNDERLYING_HANDLE)(COMM_HANDLE impl_handle);

typedef int(*COMM_LISTEN)(COMM_HANDLE impl_handle);
typedef int(*COMM_ACCEPT_CONN)(COMM_HANDLE impl_handle);
typedef COMM_HANDLE(*COMM_ACCEPT_NOTIFY)(COMM_HANDLE impl_handle, const COMM_CALLBACK_INFO* client_cb);

typedef struct COMM_INTERFACE_DESCRIPTION_TAG
{
    COMM_CREATE interface_impl_create;
    COMM_DESTROY interface_impl_destroy;
    COMM_OPEN interface_impl_open;
    COMM_CLOSE interface_impl_close;
    COMM_SEND interface_impl_send;
    COMM_SEND_NOTIFY interface_impl_send_notify;
    COMM_RECV interface_impl_recv;
    COMM_RECV_NOTIFY interface_impl_recv_notify;
    COMM_UNDERLYING_HANDLE interface_impl_underlying;
    COMM_LISTEN interface_impl_listen;
    COMM_ACCEPT_CONN interface_impl_accept_conn;
    COMM_ACCEPT_NOTIFY interface_impl_accept_notify;
} COMM_INTERFACE_DESCRIPTION;

MOCKABLE_FUNCTION(, NET_COMM_INSTANCE_HANDLE, net_comm_create, const COMM_INTERFACE_DESCRIPTION*, comm_description, const void*, parameters, const COMM_CALLBACK_INFO*, client_cb);
MOCKABLE_FUNCTION(, void, net_comm_destroy, NET_COMM_INSTANCE_HANDLE, handle);
MOCKABLE_FUNCTION(, int, net_comm_open, NET_COMM_INSTANCE_HANDLE, handle);
MOCKABLE_FUNCTION(, int, net_comm_close, NET_COMM_INSTANCE_HANDLE, handle);
MOCKABLE_FUNCTION(, int, net_comm_send, NET_COMM_INSTANCE_HANDLE, handle, const void*, buffer, size_t, size);
MOCKABLE_FUNCTION(, int, net_comm_send_notify, NET_COMM_INSTANCE_HANDLE, handle, const void*, buffer, uint32_t, size, uint32_t, flags, void*, notify_info);
MOCKABLE_FUNCTION(, int, net_comm_recv, NET_COMM_INSTANCE_HANDLE, handle);
MOCKABLE_FUNCTION(, int, net_comm_recv_notify, NET_COMM_INSTANCE_HANDLE, handle, void*, buffer, uint32_t, size, uint32_t*, flags, void*, notify_info);
MOCKABLE_FUNCTION(, SOCKET_HANDLE, net_comm_underlying_handle, NET_COMM_INSTANCE_HANDLE, handle);
MOCKABLE_FUNCTION(, int, net_comm_listen, NET_COMM_INSTANCE_HANDLE, handle);
MOCKABLE_FUNCTION(, int, net_comm_accept_conn, NET_COMM_INSTANCE_HANDLE, handle);
MOCKABLE_FUNCTION(, NET_COMM_INSTANCE_HANDLE, net_comm_accept_notify, NET_COMM_INSTANCE_HANDLE, handle, const COMM_CALLBACK_INFO*, client_cb);
```

### net_comm_create

```c
NET_COMM_INSTANCE_HANDLE net_comm_create(const COMM_INTERFACE_DESCRIPTION* comm_description, const void* parameters, const COMM_CALLBACK_INFO* client_cb)
```

**SRS_NET_COMM_INTERFACE_001: [** `net_comm_create` shall validate that `comm_desciption` is not NULL. **]**

**SRS_NET_COMM_INTERFACE_002: [** `net_comm_create` shall validate that the following pointers are is not NULL: `interface_impl_create`, `interface_impl_destroy`, `interface_impl_open`, `interface_impl_close`, `interface_impl_send`, `interface_impl_send_notify`,`interface_impl_recv`, `interface_impl_recv_notify`, `interface_impl_underlying` **]**

**SRS_NET_COMM_INTERFACE_001: [** `net_comm_create` shall validate that `client_cb` is not NULL. **]**

**SRS_NET_COMM_INTERFACE_001: [** `net_comm_create` shall allocate a new `NET_COMM_INSTANCE` and on success shall call into the COMM_INTERFACE_DESCRIPTION `interface_impl_create` function. **]**

**SRS_NET_COMM_INTERFACE_001: [** If any error occurs, `net_comm_create` shall fail and return NULL. **]**

### net_comm_destroy

```c
void net_comm_destroy(NET_COMM_INSTANCE_HANDLE handle)
```

**SRS_NET_COMM_INTERFACE_001: [** if handle is NULL, `net_comm_destroy` shall do nothing. **]**

**SRS_NET_COMM_INTERFACE_001: [** `net_comm_destroy` shall call into the COMM_INTERFACE_DESCRIPTION `interface_impl_destroy` function. **]**

**SRS_NET_COMM_INTERFACE_001: [** `net_comm_destroy` shall free all resources associated with `NET_COMM_INSTANCE`. **]**

### net_comm_open

```c
int net_comm_open(NET_COMM_INSTANCE_HANDLE handle)
```

**SRS_NET_COMM_INTERFACE_001: [** if `handle` is NULL, `net_comm_open` shall fail and return a non-zero value. **]**

**SRS_NET_COMM_INTERFACE_001: [** `net_comm_open` shall call into the COMM_INTERFACE_DESCRIPTION `interface_impl_open` function. **]**

### net_comm_close

```c
int net_comm_close(NET_COMM_INSTANCE_HANDLE handle)
```

**SRS_NET_COMM_INTERFACE_001: [** if `handle` is NULL, `net_comm_close` shall fail and return a non-zero value. **]**

**SRS_NET_COMM_INTERFACE_001: [** `net_comm_close` shall call into the COMM_INTERFACE_DESCRIPTION `interface_impl_close` function. **]**

### net_comm_send

```c
int net_comm_send(NET_COMM_INSTANCE_HANDLE handle, const void* buffer, size_t size)
```

**SRS_NET_COMM_INTERFACE_001: [** if `handle` is NULL, `net_comm_send` shall fail and return a non-zero value. **]**

**SRS_NET_COMM_INTERFACE_001: [** `net_comm_send` shall call into the COMM_INTERFACE_DESCRIPTION `interface_impl_send` function. **]**

### net_comm_send_notify

```c
int net_comm_send_notify(NET_COMM_INSTANCE_HANDLE handle, const void* buffer, uint32_t size, uint32_t flags, void* notify_info)
```

**SRS_NET_COMM_INTERFACE_001: [** if `handle` is NULL, `net_comm_send_notify` shall fail and return a non-zero value. **]**

**SRS_NET_COMM_INTERFACE_001: [** `net_comm_send_notify` shall call into the COMM_INTERFACE_DESCRIPTION `interface_impl_send_notify` function. **]**

### net_comm_recv

```c
int net_comm_recv(NET_COMM_INSTANCE_HANDLE handle)
```

**SRS_NET_COMM_INTERFACE_001: [** if `handle` is NULL, `net_comm_recv` shall fail and return a non-zero value. **]**

**SRS_NET_COMM_INTERFACE_001: [** `net_comm_recv` shall call into the COMM_INTERFACE_DESCRIPTION `interface_impl_recv` function. **]**

### net_comm_recv_notify

```c
int net_comm_recv_notify(NET_COMM_INSTANCE_HANDLE handle, void* buffer, uint32_t size, uint32_t* flags, void* notify_info)
```

**SRS_NET_COMM_INTERFACE_001: [** if `handle` is NULL, `net_comm_recv_notify` shall fail and return a non-zero value. **]**

**SRS_NET_COMM_INTERFACE_001: [** `net_comm_recv_notify` shall call into the COMM_INTERFACE_DESCRIPTION `interface_impl_recv_notify` function. **]**

### net_comm_underlying_handle

```c
SOCKET_HANDLE net_comm_underlying_handle(NET_COMM_INSTANCE_HANDLE handle)
```

**SRS_NET_COMM_INTERFACE_001: [** if `handle` is NULL, `net_comm_underlying_handle` shall fail and return a non-zero value. **]**

**SRS_NET_COMM_INTERFACE_001: [** `net_comm_underlying_handle` shall call into the COMM_INTERFACE_DESCRIPTION `interface_impl_underlying` function. **]**

### net_comm_listen

```c
int net_comm_listen(NET_COMM_INSTANCE_HANDLE handle)
```

**SRS_NET_COMM_INTERFACE_001: [** if `handle` is NULL, `net_comm_listen` shall fail and return a non-zero value. **]**

**SRS_NET_COMM_INTERFACE_001: [** if the COMM_INTERFACE_DESCRIPTION `interface_impl_listen` function is NULL, `net_comm_listen` shall fail and return a non-zero value. **]**

**SRS_NET_COMM_INTERFACE_001: [** `net_comm_listen` shall call into the COMM_INTERFACE_DESCRIPTION `interface_impl_listen` function. **]**

### net_comm_accept_conn

```c
int net_comm_accept_conn(NET_COMM_INSTANCE_HANDLE handle)
```

**SRS_NET_COMM_INTERFACE_001: [** if `handle` is NULL, `net_comm_accept_conn` shall fail and return a non-zero value. **]**

**SRS_NET_COMM_INTERFACE_001: [** if the COMM_INTERFACE_DESCRIPTION `interface_impl_accept_conn` function is NULL, `net_comm_accept_conn` shall fail and return a non-zero value. **]**

**SRS_NET_COMM_INTERFACE_001: [** `net_comm_accept_conn` shall assign the comm_description variable and call into the COMM_INTERFACE_DESCRIPTION `interface_impl_accept_conn` function. **]**

### net_comm_accept_notify

```c
NET_COMM_INSTANCE_HANDLE net_comm_accept_notify(NET_COMM_INSTANCE_HANDLE handle, const COMM_CALLBACK_INFO* client_cb)
```

**SRS_NET_COMM_INTERFACE_001: [** if `handle` is NULL, `net_comm_accept_notify` shall fail and return a NULL value. **]**

**SRS_NET_COMM_INTERFACE_001: [** if the COMM_INTERFACE_DESCRIPTION `net_comm_accept_notify` function is NULL, `net_comm_accept_notify` shall fail and return a NULL value. **]**

**SRS_NET_COMM_INTERFACE_001: [** `net_comm_accept_notify` shall allocate a new NET_COMM_INSTANCE_HANDLE and on failure return a NULL value. **]**

**SRS_NET_COMM_INTERFACE_001: [** otherwise, `net_comm_accept_notify` shall assign the comm_description variable and call into the COMM_INTERFACE_DESCRIPTION `interface_impl_accept_notify` function. **]**

**SRS_NET_COMM_INTERFACE_001: [** On COMM_INTERFACE_DESCRIPTION `interface_impl_accept_notify` failure `net_comm_accept_notify` shall free the allocated NET_COMM_INSTANCE_HANDLE. **]**
