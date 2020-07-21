`async_socket` interface requirements
================

## Overview

`async_socket` is an interface for a socket that allows asynchronous sends/receives.

An `async_socket` object receives an execution engine as creation argument in order to be able to schedule all asynchronous work using the execution engine.
An `async_socket` does not own the underlying platform specific socket passed on create. It is the responsibility of the `async_socket` owner to dispose of the platform specific socket.

`async_socket` does not take ownership of the buffers passed to `async_socket_send_async` and `async_socket_receive_async`.

The lifetime of the execution engine should supersede the lifetime of the `threadpool` object.

## Exposed API

```c
typedef struct ASYNC_SOCKET* ASYNC_SOCKET_HANDLE;

#define ASYNC_SOCKET_OPEN_RESULT_VALUES \
    ASYNC_SOCKET_OPEN_OK, \
    ASYNC_SOCKET_OPEN_ERROR

MU_DEFINE_ENUM(ASYNC_SOCKET_OPEN_RESULT, ASYNC_SOCKET_OPEN_RESULT_VALUES)

#define ASYNC_SOCKET_SEND_RESULT_VALUES \
    ASYNC_SOCKET_SEND_OK, \
    ASYNC_SOCKET_SEND_ERROR, \
    ASYNC_SOCKET_SEND_BECAUSE_CLOSE

MU_DEFINE_ENUM(ASYNC_SOCKET_SEND_RESULT, ASYNC_SOCKET_SEND_RESULT_VALUES)

#define ASYNC_SOCKET_RECEIVE_RESULT_VALUES \
    ASYNC_SOCKET_RECEIVE_OK, \
    ASYNC_SOCKET_RECEIVE_ERROR, \
    ASYNC_SOCKET_RECEIVE_BECAUSE_CLOSE

MU_DEFINE_ENUM(ASYNC_SOCKET_RECEIVE_RESULT, ASYNC_SOCKET_RECEIVE_RESULT_VALUES)

typedef void (*ON_ASYNC_SOCKET_OPEN_COMPLETE)(void* context, ASYNC_SOCKET_OPEN_RESULT open_result);
typedef void (*ON_ASYNC_SOCKET_SEND_COMPLETE)(void* context, ASYNC_SOCKET_SEND_RESULT send_result);
typedef void (*ON_ASYNC_SOCKET_RECEIVE_COMPLETE)(void* context, ASYNC_SOCKET_RECEIVE_RESULT receive_result, uint32_t bytes_received);

typedef struct ASYNC_SOCKET_BUFFER_TAG
{
    void* buffer;
    uint32_t length;
} ASYNC_SOCKET_BUFFER;

MOCKABLE_FUNCTION(, ASYNC_SOCKET_HANDLE, async_socket_create, EXECUTION_ENGINE_HANDLE, execution_engine, SOCKET_HANDLE, socket_handle);
MOCKABLE_FUNCTION(, void, async_socket_destroy, ASYNC_SOCKET_HANDLE, async_socket);

MOCKABLE_FUNCTION(, int, async_socket_open_async, ASYNC_SOCKET_HANDLE, async_socket, ON_ASYNC_SOCKET_OPEN_COMPLETE, on_open_complete, void*, on_open_complete_context);
MOCKABLE_FUNCTION(, void, async_socket_close, ASYNC_SOCKET_HANDLE, async_socket);
MOCKABLE_FUNCTION(, int, async_socket_send_async, ASYNC_SOCKET_HANDLE, async_socket, const ASYNC_SOCKET_BUFFER*, payload, uint32_t, buffer_count, ON_ASYNC_SOCKET_SEND_COMPLETE, on_send_complete, void*, on_send_complete_context);
MOCKABLE_FUNCTION(, int, async_socket_receive_async, ASYNC_SOCKET_HANDLE, async_socket, ASYNC_SOCKET_BUFFER*, payload, uint32_t, buffer_count, ON_ASYNC_SOCKET_RECEIVE_COMPLETE, on_receive_complete, void*, on_receive_complete_context);
```

