# job_object_helper requirements

`job_object_helper` is a module that wraps the Windows JobObject API, primarily for the purpose of limiting the amount of memory and CPU the current process can consume.

## Exposed API
```c

typedef struct JOB_OBJECT_HELPER_TAG JOB_OBJECT_HELPER;
THANDLE_TYPE_DECLARE(JOB_OBJECT_HELPER);

typedef struct PROCESS_HANDLE_TAG PROCESS_HANDLE;
THANDLE_TYPE_DECLARE(PROCESS_HANDLE);

MOCKABLE_FUNCTION(, THANDLE(JOB_OBJECT_HELPER), job_object_helper_create);
MOCKABLE_FUNCTION(, THANDLE(JOB_OBJECT_HELPER), job_object_helper_create_with_name, const char*, job_name);
MOCKABLE_FUNCTION(, THANDLE(JOB_OBJECT_HELPER), job_object_helper_get, const char*, job_name);
MOCKABLE_FUNCTION(, int, job_object_helper_assign_process, THANDLE(JOB_OBJECT_HELPER), job_object_helper, THANDLE(PROCESS_HANDLE), process_hndl);
MOCKABLE_FUNCTION(, int, job_object_helper_limit_memory, THANDLE(JOB_OBJECT_HELPER), job_object_helper, uint32_t, percent_physical_memory);
MOCKABLE_FUNCTION(, int, job_object_helper_limit_cpu, THANDLE(JOB_OBJECT_HELPER), job_object_helper, uint32_t, percent_cpu);
```

## job_object_helper_create
```c
MOCKABLE_FUNCTION(, THANDLE(JOB_OBJECT_HELPER), job_object_helper_create);
```
`job_object_helper_create` is used to create a `JOB_OBJECT_HELPER` object.

**SRS_JOB_OBJECT_HELPER_18_016: [** `job_object_helper_create` shall allocate a `JOB_OBJECT_HELPER` object. **]**

**SRS_JOB_OBJECT_HELPER_18_024: [** `job_object_helper_create` shall call `CreateJobObject` to create a new job object passing `NULL` for both `lpJobAttributes` and `lpName`. **]**

**SRS_JOB_OBJECT_HELPER_18_025: [** `job_object_helper_create` shall call `GetCurrentProcess` to get the current process handle. **]**

**SRS_JOB_OBJECT_HELPER_18_026: [** `job_object_helper_create` shall call `AssignProcessToJobObject` to assign the current process to the new job object. **]**

**SRS_JOB_OBJECT_HELPER_18_027: [** `job_object_helper_create` shall call `CloseHandle` to close the handle of the current process. **]**

**SRS_JOB_OBJECT_HELPER_18_018: [** If there are any failures, `job_object_helper_create` shall fail and return `NULL`. **]**

**SRS_JOB_OBJECT_HELPER_18_019: [** `job_object_helper_create` shall succeed and return the `JOB_OBJECT_HELPER` object.  **]**


## job_object_helper_create_with_name
```c
MOCKABLE_FUNCTION(, THANDLE(JOB_OBJECT_HELPER), job_object_helper_create_with_name, const char*, job_name);
```
`job_object_helper_create_with_name` is used to create a Job Object with the specified name.

**SRS_JOB_OBJECT_HELPER_19_001: [** If `job_name` is `NULL` or empty then `job_object_helper_create_with_name` shall fail and return `NULL`. **]**

**SRS_JOB_OBJECT_HELPER_19_002: [** `job_object_helper_create_with_name` shall allocate `JOB_OBJECT_HELPER` object. **]**

**SRS_JOB_OBJECT_HELPER_19_016: [** `job_object_helper_create_with_name` shall call `mbs_to_wcs` to convert job_name to `wchar_t*`. **]**

**SRS_JOB_OBJECT_HELPER_19_003: [** `job_object_helper_create_with_name` shall call `CreateJobObject` to create a new job object passing `NULL` for `lpJobAttributes` and `job_name` to `lpName`. **]**

**SRS_JOB_OBJECT_HELPER_19_004: [** If there are any failures then `job_object_helper_create_with_name` shall fail and return `NULL`. **]**

**SRS_JOB_OBJECT_HELPER_19_005: [** `job_object_helper_create_with_name` shall succeed and return the `JOB_OBJECT_HELPER` object. **]**


## job_object_helper_get
```c
MOCKABLE_FUNCTION(, THANDLE(JOB_OBJECT_HELPER), job_object_helper_get, const char*, job_name);
```
`job_object_helper_get` is used for getting a `JOB_OBJECT_HELPER` created with the name `job_name`.

**SRS_JOB_OBJECT_HELPER_19_006: [** If the `job_name` is `NULL` or empty then `job_object_helper_get` shall fail and return `NULL` **]**

