# sysinfo_win32
================

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

**SRS_SYSINFO_WIN32_01_001: [** `sysinfo_get_processor_count` shall call `GetSystemInfo` to obtain the system information. **]**

**SRS_SYSINFO_WIN32_01_002: [** `sysinfo_get_processor_count` shall return the processor count as returned by `GetSystemInfo`. **]**
