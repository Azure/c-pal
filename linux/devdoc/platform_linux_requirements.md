# platform_linux

## Overview

`platform_linux` provides initializes the completion_port system.

## Exposed API

```c
MOCKABLE_FUNCTION(, int, platform_init);
MOCKABLE_FUNCTION(, void, platform_deinit);
MOCKABLE_FUNCTION(, COMPLETION_PORT_HANDLE, platform_get_completion_port);
```

### platform_init

```c
MOCKABLE_FUNCTION(, int, platform_init);
```

`platform_init` initializes the completion port object if it's not initialized

**SRS_PLATFORM_LINUX_11_001: [** If the completion port object is NULL, `platform_init` shall call `completion_port_create`. **]**

**SRS_PLATFORM_LINUX_11_002: [** If `completion_port_create` returns a valid completion port object, `platform_init` shall return zero. **]**

**SRS_PLATFORM_LINUX_11_007: [** If the completion port object is non-NULL, `platform_init` shall return zero. **]**

**SRS_PLATFORM_LINUX_11_003: [** otherwise, `platform_init` shall return a non-zero value. **]**

### platform_deinit

```c
MOCKABLE_FUNCTION(, void, platform_deinit);
```

`platform_deinit` .

**SRS_PLATFORM_LINUX_11_004: [** If the completion port object is non-NULL, `platform_deinit` shall decrement the reference by calling `completion_port_dec_ref`. **]**

**SRS_PLATFORM_LINUX_11_008: [** If the completion port object is non-NULL, `platform_deinit` shall do nothing. **]**

### platform_get_completion_port

```c
MOCKABLE_FUNCTION(, COMPLETION_PORT_HANDLE, platform_get_completion_port);
```

`platform_get_completion_port` returns the completion port object reference.

**SRS_PLATFORM_LINUX_11_006: [** `platform_get_completion_port` shall return the completion object. **]**
