# job_object_helper requirements

`job_object_helper` is a module that wraps the Windows JobObject API, primarily for the purpose of limiting the amount of memory and CPU the current process can consume.

Note: Windows Job Objects have a critical property: `CloseHandle` on a job object handle does NOT disassociate the process from the job object. If multiple job objects are created and the process is assigned to each, the CPU rate limits compound multiplicatively (e.g., two 50% caps result in 25% effective CPU). Since this module calls `AssignProcessToJobObject` on the current process, it uses a process-level singleton pattern internally — the job object is created once and reused on subsequent calls. If subsequent calls have different parameters, the existing job object limits are reconfigured in-place via `SetInformationJobObject`. During reconfiguration, CPU and memory limits are updated independently. If one succeeds but the other fails, the function returns failure even though the successful change has already been applied to the Windows job object. The caller can simply retry to re-apply the limits.

Note: `job_object_helper_set_job_limits_to_current_process` and `job_object_helper_deinit_for_test` are NOT thread-safe. These functions must be called from a single thread. Concurrent calls from multiple threads may result in undefined behavior due to non-atomic singleton initialization and cleanup.


## Exposed API
```c
// Passing JOB_OBJECT_HELPER_DISABLE_CPU_RATE_CONTROL as percent_cpu disables CPU rate control (removes the throttle)
#define JOB_OBJECT_HELPER_DISABLE_CPU_RATE_CONTROL 0
// Passing JOB_OBJECT_HELPER_DISABLE_MEMORY_LIMIT as percent_physical_memory removes memory limits
#define JOB_OBJECT_HELPER_DISABLE_MEMORY_LIMIT 0

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
`internal_job_object_helper_set_cpu_limit` is an internal function that sets or disables the CPU rate control on the given job object by calling the Windows `SetInformationJobObject` API.

**SRS_JOB_OBJECT_HELPER_88_004: [** If `percent_cpu` is `JOB_OBJECT_HELPER_DISABLE_CPU_RATE_CONTROL`, `internal_job_object_helper_set_cpu_limit` shall call `SetInformationJobObject` passing `JobObjectCpuRateControlInformation` with `ControlFlags` set to `0` to disable CPU rate control. **]**

**SRS_JOB_OBJECT_HELPER_88_005: [** If `percent_cpu` is not `JOB_OBJECT_HELPER_DISABLE_CPU_RATE_CONTROL`, `internal_job_object_helper_set_cpu_limit` shall set `ControlFlags` to `JOB_OBJECT_CPU_RATE_CONTROL_ENABLE` and `JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP`, and `CpuRate` to `percent_cpu` times `100`. **]**

**SRS_JOB_OBJECT_HELPER_88_006: [** `internal_job_object_helper_set_cpu_limit` shall call `SetInformationJobObject` passing `JobObjectCpuRateControlInformation` and the `JOBOBJECT_CPU_RATE_CONTROL_INFORMATION`. **]**

**SRS_JOB_OBJECT_HELPER_88_007: [** If `SetInformationJobObject` fails, `internal_job_object_helper_set_cpu_limit` shall fail and return a non-zero value. **]**

**SRS_JOB_OBJECT_HELPER_88_008: [** `internal_job_object_helper_set_cpu_limit` shall succeed and return `0`. **]**


## internal_job_object_helper_set_memory_limit
```c
static int internal_job_object_helper_set_memory_limit(HANDLE job_object, uint32_t percent_physical_memory);
```
`internal_job_object_helper_set_memory_limit` is an internal function that sets or removes the memory limit on the given job object by calling the Windows `SetInformationJobObject` API.

**SRS_JOB_OBJECT_HELPER_88_010: [** If `percent_physical_memory` is `JOB_OBJECT_HELPER_DISABLE_MEMORY_LIMIT`, `internal_job_object_helper_set_memory_limit` shall call `SetInformationJobObject` passing `JobObjectExtendedLimitInformation` with `LimitFlags` set to `0` to remove memory limits. **]**

**SRS_JOB_OBJECT_HELPER_88_011: [** If `percent_physical_memory` is not `JOB_OBJECT_HELPER_DISABLE_MEMORY_LIMIT`, `internal_job_object_helper_set_memory_limit` shall call `GlobalMemoryStatusEx` to get the total amount of physical memory. **]**

**SRS_JOB_OBJECT_HELPER_88_012: [** If `percent_physical_memory` is not `JOB_OBJECT_HELPER_DISABLE_MEMORY_LIMIT`, `internal_job_object_helper_set_memory_limit` shall set `JobMemoryLimit` and `ProcessMemoryLimit` to `percent_physical_memory` percent of the physical memory and call `SetInformationJobObject` with `JobObjectExtendedLimitInformation`. **]**

**SRS_JOB_OBJECT_HELPER_88_013: [** If there are any failures, `internal_job_object_helper_set_memory_limit` shall fail and return a non-zero value. **]**

**SRS_JOB_OBJECT_HELPER_88_014: [** `internal_job_object_helper_set_memory_limit` shall succeed and return `0`. **]**


## internal_job_object_helper_reconfigure
```c
static int internal_job_object_helper_reconfigure(uint32_t percent_cpu, uint32_t percent_physical_memory);
```
`internal_job_object_helper_reconfigure` is an internal function that reconfigures the existing process-level singleton job object with new CPU and memory limits.

**SRS_JOB_OBJECT_HELPER_88_003: [** `internal_job_object_helper_reconfigure` shall call `internal_job_object_helper_set_cpu_limit` to apply the CPU rate control to the Windows job object. **]**

**SRS_JOB_OBJECT_HELPER_88_009: [** `internal_job_object_helper_reconfigure` shall call `internal_job_object_helper_set_memory_limit` to apply the memory limit to the Windows job object. **]**

**SRS_JOB_OBJECT_HELPER_88_017: [** If there are any failures, `internal_job_object_helper_reconfigure` shall fail and return a non-zero value. **]**

**SRS_JOB_OBJECT_HELPER_88_020: [** On successful reconfiguration, `internal_job_object_helper_reconfigure` shall return `0`. **]**


## internal_job_object_helper_create
```c
static int internal_job_object_helper_create(const char* job_name, uint32_t percent_cpu, uint32_t percent_physical_memory);
```
`internal_job_object_helper_create` is an internal function that creates a new job object, applies CPU and memory limits, assigns it to the current process, and stores it in the process-level singleton state.

**SRS_JOB_OBJECT_HELPER_88_035: [** `internal_job_object_helper_create` shall allocate a `JOB_OBJECT_HELPER` object. **]**

**SRS_JOB_OBJECT_HELPER_88_036: [** `internal_job_object_helper_create` shall call `CreateJobObjectA` passing `job_name` for `lpName` and `NULL` for `lpJobAttributes`. **]**

**SRS_JOB_OBJECT_HELPER_88_037: [** `internal_job_object_helper_create` shall call `internal_job_object_helper_set_cpu_limit` to apply the CPU rate control to the Windows job object. **]**

**SRS_JOB_OBJECT_HELPER_88_038: [** `internal_job_object_helper_create` shall call `internal_job_object_helper_set_memory_limit` to apply the memory limit to the Windows job object. **]**

**SRS_JOB_OBJECT_HELPER_88_039: [** `internal_job_object_helper_create` shall call `GetCurrentProcess` to get the current process handle. **]**

**SRS_JOB_OBJECT_HELPER_19_008: [** `internal_job_object_helper_create` shall call `AssignProcessToJobObject` to assign the current process to the new job object. **]**

**SRS_JOB_OBJECT_HELPER_88_024: [** On success, `internal_job_object_helper_create` shall store the `THANDLE(JOB_OBJECT_HELPER)` in the process-level singleton state. **]**

**SRS_JOB_OBJECT_HELPER_88_032: [** If there are any failures, `internal_job_object_helper_create` shall fail and return a non-zero value. **]**

**SRS_JOB_OBJECT_HELPER_88_033: [** `internal_job_object_helper_create` shall succeed and return `0`. **]**


## job_object_helper_set_job_limits_to_current_process
```c
MOCKABLE_FUNCTION(, THANDLE(JOB_OBJECT_HELPER), job_object_helper_set_job_limits_to_current_process, const char*, job_name, uint32_t, percent_cpu, uint32_t, percent_physical_memory);
```
`job_object_helper_set_job_limits_to_current_process` creates the Job Object with limits if not present and assigns the current process to it. Returns a THANDLE to allow reuse of the job object across multiple processes. If being used only in single process, then handle can be released immediately and process continues having the set limits.

The function implements a process-level singleton pattern to prevent Job Object accumulation. On the first call, it creates the job object. On subsequent calls, the existing job object's limits are applied in-place via `SetInformationJobObject` and the existing singleton is returned. CPU rate control can be disabled by passing `JOB_OBJECT_HELPER_DISABLE_CPU_RATE_CONTROL` and memory limits can be removed by passing `JOB_OBJECT_HELPER_DISABLE_MEMORY_LIMIT`.

**SRS_JOB_OBJECT_HELPER_19_001: [** If `percent_cpu` is greater than `100` then `job_object_helper_set_job_limits_to_current_process` shall fail and return `NULL`. **]**

**SRS_JOB_OBJECT_HELPER_88_034: [** If `percent_physical_memory` is greater than `100` then `job_object_helper_set_job_limits_to_current_process` shall fail and return `NULL`. **]**

**SRS_JOB_OBJECT_HELPER_88_030: [** If `job_object_singleton_state.job_object_helper` is not `NULL`, `job_object_helper_set_job_limits_to_current_process` shall not create a new job object. **]**

**SRS_JOB_OBJECT_HELPER_88_002: [** If `job_object_singleton_state.job_object_helper` is not `NULL`, `job_object_helper_set_job_limits_to_current_process` shall call `internal_job_object_helper_reconfigure` to apply the limits to the existing job object. **]**

**SRS_JOB_OBJECT_HELPER_88_021: [** If `internal_job_object_helper_reconfigure` returns `0`, `job_object_helper_set_job_limits_to_current_process` shall increment the reference count on the existing `THANDLE(JOB_OBJECT_HELPER)` and return it. **]**

**SRS_JOB_OBJECT_HELPER_88_022: [** If `job_object_singleton_state.job_object_helper` is `NULL` and `percent_cpu` is `JOB_OBJECT_HELPER_DISABLE_CPU_RATE_CONTROL` and `percent_physical_memory` is `JOB_OBJECT_HELPER_DISABLE_MEMORY_LIMIT`, `job_object_helper_set_job_limits_to_current_process` shall return `NULL` without creating a job object. **]**

**SRS_JOB_OBJECT_HELPER_88_023: [** If `job_object_singleton_state.job_object_helper` is `NULL` and both `percent_cpu` and `percent_physical_memory` are `100`, `job_object_helper_set_job_limits_to_current_process` shall return `NULL` without creating a job object. **]**

**SRS_JOB_OBJECT_HELPER_88_031: [** `job_object_helper_set_job_limits_to_current_process` shall call `internal_job_object_helper_create` to create a new job object and assign it to the current process. **]**

**SRS_JOB_OBJECT_HELPER_19_009: [** If there are any failures, `job_object_helper_set_job_limits_to_current_process` shall fail and return `NULL`. **]**

**SRS_JOB_OBJECT_HELPER_19_010: [** `job_object_helper_set_job_limits_to_current_process` shall succeed and return a `JOB_OBJECT_HELPER` object. **]**


## job_object_helper_deinit_for_test
```c
MOCKABLE_FUNCTION(, void, job_object_helper_deinit_for_test);
```
`job_object_helper_deinit_for_test` releases the process-level singleton, allowing a fresh job object to be created on the next call. This is intended for test cleanup and should not be used in production code, as calling it would allow creating new job objects that compound with the existing one.

**SRS_JOB_OBJECT_HELPER_88_027: [** `job_object_helper_deinit_for_test` shall release the singleton `THANDLE(JOB_OBJECT_HELPER)` by assigning it to `NULL`. **]**
