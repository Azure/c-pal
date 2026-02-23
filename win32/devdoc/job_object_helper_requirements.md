# job_object_helper requirements

`job_object_helper` is a module that wraps the Windows JobObject API, primarily for the purpose of limiting the amount of memory and CPU the current process can consume.

Note: Windows Job Objects have a critical property: `CloseHandle` on a job object handle does NOT disassociate the process from the job object. If multiple job objects are created and the process is assigned to each, the CPU rate limits compound multiplicatively (e.g., two 50% caps result in 25% effective CPU). Since this module calls `AssignProcessToJobObject` on the current process, it uses a process-level singleton pattern internally — the job object is created once and reused on subsequent calls. If subsequent calls have different parameters, the existing job object limits are reconfigured in-place via `SetInformationJobObject`.

Note: `job_object_helper_set_job_limits_to_current_process` and `job_object_helper_deinit_for_test` are NOT thread-safe. These functions must be called from a single thread. Concurrent calls from multiple threads may result in undefined behavior due to non-atomic singleton initialization and cleanup.

## Exposed API
```c

typedef struct JOB_OBJECT_HELPER_TAG JOB_OBJECT_HELPER;
THANDLE_TYPE_DECLARE(JOB_OBJECT_HELPER);

MOCKABLE_FUNCTION(, THANDLE(JOB_OBJECT_HELPER), job_object_helper_set_job_limits_to_current_process, const char*, job_name, uint32_t, percent_cpu, uint32_t, percent_physical_memory);
MOCKABLE_FUNCTION(, void, job_object_helper_deinit_for_test);
```

## job_object_helper_dispose
```c
static void job_object_helper_dispose(JOB_OBJECT_HELPER* job_object_helper);
```
`job_object_helper_dispose` frees all of the resources used by the `JOB_OBJECT_HELPER` object.

**SRS_JOB_OBJECT_HELPER_18_033: [** `job_object_helper_dispose` shall call `CloseHandle` to close the handle to the job object. **]**

## internal_job_object_helper_set_cpu_limit
```c
static int internal_job_object_helper_set_cpu_limit(HANDLE job_object, uint32_t percent_cpu);
```
`internal_job_object_helper_set_cpu_limit` sets or removes the CPU rate limit on the given job object.

**SRS_JOB_OBJECT_HELPER_88_018: [** If `percent_cpu` is `0`, `internal_job_object_helper_set_cpu_limit` shall call `SetInformationJobObject` passing `JobObjectCpuRateControlInformation` with `ControlFlags` set to `0` to disable CPU rate control. **]**

**SRS_JOB_OBJECT_HELPER_88_022: [** If `percent_cpu` is not `0`, `internal_job_object_helper_set_cpu_limit` shall set `ControlFlags` to `JOB_OBJECT_CPU_RATE_CONTROL_ENABLE` and `JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP`, and `CpuRate` to `percent_cpu` times `100`. **]**

**SRS_JOB_OBJECT_HELPER_88_006: [** `internal_job_object_helper_set_cpu_limit` shall call `SetInformationJobObject` passing `JobObjectCpuRateControlInformation` and the `JOBOBJECT_CPU_RATE_CONTROL_INFORMATION`. **]**

**SRS_JOB_OBJECT_HELPER_88_007: [** If `SetInformationJobObject` fails, `internal_job_object_helper_set_cpu_limit` shall return a non-zero value. **]**

**SRS_JOB_OBJECT_HELPER_88_008: [** `internal_job_object_helper_set_cpu_limit` shall succeed and return `0`. **]**

## internal_job_object_helper_set_memory_limit
```c
static int internal_job_object_helper_set_memory_limit(HANDLE job_object, uint32_t percent_physical_memory);
```
`internal_job_object_helper_set_memory_limit` sets or removes the memory limit on the given job object.

**SRS_JOB_OBJECT_HELPER_88_019: [** If `percent_physical_memory` is `0`, `internal_job_object_helper_set_memory_limit` shall call `SetInformationJobObject` passing `JobObjectExtendedLimitInformation` with `LimitFlags` set to `0` to remove memory limits. **]**

**SRS_JOB_OBJECT_HELPER_88_009: [** If `percent_physical_memory` is not `0`, `internal_job_object_helper_set_memory_limit` shall call `GlobalMemoryStatusEx` to get the total amount of physical memory. **]**

**SRS_JOB_OBJECT_HELPER_88_010: [** If `percent_physical_memory` is not `0`, `internal_job_object_helper_set_memory_limit` shall call `SetInformationJobObject`, passing `JobObjectExtendedLimitInformation` and a `JOBOBJECT_EXTENDED_LIMIT_INFORMATION` object with `JOB_OBJECT_LIMIT_JOB_MEMORY` and `JOB_OBJECT_LIMIT_PROCESS_MEMORY` set and `JobMemoryLimit` and `ProcessMemoryLimit` set to `percent_physical_memory` percent of the physical memory in bytes. **]**

**SRS_JOB_OBJECT_HELPER_88_011: [** If there are any failures, `internal_job_object_helper_set_memory_limit` shall return a non-zero value. **]**

**SRS_JOB_OBJECT_HELPER_88_012: [** `internal_job_object_helper_set_memory_limit` shall succeed and return `0`. **]**

## job_object_helper_set_job_limits_to_current_process
```c
MOCKABLE_FUNCTION(, THANDLE(JOB_OBJECT_HELPER), job_object_helper_set_job_limits_to_current_process, const char*, job_name, uint32_t, percent_cpu, uint32_t, percent_physical_memory);
```
`job_object_helper_set_job_limits_to_current_process` creates the Job Object with limits if not present and assigns the current process to it. Returns a THANDLE to allow reuse of the job object across multiple processes. If being used only in single process, then handle can be released immediately and process continues having the set limits.

