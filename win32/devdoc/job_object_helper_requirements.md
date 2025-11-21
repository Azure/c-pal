# job_object_helper requirements

`job_object_helper` is a module that wraps the Windows JobObject API, primarily for the purpose of limiting the amount of memory and CPU the current process can consume.

## Exposed API
```c

typedef struct JOB_OBJECT_HELPER_TAG JOB_OBJECT_HELPER;
THANDLE_TYPE_DECLARE(JOB_OBJECT_HELPER);

MOCKABLE_FUNCTION(, THANDLE(JOB_OBJECT_HELPER), job_object_helper_set_job_limits_to_current_process, const char*, job_name, uint32_t, percent_cpu, uint32_t, percent_physical_memory);
```

## job_object_helper_dispose
```c
static void job_object_helper_dispose(JOB_OBJECT_HELPER* job_object_helper);
```
`job_object_helper_dispose` frees all of the resources used by the `JOB_OBJECT_HELPER` object.

**SRS_JOB_OBJECT_HELPER_18_033: [** `job_object_helper_dispose` shall call `CloseHandle` to close the handle to the job object. **]**


## job_object_helper_set_job_limits_to_current_process
```c
MOCKABLE_FUNCTION(, THANDLE(JOB_OJBECT_HELPER), job_object_helper_set_job_limits_to_current_process, const char*, job_name, uint32_t, percent_cpu, uint32_t, percent_physical_memory);
```
`job_object_helper_set_job_limits_to_current_process` Creates the Job Object with limits if not present and assigns the current process to it. Returns a THANDLE to allow reuse of the job object across multiple processes. If being used only in single process, then handle can be released immediately and process continues having the set limits.

**SRS_JOB_OBJECT_HELPER_19_013: [** If `percent_cpu` is greater than `100` then `job_object_helper_set_job_limits_to_current_process` shall fail and return `NULL`. **]**

**SRS_JOB_OBJECT_HELPER_19_012: [** If `percent_physical_memory` is greater than `100` then `job_object_helper_set_job_limits_to_current_process` shall fail and return `NULL`. **]**

**SRS_JOB_OBJECT_HELPER_19_014: [** If `percent_cpu` and `percent_physical_memory` are `0` then `job_object_helper_set_job_limits_to_current_process` shall fail and return `NULL`. **]**

**SRS_JOB_OBJECT_HELPER_19_015: [** `job_object_helper_set_job_limits_to_current_process` shall allocate a `JOB_OBJECT_HELPER` object. **]**

**SRS_JOB_OBJECT_HELPER_19_002: [** `job_object_helper_set_job_limits_to_current_process` shall call `CreateJobObjectA` passing `job_name` for `lpName` and `NULL` for `lpJobAttributes`. **]**

**SRS_JOB_OBJECT_HELPER_19_003: [** If `percent_cpu` is not `0` then `job_object_helper_set_job_limits_to_current_process` shall call `SetInformationJobObject` passing `JobObjectCpuRateControlInformation` and a `JOBOBJECT_CPU_RATE_CONTROL_INFORMATION` object with `JOB_OBJECT_CPU_RATE_CONTROL_ENABLE` and `JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP` set, and `CpuRate` set to `percent_cpu` times `100`. **]**

**SRS_JOB_OBJECT_HELPER_19_004: [** If `percent_physical_memory` is not `0` then `job_object_helper_set_job_limits_to_current_process` shall call `GlobalMemoryStatusEx` to get the total amount of physical memory in kb. **]**

**SRS_JOB_OBJECT_HELPER_19_005: [** If `percent_physical_memory` is not `0` then `job_object_helper_set_job_limits_to_current_process` shall call `SetInformationJobObject`, passing `JobObjectExtendedLimitInformation` and a `JOBOBJECT_EXTENDED_LIMIT_INFORMATION` object with `JOB_OBJECT_LIMIT_JOB_MEMORY` set and `JobMemoryLimit` set to the `percent_physical_memory` percent of the physical memory in bytes. **]**

**SRS_JOB_OBJECT_HELPER_19_006: [** `job_object_helper_set_job_limits_to_current_process` shall call `GetCurrentProcess` to get the current process handle. **]**

**SRS_JOB_OBJECT_HELPER_19_007: [** `job_object_helper_set_job_limits_to_current_process` shall call `AssignProcessToJobObject` to assign the current process to the new job object. **]**

**SRS_JOB_OBJECT_HELPER_19_008: [** `job_object_helper_set_job_limits_to_current_process` shall call `CloseHandle` to close the handle of the current process. **]**

**SRS_JOB_OBJECT_HELPER_19_009: [** If there are any failures, `job_object_helper_set_job_limits_to_current_process` shall fail and return `NULL`. **]**

**SRS_JOB_OBJECT_HELPER_19_010: [** `job_object_set_job_limits_to_current_process` shall succeed and return a `JOB_OBJECT_HELPER` object. **]**
