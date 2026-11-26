# completion_port_linux requirements

## Overview

The `completion_port_linux` module handles the threading and control of the epoll socket system.

## Design

epoll keeps exactly one registration (one event mask and one `data` pointer) per file descriptor. A socket can have operations pending in both directions at the same time (for example `async_socket_linux` has a receive waiting for `EPOLLIN` while a send that hit `EAGAIN` waits for `EPOLLOUT`). Registering every operation separately would make the last `epoll_ctl` call replace the registration of the other operations, which would then never complete.

Therefore `completion_port_linux` keeps one socket entry per socket:

- The socket entry is created by the first `completion_port_add` for the socket and is kept in a socket entry lookup (a fixed size hash of buckets keyed by the socket) until `completion_port_remove` is called for the socket.
- An operation is *inbound* if its `epoll_op` does not contain `EPOLLOUT` and *outbound* otherwise. The socket entry has a FIFO queue of pending inbound operations and a FIFO queue of pending outbound operations.
- The *epoll interest* of a socket entry is `EPOLLIN | EPOLLRDHUP` if inbound operations are pending, plus `EPOLLOUT` if outbound operations are pending, plus `EPOLLONESHOT`.
- The socket is registered once in epoll, with the epoll interest of the socket entry and with `data.ptr` pointing to the socket entry.
- For each event, `epoll_worker_func` completes at most one inbound and at most one outbound operation and re-arms the socket with the epoll interest of the operations that are still pending.
- Every operation accepted by `completion_port_add` has its `event_callback` called exactly once: either by `epoll_worker_func` or with `COMPLETION_PORT_EPOLL_ABANDONED` by `completion_port_remove`. Both take an operation out of its queue under the completion port lock before calling its `event_callback`, and they never call an `event_callback` while holding the lock.
- `completion_port_remove` does not free the socket entry, because `epoll_worker_func` may still hold a pointer to it from an `epoll_wait` batch that returned before the socket was removed from epoll. The socket entry is put on a retired list instead and is freed by `epoll_worker_func` before its next `epoll_wait` call (when all such batches have been processed) or by `completion_port_dec_ref`.

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

typedef void (*ON_COMPLETION_PORT_EVENT_COMPLETE)(void* context, COMPLETION_PORT_EPOLL_ACTION action);

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

- **SRS_COMPLETION_PORT_LINUX_01_001: [** free the pending operations of every socket entry without calling their `event_callback`. **]**

- **SRS_COMPLETION_PORT_LINUX_01_002: [** free every socket entry. **]**

- **SRS_COMPLETION_PORT_LINUX_01_003: [** free every retired socket entry. **]**

- **SRS_COMPLETION_PORT_LINUX_11_015: [** then the memory associated with completion_port shall be freed. **]**

### completion_port_add

```C
MOCKABLE_FUNCTION(, int, completion_port_add, COMPLETION_PORT_HANDLE, completion_port, int, epoll_op, SOCKET_HANDLE, socket,
    ON_COMPLETION_PORT_EVENT_COMPLETE, event_callback, void*, event_callback_ctx);
```

`completion_port_add` queues an operation that waits for `socket` to become ready in the direction given by `epoll_op`. On success `event_callback` is called exactly once, either by `epoll_worker_func` or by `completion_port_remove`.

**SRS_COMPLETION_PORT_LINUX_11_016: [** If `completion_port` is `NULL`, `completion_port_add` shall return a non-zero value. **]**

**SRS_COMPLETION_PORT_LINUX_11_017: [** If `socket` is `INVALID_SOCKET`, `completion_port_add` shall return a non-zero value. **]**

**SRS_COMPLETION_PORT_LINUX_11_018: [** If `event_callback` is `NULL`, `completion_port_add` shall return a non-zero value. **]**

**SRS_COMPLETION_PORT_LINUX_11_019: [** `completion_port_add` shall ensure the thread completion flag is not set. **]**

**SRS_COMPLETION_PORT_LINUX_11_020: [** `completion_port_add` shall increment the ongoing call count value to prevent close. **]**

**SRS_COMPLETION_PORT_LINUX_01_004: [** `completion_port_add` shall allocate a pending operation that stores `epoll_op`, `event_callback` and `event_callback_ctx`. **]**

