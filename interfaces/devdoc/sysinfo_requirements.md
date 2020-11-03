# sysinfo
================

## Overview

`sysinfo` provides platform-independent primitives to obtain system information (like processor count).

## Exposed API

```c
MOCKABLE_FUNCTION(, uint32_t, sysinfo_get_processor_count);
```

### sysinfo_get_processor_count

```c
MOCKABLE_FUNCTION(, uint32_t, sysinfo_get_processor_count);
```

`sysinfo_get_processor_count` gets the processor count.

**SRS_SYSINFO_01_001: [** `sysinfo_get_processor_count` shall obtain the processor count as reported by the operating system. **]**

**SRS_SYSINFO_01_002: [** If any error occurs, `sysinfo_get_processor_count` shall return 0. **]**
