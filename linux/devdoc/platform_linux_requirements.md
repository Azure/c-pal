# platform_linux

## Overview

`platform_linux` provides initializes the completion_port object.

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

`platform_init` performs platform initialization for Linux. This includes warming up `getaddrinfo` (needed for avoiding init races) and initializing the Linux equivalent of completion ports.

**SRS_PLATFORM_LINUX_11_007: [** If the completion port object is non-NULL, `platform_init` shall return zero. **]**

**SRS_PLATFORM_LINUX_01_001: [** Otherwise, `platform_init` shall call `getaddrinfo` for `localhost` and port `4242`. **]**

**SRS_PLATFORM_LINUX_11_001: [** `platform_init` shall call `completion_port_create`. **]**

**SRS_PLATFORM_LINUX_11_002: [** `platform_init` shall succeed and return zero. **]**

**SRS_PLATFORM_LINUX_01_002: [** If any error occurs, `platform_init` shall return a non-zero value. **]**

### platform_deinit

```c
MOCKABLE_FUNCTION(, void, platform_deinit);
```

`platform_deinit` .

**SRS_PLATFORM_LINUX_11_004: [** If the completion port object is non-NULL, `platform_deinit` shall decrement whose reference by calling `completion_port_dec_ref`. **]**

**SRS_PLATFORM_LINUX_11_008: [** If the completion port object is non-NULL, `platform_deinit` shall do nothing. **]**

### platform_get_completion_port

```c
MOCKABLE_FUNCTION(, COMPLETION_PORT_HANDLE, platform_get_completion_port);
```

`platform_get_completion_port` returns the completion port object reference.

**SRS_PLATFORM_LINUX_11_005: [** If the completion object is not NULL, `platform_get_completion_port` shall increment the reference count of the `COMPLETION_PORT_HANDLE` object by calling `completion_port_inc_ref`. **]**

**SRS_PLATFORM_LINUX_11_006: [** `platform_get_completion_port` shall return the completion object. **]**