**SRS_COMPLETION_PORT_LINUX_01_005: [** `completion_port_add` shall acquire the completion port lock. **]**

**SRS_COMPLETION_PORT_LINUX_01_006: [** If no socket entry exists for `socket`, `completion_port_add` shall allocate a new socket entry and add it to the socket entry lookup. **]**

**SRS_COMPLETION_PORT_LINUX_01_007: [** If the socket entry is not registered in epoll, `completion_port_add` shall call `epoll_ctl` with `EPOLL_CTL_ADD`, the epoll interest of the socket entry including the new operation and the socket entry as `data.ptr`. **]**

**SRS_COMPLETION_PORT_LINUX_01_008: [** If the socket entry is registered in epoll, `completion_port_add` shall call `epoll_ctl` with `EPOLL_CTL_MOD`, the epoll interest of the socket entry including the new operation and the socket entry as `data.ptr`. **]**

**SRS_COMPLETION_PORT_LINUX_01_009: [** If `epoll_ctl` succeeds and `epoll_op` contains `EPOLLOUT`, `completion_port_add` shall append the operation to the outbound operation queue of the socket entry. **]**

**SRS_COMPLETION_PORT_LINUX_01_010: [** If `epoll_ctl` succeeds and `epoll_op` does not contain `EPOLLOUT`, `completion_port_add` shall append the operation to the inbound operation queue of the socket entry. **]**

**SRS_COMPLETION_PORT_LINUX_01_011: [** If `epoll_ctl` with `EPOLL_CTL_ADD` fails, `completion_port_add` shall remove the new socket entry from the socket entry lookup and free it. **]**

**SRS_COMPLETION_PORT_LINUX_01_012: [** `completion_port_add` shall release the completion port lock. **]**

**SRS_COMPLETION_PORT_LINUX_01_013: [** If queueing the operation fails, `completion_port_add` shall free the pending operation. **]**

**SRS_COMPLETION_PORT_LINUX_11_025: [** `completion_port_add` shall decrement the ongoing call count value to unblock close. **]**

**SRS_COMPLETION_PORT_LINUX_11_026: [** On success, `completion_port_add` shall return 0. **]**

**SRS_COMPLETION_PORT_LINUX_11_027: [** If any error occurs, `completion_port_add` shall fail and return a non-zero value. **]**

### completion_port_remove

```C
MOCKABLE_FUNCTION(, void, completion_port_remove, COMPLETION_PORT_HANDLE, completion_port, SOCKET_HANDLE, socket);
```

`completion_port_remove` removes `socket` from epoll and abandons all the operations that are pending for it.

**SRS_COMPLETION_PORT_LINUX_11_028: [** If `completion_port` is `NULL`, `completion_port_remove` shall return. **]**

**SRS_COMPLETION_PORT_LINUX_11_029: [** If `socket` is `INVALID_SOCKET`, `completion_port_remove` shall return. **]**

**SRS_COMPLETION_PORT_LINUX_01_014: [** `completion_port_remove` shall acquire the completion port lock. **]**

**SRS_COMPLETION_PORT_LINUX_11_030: [** `completion_port_remove` shall remove the underlying socket from the epoll by calling `epoll_ctl` with `EPOLL_CTL_DEL`. **]**

**SRS_COMPLETION_PORT_LINUX_01_015: [** If no socket entry exists for `socket`, `completion_port_remove` shall not call any `event_callback`. **]**

**SRS_COMPLETION_PORT_LINUX_01_016: [** `completion_port_remove` shall remove the socket entry from the socket entry lookup. **]**

**SRS_COMPLETION_PORT_LINUX_01_017: [** `completion_port_remove` shall mark the socket entry as removed. **]**

**SRS_COMPLETION_PORT_LINUX_01_018: [** `completion_port_remove` shall release the completion port lock before calling any `event_callback`. **]**

**SRS_COMPLETION_PORT_LINUX_01_019: [** `completion_port_remove` shall call the `event_callback` of every pending operation of the socket entry with `COMPLETION_PORT_EPOLL_ABANDONED`. **]**

**SRS_COMPLETION_PORT_LINUX_01_020: [** `completion_port_remove` shall free every abandoned operation. **]**

