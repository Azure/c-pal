# pipe
================

## Overview

`pipe` provides platform-independent primitives to work with pipes. The Win32 implementation supports `_popen` and `_pclose`.

## Exposed API

```c
MOCKABLE_FUNCTION(, FILE*, pipe_popen, const char*, command);

MOCKABLE_FUNCTION(, int, pipe_pclose, FILE*, stream, int*, exit_code);
```

### pipe_popen

```c
MOCKABLE_FUNCTION(, FILE*, pipe_popen, const char*, command);
```

`pipe_popen` creates a pipe and executes a command. The pipe only supports reading.

**SRS_WIN32_PIPE_42_001: [** `pipe_popen` shall call `_popen` with `command` and `"rt"` as `type`. **]**

**SRS_WIN32_PIPE_42_002: [** `pipe_popen` shall return the result of `_popen`. **]**

### pipe_pclose

```c
MOCKABLE_FUNCTION(, int, pipe_pclose, FILE*, stream, int*, exit_code);
```

`pipe_pclose` closes a pipe opened by `pipe_popen`. It returns the exit code of the process in the `exit_code` parameter.

**SRS_WIN32_PIPE_42_007: [** If `exit_code` is `NULL` then `pipe_pclose` shall fail and return a non-zero value. **]**

**SRS_WIN32_PIPE_42_003: [** `pipe_pclose` shall call `_pclose` with `stream`. **]**

**SRS_WIN32_PIPE_42_004: [** `pipe_pclose` shall return a non-zero value if the return value of `_pclose` is `-1`. **]**

**SRS_WIN32_PIPE_42_005: [** Otherwise, `pipe_pclose` shall return 0. **]**

**SRS_WIN32_PIPE_42_006: [** `pipe_pclose` shall store the return value of `_pclose` in `exit_code`. **]**
