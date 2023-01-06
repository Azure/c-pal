# completion_port_linux requirements

## Overview

The `completion_port_linux` module handles the threading and control of the epoll socket system.

## Exposed API

```C
typedef struct COMPLETION_PORT_TAG* COMPLETION_PORT_HANDLE;

#define COMPLETION_PORT_EPOLL_ACTION_VALUES \
    COMPLETION_PORT_EPOLL_EPOLLRDHUP, \
    COMPLETION_PORT_EPOLL_EPOLLIN, \
    COMPLETION_PORT_EPOLL_EPOLLOUT, \
    COMPLETION_PORT_EPOLL_ABANDONED, \
    COMPLETION_PORT_EPOLL_ERROR

MU_DEFINE_ENUM(COMPLETION_PORT_EPOLL_ACTION, COMPLETION_PORT_EPOLL_ACTION_VALUES)

typedef void (*ON_COMPLETION_PORT_EVENT_COMPLETE)(void* context, COMPLETION_PORT_EPOLL_ACTION epoll_action);

MOCKABLE_FUNCTION(, COMPLETION_PORT_HANDLE, completion_port_create);
MOCKABLE_FUNCTION(, void, completion_port_inc_ref, COMPLETION_PORT_HANDLE, completion_port);
MOCKABLE_FUNCTION(, void, completion_port_dec_ref, COMPLETION_PORT_HANDLE, completion_port);
MOCKABLE_FUNCTION(, int, completion_port_add, COMPLETION_PORT_HANDLE, completion_port, int, epoll_op, SOCKET_HANDLE, socket,
    ON_COMPLETION_PORT_EVENT_COMPLETE, event_callback, void*, event_callback_ctx);
MOCKABLE_FUNCTION(, void, completion_port_remove, COMPLETION_PORT_HANDLE, completion_port, SOCKET_HANDLE, socket);
```

### completion_port_create

```C
MOCKABLE_FUNCTION(, COMPLETION_PORT_HANDLE, completion_port_create);
```

`completion_port_create` creates and initializes the completion port module

`completion_port_create` shall allocate memory for a completion port object.

`completion_port_create` shall create the epoll instance by calling `epoll_create`.

`completion_port_create` shall create a thread that runs `epoll_worker_func` to handle the epoll events.

On success `completion_port_create` shall return the allocated `COMPLETION_PORT_HANDLE`.

If there are any errors then `completion_port_create` shall fail and return `NULL`.

### completion_port_inc_ref

```C
MOCKABLE_FUNCTION(, void, completion_port_inc_ref, COMPLETION_PORT_HANDLE, completion_port);
```

`completion_port_inc_ref` handles the incrementing of the reference count of the module

If `completion_port` is `NULL`, `completion_port_inc_ref` shall return.

Otherwise `completion_port_inc_ref` shall increment the internally maintained reference count.

### completion_port_dec_ref

```C
MOCKABLE_FUNCTION(, void, completion_port_dec_ref, COMPLETION_PORT_HANDLE, completion_port);
```

`completion_port_dec_ref` handles the decrement of the reference count and freeing the memory if necessary

`completion_port_dec_ref` shall decrement the reference count for `completion_port`.

If the reference count reaches 0, `completion_port_dec_ref` shall do the following:

- increment the flag signaling that the threads can complete.

- close the epoll object.

- close the thread by calling `ThreadAPI_Join`

- then the memory associated with `completion_port` shall be freed.

### completion_port_add

```C
MOCKABLE_FUNCTION(, int, completion_port_add, COMPLETION_PORT_HANDLE, completion_port, int, epoll_op, SOCKET_HANDLE, socket,
    ON_COMPLETION_PORT_EVENT_COMPLETE, event_callback, void*, event_callback_ctx);
```

`completion_port_add` adds an ingress item to the post queue for epoll signaling

If `completion_port` is `NULL`, `completion_port_add` shall return a non-NULL value.

If `socket` is `INVALID_SOCKET`, `completion_port_add` shall return a non-NULL value.

If `event_callback` is `NULL`, `completion_port_add` shall return a non-NULL value.

`completion_port_add` shall allocate a `EPOLL_THREAD_DATA` object to store thread data.

`completion_port_add` shall add the `EPOLL_THREAD_DATA` object to a list for later removal

`completion_port_add` shall add the socket in the epoll system by calling `epoll_ctl` with `EPOLL_CTL_MOD` along with the `epoll_op` variable.

If the `epoll_ctl` call fails with `ENOENT`, `completion_port_add` shall call `epoll_ctl` again with `EPOLL_CTL_ADD`.

On success, `completion_port_add` shall return 0.

If any error occurs, `completion_port_add` shall fail and return a non-zero value.

### completion_port_remove

```C
MOCKABLE_FUNCTION(, void, completion_port_remove, COMPLETION_PORT_HANDLE, completion_port, SOCKET_HANDLE, socket);
```

If `completion_port` is `NULL`, `completion_port_remove` shall return.

If `socket` is `INVALID_SOCKET`, `completion_port_remove` shall return.

`completion_port_remove` shall remove the underlying socket from the epoll by calling `epoll_ctl` with `EPOLL_CTL_DEL`.

### epoll_worker_func

```c
static int epoll_worker_func(void* parameter)
```

If `parameter` is `NULL`, `epoll_worker_func` shall do nothing.

`epoll_worker_func` shall call `epoll_wait` to wait for an epoll event to become signaled with a timeout of 2 Seconds.

`epoll_worker_func` shall loop through the num of descriptors that was returned.

`epoll_worker_func` shall call the `event_callback` with the specified `COMPLETION_PORT_EPOLL_ACTION` that was returned

Then `epoll_worker_func` shall remove the `EPOLL_THREAD_DATA` from the list and free the object
