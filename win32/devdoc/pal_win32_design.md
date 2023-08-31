# PAL Win32 design

## Overview

This document details how the objects in the PAL are created on Windows.

### Execution engine

An execution object handle on Windows wraps a PTP_POOL.
When the execution object is create the PTP_POOL is created and the min/max number of threads is set on the PTP_POOL object (using `SetThreadpoolThreadMinimum`/`SetThreadpoolThreadMaximum`).

The min/max number of threads are given to create as a pointer to the below structure:

```c
    typedef struct EXECUTION_ENGINE_PARAMETERS_TAG
    {
        uint32_t min_thread_count;
        uint32_t max_thread_count;
    } EXECUTION_ENGINE_PARAMETERS;
```

### Threadpool API

- create - create the threadpool object based on an execution engine, create a threadpool environment, create a cleanup group
- destroy - frees resources (memory, threadpool environment)
- open_async - no special handling for the Windows implementation
- close - closes the theadpool object (close cleanup group)
- schedule_work - schedules a work item to be executed in the threadpool (calls `SubmitThreadpoolWork`)

### Asynchronous socket API

Design details for the Windows implementation will be provided `when time comes`.

### Asynchronous file API

Design details for the Windows implementation will be provided `when time comes`.
