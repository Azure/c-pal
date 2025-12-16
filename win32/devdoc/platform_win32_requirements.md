# platform_win32

## Overview

`platform_win32` provides the Windows implementation for platform initialization and deinitialization.

## Exposed API

```c
MOCKABLE_FUNCTION(, int, platform_init);
MOCKABLE_FUNCTION(, void, platform_deinit);
```

### platform_init

```c
MOCKABLE_FUNCTION(, int, platform_init);
```

`platform_init` initializes the platform-specific resources required for the application.

**SRS_PLATFORM_WIN32_01_001: [** `platform_init` shall call `WSAStartup` to initialize the Windows Sockets library with version 2.2. **]**

**SRS_PLATFORM_WIN32_01_002: [** If `WSAStartup` fails, `platform_init` shall return a non-zero value. **]**

**SRS_PLATFORM_WIN32_01_003: [** If `WSAStartup` succeeds, `platform_init` shall return 0. **]**

### platform_deinit

```c
MOCKABLE_FUNCTION(, void, platform_deinit);
```

`platform_deinit` deinitializes the platform-specific resources.

**SRS_PLATFORM_WIN32_01_004: [** `platform_deinit` shall call `WSACleanup` to clean up the Windows Sockets library. **]**