### async_socket_create

```c
MOCKABLE_FUNCTION(, ASYNC_SOCKET_HANDLE, async_socket_create, EXECUTION_ENGINE_HANDLE, execution_engine, SOCKET_HANDLE, socket_handle);
```

`async_socket_create` creates an async socket.

Note: `execution_engine` is to be used by each platform for platform specific asynchronous operations (i.e. on Windows the `PTP_POOL` behind the `execution_engine` shall be used to perform the asynchronous IO).

**SRS_ASYNC_SOCKET_01_001: [** `async_socket_create` shall allocate a new async socket and on success shall return a non-NULL handle. **]**

**SRS_ASYNC_SOCKET_01_002: [** If `execution_engine` is NULL, `async_socket_create` shall fail and return NULL. **]**

**SRS_ASYNC_SOCKET_01_003: [** If any error occurs, `async_socket_create` shall fail and return NULL. **]**

### async_socket_destroy

```c
MOCKABLE_FUNCTION(, void, async_socket_destroy, ASYNC_SOCKET_HANDLE, async_socket);
```

`async_socket_destroy` frees all resources associated with `async_socket`.

**SRS_ASYNC_SOCKET_01_004: [** If `async_socket` is NULL, `async_socket_destroy` shall return. **]**

**SRS_ASYNC_SOCKET_01_005: [** Otherwise, `async_socket_destroy` shall free all resources associated with `async_socket`. **]**

**SRS_ASYNC_SOCKET_01_050: [** While `async_socket` is OPENING, `async_socket_destroy` shall wait for the open to complete either successfully or with error. **]**

**SRS_ASYNC_SOCKET_01_006: [** `async_socket_destroy` shall perform an implicit close if `async_socket` is OPEN. **]**

### async_socket_open_async

```c
MOCKABLE_FUNCTION(, int, async_socket_open_async, ASYNC_SOCKET_HANDLE, async_socket, ON_ASYNC_SOCKET_OPEN_COMPLETE, on_open_complete, void*, on_open_complete_context);
```

`async_socket_open_async` opens the async socket.