**SRS_COMPLETION_PORT_LINUX_11_039: [** If the event_callback is currently executing, `completion_port_remove` shall wait for the event_callback function to finish before completing. **]**

**SRS_COMPLETION_PORT_LINUX_01_021: [** `completion_port_remove` shall add the socket entry to the retired list of the completion port. **]**

### epoll_worker_func

```c
static int epoll_worker_func(void* parameter)
```

`epoll_worker_func` is the thread function that handles the dispatching of epoll work item that are signaled

**SRS_COMPLETION_PORT_LINUX_11_031: [** If `parameter` is `NULL`, `epoll_worker_func` shall do nothing. **]**

**SRS_COMPLETION_PORT_LINUX_01_022: [** Before each call to `epoll_wait`, `epoll_worker_func` shall free the socket entries in the retired list. **]**

**SRS_COMPLETION_PORT_LINUX_11_032: [** `epoll_worker_func` shall call `epoll_wait` to wait for an epoll event to become signaled with a timeout of 2 Seconds. **]**

**SRS_COMPLETION_PORT_LINUX_11_033: [** On a `epoll_wait` timeout `epoll_worker_func` shall ensure it should not exit and issue another `epoll_wait`. **]**

**SRS_COMPLETION_PORT_LINUX_11_034: [** `epoll_worker_func` shall loop through the num of descriptors that was returned. **]**

**SRS_COMPLETION_PORT_LINUX_01_023: [** For each event, `epoll_worker_func` shall acquire the completion port lock. **]**

**SRS_COMPLETION_PORT_LINUX_01_024: [** If the socket entry of the event is marked as removed, `epoll_worker_func` shall skip the event. **]**

**SRS_COMPLETION_PORT_LINUX_01_025: [** If the event contains `EPOLLIN`, `EPOLLRDHUP`, `EPOLLERR` or `EPOLLHUP`, `epoll_worker_func` shall remove the first operation, if any, from the inbound operation queue of the socket entry. **]**

**SRS_COMPLETION_PORT_LINUX_01_026: [** If the event contains `EPOLLOUT`, `EPOLLERR` or `EPOLLHUP`, `epoll_worker_func` shall remove the first operation, if any, from the outbound operation queue of the socket entry. **]**

**SRS_COMPLETION_PORT_LINUX_01_027: [** If an operation was removed from a queue, `epoll_worker_func` shall increment the executing callback count of the socket entry. **]**

**SRS_COMPLETION_PORT_LINUX_01_028: [** If any operation is still pending for the socket entry, `epoll_worker_func` shall re-arm the socket by calling `epoll_ctl` with `EPOLL_CTL_MOD`, the epoll interest of the socket entry and the socket entry as `data.ptr`. **]**

**SRS_COMPLETION_PORT_LINUX_01_029: [** If no operation is pending for the socket entry, `epoll_worker_func` shall not re-arm the socket. **]**

**SRS_COMPLETION_PORT_LINUX_01_030: [** `epoll_worker_func` shall release the completion port lock before calling any `event_callback`. **]**

**SRS_COMPLETION_PORT_LINUX_01_031: [** If the removed inbound operation was added with `EPOLLRDHUP` in `epoll_op` and the event contains `EPOLLRDHUP`, `EPOLLERR` or `EPOLLHUP`, `epoll_worker_func` shall call its `event_callback` with `COMPLETION_PORT_EPOLL_EPOLLRDHUP`. **]**

**SRS_COMPLETION_PORT_LINUX_01_032: [** Otherwise, `epoll_worker_func` shall call the `event_callback` of the removed inbound operation with `COMPLETION_PORT_EPOLL_EPOLLIN`. **]**

**SRS_COMPLETION_PORT_LINUX_01_033: [** `epoll_worker_func` shall call the `event_callback` of the removed outbound operation with `COMPLETION_PORT_EPOLL_EPOLLOUT`. **]**

**SRS_COMPLETION_PORT_LINUX_01_034: [** `epoll_worker_func` shall free the removed operations after calling their `event_callback`. **]**

**SRS_COMPLETION_PORT_LINUX_01_035: [** After calling the event callbacks, `epoll_worker_func` shall decrement the executing callback count of the socket entry by calling `InterlockedHL_DecrementAndWake`. **]**