The function implements a process-level singleton pattern to prevent Job Object accumulation. On the first call, it creates the job object and remembers the parameters. On subsequent calls with the same parameters, it returns the existing singleton. If parameters change, the existing job object is reconfigured in-place via `SetInformationJobObject`. During reconfiguration, a limit can be removed by setting it to `0`.

**SRS_JOB_OBJECT_HELPER_19_013: [** If `percent_cpu` is greater than `100` then `job_object_helper_set_job_limits_to_current_process` shall fail and return `NULL`. **]**

**SRS_JOB_OBJECT_HELPER_19_012: [** If `percent_physical_memory` is greater than `100` then `job_object_helper_set_job_limits_to_current_process` shall fail and return `NULL`. **]**

**SRS_JOB_OBJECT_HELPER_88_001: [** If the singleton has not been created and the requested limits are effectively no limits (both `percent_cpu` and `percent_physical_memory` are `0`, or both are `100`), `job_object_helper_set_job_limits_to_current_process` shall return `NULL` without creating a job object. **]**

**SRS_JOB_OBJECT_HELPER_88_002: [** If the process-level singleton job object has already been created with the same `percent_cpu` and `percent_physical_memory` values, `job_object_helper_set_job_limits_to_current_process` shall increment the reference count on the existing `THANDLE(JOB_OBJECT_HELPER)` and return it. **]**

**SRS_JOB_OBJECT_HELPER_88_014: [** If the process-level singleton job object has already been created with different `percent_cpu` or `percent_physical_memory` values, `job_object_helper_set_job_limits_to_current_process` shall reconfigure the existing job object. **]**

**SRS_JOB_OBJECT_HELPER_88_015: [** If `percent_cpu` has changed, `job_object_helper_set_job_limits_to_current_process` shall call `internal_job_object_helper_set_cpu_limit` to update the CPU limit on the existing job object. **]**

**SRS_JOB_OBJECT_HELPER_88_016: [** If `percent_physical_memory` has changed, `job_object_helper_set_job_limits_to_current_process` shall call `internal_job_object_helper_set_memory_limit` to update the memory limit on the existing job object. **]**

**SRS_JOB_OBJECT_HELPER_88_017: [** If reconfiguration succeeds, `job_object_helper_set_job_limits_to_current_process` shall update the stored `percent_cpu` and `percent_physical_memory` values, increment the reference count on the existing `THANDLE(JOB_OBJECT_HELPER)` and return it. **]**

**SRS_JOB_OBJECT_HELPER_88_020: [** If memory limit fails after CPU limit succeeded, `job_object_helper_set_job_limits_to_current_process` shall attempt to rollback the CPU limit to its original value. **]**

**SRS_JOB_OBJECT_HELPER_88_021: [** If rollback fails, `job_object_helper_set_job_limits_to_current_process` shall update the singleton state to reflect the actual job object state (new CPU, original memory) to maintain consistency. **]**

**SRS_JOB_OBJECT_HELPER_19_015: [** `job_object_helper_set_job_limits_to_current_process` shall allocate a `JOB_OBJECT_HELPER` object. **]**

**SRS_JOB_OBJECT_HELPER_19_002: [** `job_object_helper_set_job_limits_to_current_process` shall call `CreateJobObjectA` passing `job_name` for `lpName` and `NULL` for `lpJobAttributes`. **]**

**SRS_JOB_OBJECT_HELPER_19_003: [** If `percent_cpu` is not `0` then `job_object_helper_set_job_limits_to_current_process` shall call `internal_job_object_helper_set_cpu_limit` to set the CPU limit. **]**

**SRS_JOB_OBJECT_HELPER_19_004: [** If `percent_physical_memory` is not `0` then `job_object_helper_set_job_limits_to_current_process` shall call `internal_job_object_helper_set_memory_limit` to set the memory limit. **]**

**SRS_JOB_OBJECT_HELPER_19_006: [** `job_object_helper_set_job_limits_to_current_process` shall call `GetCurrentProcess` to get the current process handle. **]**

**SRS_JOB_OBJECT_HELPER_19_007: [** `job_object_helper_set_job_limits_to_current_process` shall call `AssignProcessToJobObject` to assign the current process to the new job object. **]**

**SRS_JOB_OBJECT_HELPER_19_009: [** If there are any failures, `job_object_helper_set_job_limits_to_current_process` shall fail and return `NULL`. **]**

**SRS_JOB_OBJECT_HELPER_88_004: [** On success, `job_object_helper_set_job_limits_to_current_process` shall store the `THANDLE(JOB_OBJECT_HELPER)` and the `percent_cpu` and `percent_physical_memory` values in static variables for the singleton pattern. **]**

**SRS_JOB_OBJECT_HELPER_19_010: [** `job_object_helper_set_job_limits_to_current_process` shall succeed and return a `JOB_OBJECT_HELPER` object. **]**

## job_object_helper_deinit_for_test
```c
MOCKABLE_FUNCTION(, void, job_object_helper_deinit_for_test);
```
`job_object_helper_deinit_for_test` releases the process-level singleton and resets stored parameters, allowing a fresh job object to be created on the next call. This is intended for test cleanup and should not be used in production code, as calling it would allow creating new job objects that compound with the existing one.

**SRS_JOB_OBJECT_HELPER_88_005: [** `job_object_helper_deinit_for_test` shall release the singleton `THANDLE(JOB_OBJECT_HELPER)` and reset the stored parameters to zero. **]**
