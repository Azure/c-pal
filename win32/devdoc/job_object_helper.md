# job_object_helper requirements

`job_object_helper` is a module that wraps the Windows JobObject API, primarily for the purpose of limiting the amount of memory and CPU the current process can consume.

## Exposed API
```c
MOCKABLE_FUNCTION(, THANDLE(JOB_OBJECT_HELPER), job_object_helper_create);
MOCKABLE_FUNCTION(, int, job_object_helper_open, THANDLE(JOB_OBJECT_HELPER), job_object_helper);
MOCKABLE_FUNCTION(, void, job_object_helper_close, THANDLE(JOB_OBJECT_HELPER), job_object_helper);
MOCKABLE_FUNCTION(, int, job_object_helper_limit_memory, THANDLE(JOB_OBJECT_HELPER), job_object_helper, uint32_t, percent_physical_memory);
MOCKABLE_FUNCTION(, int, job_object_helper_limit_cpu, THANDLE(JOB_OBJECT_HELPER), job_object_helper, uint32_t, percent_cpu);
```

## job_object_helper_create
```c
MOCKABLE_FUNCTION(, THANDLE(JOB_OBJECT_HELPER), job_object_helper_create);
```
``job_object_helper_create`` is used to create a `JOB_OBJECT_HELPER` object.

**SRS_JOB_OBJECT_HELPER_18_016: [** `job_object_helper_create` shall allocate a `JOB_OBJECT_HELPER` object. **]**

**SRS_JOB_OBJECT_HELPER_18_017: [** `job_object_helper_create` shall call `sm_create`. **]**

**SRS_JOB_OBJECT_HELPER_18_018: [** If there are any failures, `job_object_helper_create` shall fail and return `NULL`. **]**

**SRS_JOB_OBJECT_HELPER_18_019: [** `job_object_helper_create` shall succeed and return the `JOB_OBJECT_HELPER` object.  **]**


## job_object_helper_dispose
```c
static void job_object_helper_dispose(JOB_OBJECT_HELPER* dispose);
```
`job_object_helper_dispose` frees all of the resources used by the `JOB_OBJECT_HELPER` object.

**SRS_JOB_OBJECT_HELPER_18_020: [** `job_object_helper_dispose` shall call `sm_destroy`. **]**


## job_object_helper_open
```c
MOCKABLE_FUNCTION(, int, job_object_helper_open, THANDLE(JOB_OBJECT_HELPER), job_object_helper);
```
`job_object_helper_open` prepares the `JOB_OBJECT_HELPER` object for use by initializing the resources that it needs. 

**SRS_JOB_OBJECT_HELPER_18_021: [** If `job_object_helper` is `NULL`, `job_object_helper_open` shall fail and return a non-zero value. **]**

**SRS_JOB_OBJECT_HELPER_18_022: [** `job_object_helper_open` shall call `sm_open_begin`. **]**

**SRS_JOB_OBJECT_HELPER_18_023: [** `job_object_helper_open` shall call `GlobalMemoryStatusEx` to get the total amount of physical memory in kb. **]**

**SRS_JOB_OBJECT_HELPER_18_024: [** `job_object_helper_open` shall call `CreateJobObject` to create a new job object passing `NULL` for both `lpJobAttributes` and `lpName`. **]**

**SRS_JOB_OBJECT_HELPER_18_025: [** `job_object_helper_open` shall call `GetCurrentProcess` to get the current process handle. **]**

**SRS_JOB_OBJECT_HELPER_18_026: [** `job_object_helper_open` shall call `AssignProcessToJobObject` to assign the current process to the new job object. **]**

**SRS_JOB_OBJECT_HELPER_18_027: [** `job_object_helper_open` shall call `CloseHandle` to close the handle of the current process. **]**

**SRS_JOB_OBJECT_HELPER_18_028: [** `job_object_helper_open` shall call `sm_open_end`. **]**

**SRS_JOB_OBJECT_HELPER_18_029: [** If there are any failures, `job_object_helper_open` shall fail and return a non-zero value. **]**

**SRS_JOB_OBJECT_HELPER_18_030: [** `job_object_helper_open` shall succeed and return `0`. **]**


## job_object_helper_close
```c
MOCKABLE_FUNCTION(, void, job_object_helper_close, THANDLE(JOB_OBJECT_HELPER), job_object_helper);
```
`job_object_helper_close` de-initializes all of the resources used by the `JOB_OBJECT_HELPER` object. 

