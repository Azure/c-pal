# job_object_help requirements

`job_object_help` is a module that wraps the Windows JobObject API, primarily for the purpose of limiting the amount of memory and CPU the current process can consume.

## Exposed API
```c
    MOCKABLE_FUNCTION(, int, job_object_helper_limit_resources, uint32_t, percent_physical_memory, uint32_t, percent_cpu);
```

## job_object_helper_limit_resources
```c
    MOCKABLE_FUNCTION(, int, job_object_helper_limit_resources, uint32_t, percent_physical_memory, uint32_t, percent_cpu);
```

`job_object_helper_limit_resources` is an API that uses the job object API to limit the amount of physical memory and CPU that the current process can consume.

**SRS_JOB_OBJECT_HELPER_18_001: [** If `percent_physical_memory` is `0`, `job_object_helper_limit_resources` shall fail and return a non-zero value. **]**

**SRS_JOB_OBJECT_HELPER_18_002: [** If `percent_cpu` is  `0`, `job_object_helper_limit_resources` shall fail and return a non-zero value. **]**

**SRS_JOB_OBJECT_HELPER_18_004: [** If `percent_physical_memory` is greater than `100`, `job_object_helper_limit_resources` shall fail and return a non-zero value. **]**

**SRS_JOB_OBJECT_HELPER_18_005: [** If `percent_cpu` is greater than `100`, `job_object_helper_limit_resources` shall fail and return a non-zero value. **]**

**SRS_JOB_OBJECT_HELPER_18_006: [** `job_object_helper_limit_resources` shall call `CreateJobObject` to create a new job object passing `NULL` for both `lpJobAttributes` and `lpName`. **]**

**SRS_JOB_OBJECT_HELPER_18_007: [** `job_object_helper_limit_resources` shall call `GetCurrentProcess` to get the current process handle. **]**

**SRS_JOB_OBJECT_HELPER_18_008: [** `job_object_helper_limit_resources` shall call `AssignProcessToJobObject` to assign the current process to the new job object. **]**

**SRS_JOB_OBJECT_HELPER_18_009: [** `job_object_helper_limit_resources` shall call `GlobalMemoryStatusEx` to get the total amount of physical memory in kb. **]**

**SRS_JOB_OBJECT_HELPER_18_010: [** `job_object_helper_limit_resources` shall call `SetInformationJobObject`, passing `JobObjectExtendedLimitInformation` and a `JOBOBJECT_EXTENDED_LIMIT_INFORMATION` object with `JOB_OBJECT_LIMIT_JOB_MEMORY` set and `JobMemoryLimit` set to the `percent_physical_memory` percent of the physical memory in bytes. **]**

**SRS_JOB_OBJECT_HELPER_18_011: [** `job_object_helper_limit_resources` shall call `SetInformationJobObject` passing `JobObjectCpuRateControlInformation` and a `JOBOBJECT_CPU_RATE_CONTROL_INFORMATION` object with `JOB_OBJECT_CPU_RATE_CONTROL_ENABLE` and `JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP` set, and `CpuRate` set to `percent_cpu` times `100`. **]**

**SRS_JOB_OBJECT_HELPER_18_012: [** `job_object_helper_limit_resources` shall call `CloseHandle` to close the handle of the current process. **]**

**SRS_JOB_OBJECT_HELPER_18_013: [** `job_object_helper_limit_resources` shall call `CloseHandle` to close the handle to the job object. **]**

**SRS_JOB_OBJECT_HELPER_18_014: [** `job_object_helper_limit_resources` shall succeed and return `0`. **]**

**SRS_JOB_OBJECT_HELPER_18_015: [** If there are any failures, `job_object_helper_limit_resources` shall fail and return a non-zero value. **]**