**SRS_JOB_OBJECT_HELPER_19_017: [** `job_object_helper_get` shall call `mbs_to_wcs` to convert the `job_name` to `wchar_t*` type. **]**

**SRS_JOB_OBJECT_HELPER_19_007: [** `job_object_helper_get` shall call `OpenJobObject` with `lpName` parameter set to `job_name`. **]**

**SRS_JOB_OBJECT_HELPER_19_008: [** If there are any failures then `job_object_helper_get` shall return `NULL`. **]**

**SRS_JOB_OBJECT_HELPER_19_009: [** `job_object_helper_get` shall succeed and return the `JOB_OBJECT_HELPER` object. **]**


## job_object_helper_assign_process
```c
MOCKABLE_FUNCTION(, int, job_object_helper_assign_process, THANDLE(JOB_OBJECT_HELPER), job_object_helper, THANDLE(PROCESS_HANDLE), process_handle);
```
`job_object_helper_assign_process` assigns the process to the Job Object.

**SRS_JOB_OBJECT_HELPER_19_010: [** If the `job_object_helper` is `NULL` then `job_object_helper_assign_process` shall fail and return a non-zero value. **]**

**SRS_JOB_OBJECT_HELPER_19_020: [** If the `process_handle` is null, then `job_object_helper_assign_process` shall set it to the `GetCurrentProcess()` output. **]**

**SRS_JOB_OBJECT_HELPER_19_011: [** `job_object_helper_assign_process` shall call `OpenJobObject` with Permission `JOB_OBJECT_ASSIGN_PROCESS`. **]**

**SRS_JOB_OBJECT_HELPER_19_013: [** `job_object_helper_assign_process` shall call `AssignProcessToJobObject` to assign the `process_handle` to the job object. **]**

**SRS_JOB_OBJECT_HELPER_19_014: [** If there are any failures then `job_object_helper_assign_process` shall fail and return a non-zero value. **]**

**SRS_JOB_OBJECT_HELPER_19_015: [** `job_object_helper_assign_process` shall succeed and return `0`. **]**


## job_object_helper_dispose
```c
static void job_object_helper_dispose(JOB_OBJECT_HELPER* job_object_helper);
```
`job_object_helper_dispose` frees all of the resources used by the `JOB_OBJECT_HELPER` object.

**SRS_JOB_OBJECT_HELPER_18_033: [** `job_object_helper_dispose` shall call `CloseHandle` to close the handle to the job object. **]**


## job_object_helper_limit_memory
```c
MOCKABLE_FUNCTION(, int, job_object_helper_limit_memory, THANDLE(JOB_OBJECT_HELPER), job_object_helper, uint32_t, percent_physical_memory);
```
`job_object_helper_limit_memory` limits the amount of physical memory available to the current process.

**SRS_JOB_OBJECT_HELPER_18_035: [** If `job_object_helper` is `NULL`, `job_object_helper_limit_memory` shall fail and return a non-zero value. **]**

**SRS_JOB_OBJECT_HELPER_18_036: [** If `percent_physical_memory` is `0`, `job_object_helper_limit_memory` shall fail and return a non-zero value. **]**

**SRS_JOB_OBJECT_HELPER_18_037: [** If `percent_physical_memory` is greater than `100`, `job_object_helper_limit_memory` shall fail and return a non-zero value. **]**

**SRS_JOB_OBJECT_HELPER_18_023: [** `job_object_helper_limit_memory` shall call `GlobalMemoryStatusEx` to get the total amount of physical memory in kb. **]**

**SRS_JOB_OBJECT_HELPER_18_039: [** `job_object_helper_limit_memory` shall call `SetInformationJobObject`, passing `JobObjectExtendedLimitInformation` and a `JOBOBJECT_EXTENDED_LIMIT_INFORMATION` object with `JOB_OBJECT_LIMIT_JOB_MEMORY` set and `JobMemoryLimit` set to the `percent_physical_memory` percent of the physical memory in bytes. **]**

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

**SRS_JOB_OBJECT_HELPER_18_047: [** `job_object_helper_limit_cpu` shall call `SetInformationJobObject` passing `JobObjectCpuRateControlInformation` and a `JOBOBJECT_CPU_RATE_CONTROL_INFORMATION` object with `JOB_OBJECT_CPU_RATE_CONTROL_ENABLE` and `JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP` set, and `CpuRate` set to `percent_cpu` times `100`. **]**

**SRS_JOB_OBJECT_HELPER_18_049: [** If there are any failures, `job_object_helper_limit_cpu` shall fail and return a non-zero value. **]**

**SRS_JOB_OBJECT_HELPER_18_050: [** `job_object_helper_limit_cpu` shall succeed and return `0`. **]**
