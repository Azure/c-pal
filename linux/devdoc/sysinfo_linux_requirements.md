# sysinfo_linux

## Overview

`sysinfo_linux` provides the Linux implementation for `sysinfo`.

## Exposed API

```c
MOCKABLE_FUNCTION(, uint32_t, sysinfo_get_processor_count);
```

### sysinfo_get_processor_count

```c
MOCKABLE_FUNCTION(, uint32_t, sysinfo_get_processor_count);
```

`sysinfo_get_processor_count` returns the processor count as returned by the operating system.

**SRS_SYSINFO_LINUX_01_001: [** `sysinfo_get_processor_count` shall call `sysconf` with `SC_NPROCESSORS_ONLN` to obtain the number of configured processors. **]**

**SRS_SYSINFO_LINUX_01_002: [** If any error occurs, `sysinfo_get_processor_count` shall return 0. **]**

**SRS_SYSINFO_LINUX_01_003: [** If `sysconf` returns a number bigger than `UINT32_MAX`, `sysinfo_get_processor_count` shall fail and return 0. **]**
