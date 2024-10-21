# job_object_helper requirements

`job_object_helper` is a module that wraps the Windows JobObject API, primarily for the purpose of limiting the amount of memory and CPU the current process can consume.

## Exposed API
```c
MOCKABLE_FUNCTION(, THANDLE(JOB_OBJECT_HELPER), job_object_helper_create);
MOCKABLE_FUNCTION(, int, job_object_helper_limit_memory, THANDLE(JOB_OBJECT_HELPER), job_object_helper, uint32_t, percent_physical_memory);
MOCKABLE_FUNCTION(, int, job_object_helper_limit_cpu, THANDLE(JOB_OBJECT_HELPER), job_object_helper, uint32_t, percent_cpu);
```

## job_object_helper_create

```c
MOCKABLE_FUNCTION(, THANDLE(JOB_OBJECT_HELPER), job_object_helper_create);
```

``job_object_helper_create`` is used to create a `JOB_OBJECT_HELPER` object.

`job_object_helper_create` shall allocate a `JOB_OBJECT_HELPER` object.

`job_object_helper_create` shall call `GlobalMemoryStatusEx` to get the total amount of physical memory in kb.

`job_object_helper_create` shall call `CreateJobObject` to create a new job object passing `NULL` for both `lpJobAttributes` and `lpName`.

`job_object_helper_create` shall call `GetCurrentProcess` to get the current process handle.

`job_object_helper_create` shall call `AssignProcessToJobObject` to assign the current process to the new job object.

`job_object_helper_create` shall call `CloseHandle` to close the handle of the current process.

If there are any failures, `job_object_helper_create` shall fail and return `NULL`.

`job_object_helper_create` shall succeed and return the `JOB_OBJECT_HELPER` object.

## job_object_helper_dispose

```c
static void job_object_helper_dispose(JOB_OBJECT_HELPER* job_object_helper);
```

`job_object_helper_dispose` frees all of the resources used by the `JOB_OBJECT_HELPER` object.

`job_object_helper_dispose` shall call `CloseHandle` to close the handle to the job object.

## job_object_helper_limit_memory

```c
MOCKABLE_FUNCTION(, int, job_object_helper_limit_memory, THANDLE(JOB_OBJECT_HELPER), job_object_helper, uint32_t, percent_physical_memory);
```

`job_object_helper_limit_memory` limits the amount of physical memory available to the current progess.

If `job_object_helper` is `NULL`, `job_object_helper_limit_memory` shall fail and return a non-zero value.

If `percent_physical_memory` is `0`, `job_object_helper_limit_memory` shall fail and return a non-zero value.

If `percent_physical_memory` is greater than `100`, `job_object_helper_limit_memory` shall fail and return a non-zero value.

`job_object_helper_limit_memory` shall call `SetInformationJobObject`, passing `JobObjectExtendedLimitInformation` and a `JOBOBJECT_EXTENDED_LIMIT_INFORMATION` object with `JOB_OBJECT_LIMIT_JOB_MEMORY` set and `JobMemoryLimit` set to the `percent_physical_memory` percent of the physical memory in bytes.

If there are any failures, `job_object_helper_limit_memory` shall fail and return a non-zero value.

`job_object_helper_limit_memory` shall succeed and return `0`.

## job_object_helper_limit_cpu

```c
MOCKABLE_FUNCTION(, int, job_object_helper_limit_cpu, THANDLE(JOB_OBJECT_HELPER), job_object_helper, uint32_t, percent_cpu);
```

`job_object_helper_limit_cpu` limits the amount of CPU available to the current process.

If `job_object_helper` is `NULL`, `job_object_helper_limit_cpu` shall fail and return a non-zero value.

If `percent_cpu` is  `0`, `job_object_helper_limit_cpu` shall fail and return a non-zero value.

If `percent_cpu` is greater than `100`, `job_object_helper_limit_cpu` shall fail and return a non-zero value.

`job_object_helper_limit_cpu` shall call `SetInformationJobObject` passing `JobObjectCpuRateControlInformation` and a `JOBOBJECT_CPU_RATE_CONTROL_INFORMATION` object with `JOB_OBJECT_CPU_RATE_CONTROL_ENABLE` and `JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP` set, and `CpuRate` set to `percent_cpu` times `100`.

If there are any failures, `job_object_helper_limit_cpu` shall fail and return a non-zero value.

`job_object_helper_limit_cpu` shall succeed and return `0`.
