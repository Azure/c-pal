# sysinfo_win32

## Overview

`sysinfo_win32` provides the Windows implementation for `sysinfo`.

## Exposed API

```c
MOCKABLE_FUNCTION(, uint32_t, sysinfo_get_processor_count);
```

### sysinfo_get_processor_count

```c
MOCKABLE_FUNCTION(, uint32_t, sysinfo_get_processor_count);
```

`sysinfo_get_processor_count` returns the processor count as returned by the operating system.

**SRS_SYSINFO_WIN32_43_001: [** `sysinfo_get_processor_count` shall call `GetActiveProcessorCount(ALL_PROCESSOR_GROUPS)` to obtain the number of processors. **]**

**SRS_SYSINFO_WIN32_43_002: [** `sysinfo_get_processor_count` shall return the processor count as returned by `GetActiveProcessorCount`. **]**

**SRS_SYSINFO_WIN32_43_003: [** If there are any failures, `sysinfo_get_processor_count` shall fail and return zero. **]**