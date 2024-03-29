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

`completion_port_create` creates and initializes the completion port module.

**SRS_COMPLETION_PORT_LINUX_11_001: [** `completion_port_create` shall allocate memory for a completion port object. **]**

**SRS_COMPLETION_PORT_LINUX_11_002: [** `completion_port_create` shall create the epoll instance by calling `epoll_create`. **]**

**SRS_COMPLETION_PORT_LINUX_11_003: [** `completion_port_create` shall create a thread that runs `epoll_worker_func` to handle the epoll events. **]**

**SRS_COMPLETION_PORT_LINUX_11_004: [** On success `completion_port_create` shall return the allocated `COMPLETION_PORT_HANDLE`. **]**

**SRS_COMPLETION_PORT_LINUX_11_005: [** If there are any errors then `completion_port_create` shall fail and return `NULL`. **]**

### completion_port_inc_ref

```C
MOCKABLE_FUNCTION(, void, completion_port_inc_ref, COMPLETION_PORT_HANDLE, completion_port);
```

`completion_port_inc_ref` handles the incrementing of the reference count of the module.

**SRS_COMPLETION_PORT_LINUX_11_006: [** If `completion_port` is `NULL`, `completion_port_inc_ref` shall return. **]**

**SRS_COMPLETION_PORT_LINUX_11_007: [** Otherwise `completion_port_inc_ref` shall increment the internally maintained reference count. **]**

### completion_port_dec_ref

```C
MOCKABLE_FUNCTION(, void, completion_port_dec_ref, COMPLETION_PORT_HANDLE, completion_port);
```

`completion_port_dec_ref` handles the decrement of the reference count and freeing the memory if necessary.

**SRS_COMPLETION_PORT_LINUX_11_008: [** If `completion_port` is `NULL`, `completion_port_dec_ref` shall return. **]**

**SRS_COMPLETION_PORT_LINUX_11_009: [** `completion_port_dec_ref` shall decrement the reference count for `completion_port`. **]**

**SRS_COMPLETION_PORT_LINUX_11_010: [** If the reference count reaches 0, `completion_port_dec_ref` shall do the following: **]**

- **SRS_COMPLETION_PORT_LINUX_11_011: [** wait for the ongoing call count to reach zero. **]**

- **SRS_COMPLETION_PORT_LINUX_11_012: [** increment the flag signaling that the threads can complete. **]**

- **SRS_COMPLETION_PORT_LINUX_11_013: [** close the epoll object. **]**

- **SRS_COMPLETION_PORT_LINUX_11_014: [** close the thread by calling `ThreadAPI_Join`. **]**

- **SRS_COMPLETION_PORT_LINUX_11_015: [** then the memory associated with `completion_port` shall be freed. **]**

### completion_port_add

```C
MOCKABLE_FUNCTION(, int, completion_port_add, COMPLETION_PORT_HANDLE, completion_port, int, epoll_op, SOCKET_HANDLE, socket,
    ON_COMPLETION_PORT_EVENT_COMPLETE, event_callback, void*, event_callback_ctx);
```

`completion_port_add` adds an ingress item to the post queue for epoll signaling.

**SRS_COMPLETION_PORT_LINUX_11_016: [** If `completion_port` is `NULL`, `completion_port_add` shall return a non-zero value. **]**

**SRS_COMPLETION_PORT_LINUX_11_017: [** If `socket` is `INVALID_SOCKET`, `completion_port_add` shall return a non-zero value. **]**

**SRS_COMPLETION_PORT_LINUX_11_018: [** If `event_callback` is `NULL`, `completion_port_add` shall return a non-zero value. **]**

**SRS_COMPLETION_PORT_LINUX_11_019: [** `completion_port_add` shall ensure the thread completion flag is not set. **]**

**SRS_COMPLETION_PORT_LINUX_11_020: [** `completion_port_add` shall increment the ongoing call count value to prevent close. **]**

**SRS_COMPLETION_PORT_LINUX_11_021: [** `completion_port_add` shall allocate a `EPOLL_THREAD_DATA` object to store thread data. **]**

**SRS_COMPLETION_PORT_LINUX_11_022: [** `completion_port_add` shall add the `EPOLL_THREAD_DATA` object to a list for later removal. **]**

**SRS_COMPLETION_PORT_LINUX_11_023: [** `completion_port_add` shall add the socket in the epoll system by calling `epoll_ctl` with `EPOLL_CTL_MOD` along with the `epoll_op` variable. **]**

**SRS_COMPLETION_PORT_LINUX_11_024: [** If the `epoll_ctl` call fails with `ENOENT`, `completion_port_add` shall call `epoll_ctl` again with `EPOLL_CTL_ADD`. **]**

**SRS_COMPLETION_PORT_LINUX_11_025: [** `completion_port_add` shall decrement the ongoing call count value to unblock close. **]**

**SRS_COMPLETION_PORT_LINUX_11_026: [** On success, `completion_port_add` shall return 0. **]**

**SRS_COMPLETION_PORT_LINUX_11_027: [** If any error occurs, `completion_port_add` shall fail and return a non-zero value. **]**

### completion_port_remove

```C
MOCKABLE_FUNCTION(, void, completion_port_remove, COMPLETION_PORT_HANDLE, completion_port, SOCKET_HANDLE, socket);
```

`completion_port_remove` removes the ingress item to the post queue for epoll signaling.

**SRS_COMPLETION_PORT_LINUX_11_028: [** If `completion_port` is `NULL`, `completion_port_remove` shall return. **]**

**SRS_COMPLETION_PORT_LINUX_11_029: [** If `socket` is `INVALID_SOCKET`, `completion_port_remove` shall return. **]**

**SRS_COMPLETION_PORT_LINUX_11_030: [** `completion_port_remove` shall remove the underlying socket from the epoll by calling `epoll_ctl` with `EPOLL_CTL_DEL`. **]**

**SRS_COMPLETION_PORT_LINUX_11_037: [** `completion_port_remove` shall loop through the list of `EPOLL_THREAD_DATA` object and call the event_callback with `COMPLETION_PORT_EPOLL_ABANDONED` **]**

**SRS_COMPLETION_PORT_LINUX_11_038: [** If the event_callback has not been called, `completion_port_remove` shall call the event_callback. **]**

**SRS_COMPLETION_PORT_LINUX_11_039: [** If the event_callback is currently executing, `completion_port_remove` shall wait for the event_callback function to finish before completing. **]**

### epoll_worker_func

```c
static int epoll_worker_func(void* parameter)
```

`epoll_worker_func` is the thread function that handles the dispatching of epoll work item that are signaled

**SRS_COMPLETION_PORT_LINUX_11_031: [** If `parameter` is `NULL`, `epoll_worker_func` shall do nothing. **]**

**SRS_COMPLETION_PORT_LINUX_11_032: [** `epoll_worker_func` shall call `epoll_wait` to wait for an epoll event to become signaled with a timeout of 2 Seconds. **]**

**SRS_COMPLETION_PORT_LINUX_11_033: [** On a `epoll_wait` timeout `epoll_worker_func` shall ensure it should not exit and issue another `epoll_wait`. **]**

**SRS_COMPLETION_PORT_LINUX_11_034: [** `epoll_worker_func` shall loop through the num of descriptors that was returned. **]**

**SRS_COMPLETION_PORT_LINUX_11_035: [** `epoll_worker_func` shall call the `event_callback` with the specified `COMPLETION_PORT_EPOLL_ACTION` that was returned. **]**

**SRS_COMPLETION_PORT_LINUX_11_036: [** Then `epoll_worker_func` shall remove the `EPOLL_THREAD_DATA` from the list and free the object. **]**