**SRS_JOB_OBJECT_HELPER_18_031: [** If `job_object_helper` is `NULL`, `job_object_helper_close` shall return. **]**

**SRS_JOB_OBJECT_HELPER_18_032: [** `job_object_helper_close` shall call `sm_close_begin`. **]**

**SRS_JOB_OBJECT_HELPER_18_033: [** `job_object_helper_close` shall call `CloseHandle` to close the handle to the job object. **]**

**SRS_JOB_OBJECT_HELPER_18_034: [** `job_object_helper_close` shall call `sm_close_end`. **]**


## job_object_helper_limit_memory
```c
MOCKABLE_FUNCTION(, int, job_object_helper_limit_memory, THANDLE(JOB_OBJECT_HELPER), job_object_helper, uint32_t, percent_physical_memory);
```
`job_object_helper_limit_memory` limits the amount of physical memory available to the current progess.

**SRS_JOB_OBJECT_HELPER_18_035: [** If `job_object_helper` is `NULL`, `job_object_helper_limit_memory` shall fail and return a non-zero value. **]**

**SRS_JOB_OBJECT_HELPER_18_036: [** If `percent_physical_memory` is `0`, `job_object_helper_limit_memory` shall fail and return a non-zero value. **]**

**SRS_JOB_OBJECT_HELPER_18_037: [** If `percent_physical_memory` is greater than `100`, `job_object_helper_limit_memory` shall fail and return a non-zero value. **]**

**SRS_JOB_OBJECT_HELPER_18_038: [** `job_object_helper_limit_memory` shall call `sm_exec_begin`. **]**

**SRS_JOB_OBJECT_HELPER_18_039: [** `job_object_helper_limit_memory` shall call `SetInformationJobObject`, passing `JobObjectExtendedLimitInformation` and a `JOBOBJECT_EXTENDED_LIMIT_INFORMATION` object with `JOB_OBJECT_LIMIT_JOB_MEMORY` set and `JobMemoryLimit` set to the `percent_physical_memory` percent of the physical memory in bytes. **]**

**SRS_JOB_OBJECT_HELPER_18_040: [** `job_object_helper_limit_memory` shall call `sm_exec_end`. **]**

**SRS_JOB_OBJECT_HELPER_18_041: [** If there are any failures, `job_object_helper_limit_memory` shall fail and return a non-zero value. **]**

**SRS_JOB_OBJECT_HELPER_18_042: [** `job_object_helper_limit_memory` shall succeed and return `0`. **]**


## job_object_helper_limit_cpu
```c
MOCKABLE_FUNCTION(, int, job_object_helper_limit_cpu, THANDLE(JOB_OBJECT_HELPER), job_object_helper, uint32_t, percent_cpu);
```
`job_object_helper_limit_cpu` limits the amount of CPU available to the current process. 

**SRS_JOB_OBJECT_HELPER_18_043: [** If `job_object_helper` is `NULL`, `job_object_helper_limit_cpu` shall fail and return a non-zero value. **]**

**SRS_JOB_OBJECT_HELPER_18_044: [** If `percent_cpu` is  `0`, `job_object_helper_limit_cpu` shall fail and return a non-zero value. **]**

**SRS_JOB_OBJECT_HELPER_18_045: [** If `percent_cpu` is greater than `100`, `job_object_helper_limit_cpu` shall fail and return a non-zero value. **]**

**SRS_JOB_OBJECT_HELPER_18_046: [** `job_object_helper_limit_cpu` shall call `sm_exec_begin`. **]**

**SRS_JOB_OBJECT_HELPER_18_047: [** `job_object_helper_limit_cpu` shall call `SetInformationJobObject` passing `JobObjectCpuRateControlInformation` and a `JOBOBJECT_CPU_RATE_CONTROL_INFORMATION` object with `JOB_OBJECT_CPU_RATE_CONTROL_ENABLE` and `JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP` set, and `CpuRate` set to `percent_cpu` times `100`. **]**

**SRS_JOB_OBJECT_HELPER_18_048: [** `job_object_helper_limit_cpu` shall call `sm_exec_end`. **]**

**SRS_JOB_OBJECT_HELPER_18_049: [** If there are any failures, `job_object_helper_limit_cpu` shall fail and return a non-zero value. **]**

**SRS_JOB_OBJECT_HELPER_18_050: [** `job_object_helper_limit_cpu` shall succeed and return `0`. **]**
