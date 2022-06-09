# PAL

## Overview

The Platform Abstraction Layer interfaces presented here are:

- Asynchronous socket
- Execution engine
- InterlockedHL 
- Interlocked
- Lock
- Platform
- Refcount
- Socket handle
- SRW lock
- String utils
- Thread API
- Threadpool
- Timer
- Unique ID
Each of these APIs have:
- an interface header (for example execution_engine.h)
- specific platform implementations (for example execution_engine_win32.c)

Project structure:
- interfaces - project that contains all the interface headers (no specific platform implementations exist in this folder/project)
- win32 - project that contains concrete Windows implementations for the components (execution_engine_win32.c, ...).

For design details of specific platforms see the design document for that platform.

### Execution engine

```c
typedef struct EXECUTION_ENGINE* EXECUTION_ENGINE_HANDLE;

MOCKABLE_FUNCTION(, EXECUTION_ENGINE_HANDLE, execution_engine_create, void*, execution_engine_parameters);
MOCKABLE_FUNCTION(, void, execution_engine_dec_ref, EXECUTION_ENGINE_HANDLE, execution_engine);
MOCKABLE_FUNCTION(, void, execution_engine_inc_ref, EXECUTION_ENGINE_HANDLE, execution_engine);
```

The execution engine component is the context needed for instantiating any of the other 3 components (threadpool, asynchronous socket APIS, asynchronous file APIS). For example on Windows it wraps a PTP_POOL.
Thus in order to instantiate a thread pool component, first an execution engine object needs to be created.

The reason the execution engine exists is in order to be able to have multiple threadpool API/asynchronous API objects created based on the same execution engine (in Windows terms using only one PTP_POOL one can create as many instances of asynchronous socket objects as needed).

When an execution engine is created is receives a `void*` argument which contains platform specific arguments.

For example for Windows it is desired to specify the min and max number of threads for the PTP_POOL being created. Thus for Windows a platform specific argument structure is used:

```c
    typedef struct EXECUTION_ENGINE_PARAMETERS_WIN32_TAG
    {
        uint32_t min_thread_count;
        uint32_t max_thread_count;
    } EXECUTION_ENGINE_PARAMETERS_WIN32;
```

### Threadpool API

The thread pool API object allows scheduling work items in a threadpool.

It provides the following functionality (exact APIs will be defined in module requirements):

- create - create the threadpool object based on an execution engine
- destroy - frees resources associated with the threadpool object
- open_async - opens the threadpool object (work can be scheduled on it after open completes)
- close - closes the theadpool object (all work has either been executed or cancelled after close completes)
- schedule_work - schedules a work item to be executed in the threadpool

### Asynchronous socket API

The asynchronous socket API object allows sending/receiving asynchronously over a socket using platform specific asynchronous APIs.

It provides the following functionality (exact APIs will be defined in module requirements):

- create - create the asynchronous socket object based on an execution engine
- destroy - frees resources associated with the asynchronous socket
- open_async - opens the asynchronous socket object (send/receives can be performed when open is complete)
- close - closes the asynchronous socket
- send_async - sends a number of byte buffers asynchronously
- on_receive - a number of byte buffers have been received asynchronously
- on_error - an error has occured and user needs to handle it

### Asynchronous file API

The asynchronous file API object allows writing/reading asynchronously from a file using platform specific asynchronous APIs.

It provides the following functionality (exact APIs will be defined in module requirements):

- create - create the asynchronous file object based on an execution engine
- destroy - frees resources associated with the asynchronous file object
- open_async - opens the asynchronous file object (reads/writes can be performed when open is complete)
- close - closes the asynchronous file object
- write_async - writes a byte buffer asynchronously as a given position in the file
- read_async - reads a byte buffer asynchronously as a given position in the file
- on_error - an error has occured and user needs to handle it