**SRS_ASYNC_SOCKET_01_007: [** If `async_socket` is NULL, `async_socket_open_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_01_008: [** If `on_open_complete` is NULL, `async_socket_open_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_01_009: [** `on_open_complete_context` shall be allowed to be NULL. **]**

**SRS_ASYNC_SOCKET_01_023: [** Otherwise, `async_socket_open_async` shall switch the state to OPENING and perform any actions needed to open the `async_socket` object. **]**

**SRS_ASYNC_SOCKET_01_014: [** On success, `async_socket_open_async` shall return 0. **]**

**SRS_ASYNC_SOCKET_01_015: [** If `async_socket` is already OPEN or OPENING, `async_socket_open_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_01_016: [** When opening the async socket completes successfully, `on_open_complete_context` shall be called with `ASYNC_SOCKET_OPEN_OK`. **]**

**SRS_ASYNC_SOCKET_01_017: [** When opening the async socket completes with failure, `on_open_complete_context` shall be called with `ASYNC_SOCKET_OPEN_ERROR`. **]**

### async_socket_close

```c
MOCKABLE_FUNCTION(, void, async_socket_close, ASYNC_SOCKET_HANDLE, async_socket);
```

`async_socket_close` closes an open `async_socket`.

**SRS_ASYNC_SOCKET_01_018: [** If `async_socket` is NULL, `async_socket_close` shall return. **]**

**SRS_ASYNC_SOCKET_01_019: [** Otherwise, `async_socket_close` shall switch the state to CLOSING. **]**

**SRS_ASYNC_SOCKET_01_035: [** Any sends that are not completed shall be indicated as complete with `ASYNC_SOCKET_SEND_BECAUSE_CLOSE`. **]**

**SRS_ASYNC_SOCKET_01_036: [** Any receives that are not completed shall be indicated as complete with `ASYNC_SOCKET_RECEIVE_BECAUSE_CLOSE`. **]**

**SRS_ASYNC_SOCKET_01_021: [** Then `async_socket_close` shall close the async socket, leaving it in a state where an `async_socket_open_async` can be performed. **]**

**SRS_ASYNC_SOCKET_01_022: [** If `async_socket` is not OPEN, `async_socket_close` shall return. **]**

### async_socket_send_async

```c
MOCKABLE_FUNCTION(, int, async_socket_send_async, ASYNC_SOCKET_HANDLE, async_socket, const ASYNC_SOCKET_BUFFER*, payload, uint32_t, buffer_count, ON_ASYNC_SOCKET_SEND_COMPLETE, on_send_complete, void*, on_send_complete_context);
```

`async_socket_send_async` sends a number of buffers asynchronously.

Note: It is the responsibility of the caller to ensure the order of the send calls.

**SRS_ASYNC_SOCKET_01_024: [** If `async_socket` is NULL, `async_socket_send_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_01_025: [** If `payload` is NULL, `async_socket_send_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_01_034: [** If `buffer_count` is 0, `async_socket_send_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_01_046: [** If any of the buffers in `payload` has `buffer` set to NULL, `async_socket_send_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_01_047: [** If any of the buffers in `payload` has `length` set to 0, `async_socket_send_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_01_026: [** If `on_send_complete` is NULL, `async_socket_send_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_01_027: [** `on_send_complete_context` shall be allowed to be NULL. **]**

**SRS_ASYNC_SOCKET_01_028: [** Otherwise `async_socket_send_async` shall send the bytes via the socket passed to `async_socket_create` and on success it shall return 0. **]**

**SRS_ASYNC_SOCKET_01_029: [** If any error occurs, `async_socket_send_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_01_030: [** When sending completes successfully, `on_send_complete` shall be called with `ASYNC_SOCKET_SEND_OK`. **]**

**SRS_ASYNC_SOCKET_01_031: [** When sending completes with error, `on_send_complete` shall be called with `ASYNC_SOCKET_SEND_ERROR`. **]**

```c
MOCKABLE_FUNCTION(, int, async_socket_receive_async, ASYNC_SOCKET_HANDLE, async_socket, ASYNC_SOCKET_BUFFER*, payload, uint32_t, buffer_count, ON_ASYNC_SOCKET_RECEIVE_COMPLETE, on_receive_complete, void*, on_receive_complete_context);
```

`async_socket_receive_async` receives in a number of buffers asynchronously.

Note: It is the responsibility of the caller to ensure the order of the receive calls.

**SRS_ASYNC_SOCKET_01_037: [** If `async_socket` is NULL, `async_socket_receive_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_01_038: [** If `payload` is NULL, `async_socket_receive_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_01_039: [** If `buffer_count` is 0, `async_socket_receive_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_01_048: [** If any of the buffers in `payload` has `buffer` set to NULL, `async_socket_receive_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_01_049: [** If any of the buffers in `payload` has `length` set to 0, `async_socket_receive_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_01_040: [** If `on_receive_complete` is NULL, `async_socket_receive_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_01_041: [** `on_receive_complete_context` shall be allowed to be NULL. **]**

**SRS_ASYNC_SOCKET_01_042: [** Otherwise `async_socket_receive_async` shall receive bytes from the socket passed to `async_socket_create` and on success it shall return 0. **]**

**SRS_ASYNC_SOCKET_01_043: [** If any error occurs, `async_socket_receive_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_01_044: [** When receiving completes successfully, `on_receive_complete` shall be called with `ASYNC_SOCKET_RECEIVE_OK`. **]**

**SRS_ASYNC_SOCKET_01_045: [** When receiving completes with error, `on_receive_complete` shall be called with `ASYNC_SOCKET_RECEIVE_ERROR`. **]**
