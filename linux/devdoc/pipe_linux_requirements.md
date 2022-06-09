# pipe

## Overview

`pipe` provides platform-independent primitives to work with pipes. The Linux implementation supports POSIX `popen` and `pclose`.

This is a wrapper around [popen](https://www.man7.org/linux/man-pages/man3/popen.3.html) and [pclose](https://man7.org/linux/man-pages/man3/pclose.3.html)

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

**SRS_LINUX_PIPE_42_001: [** `pipe_popen` shall call `popen` with `command` and `"r"` as `type`. **]**

**SRS_LINUX_PIPE_42_002: [** `pipe_popen` shall return the result of `popen`. **]**

### pipe_pclose

```c
MOCKABLE_FUNCTION(, int, pipe_pclose, FILE*, stream, int*, exit_code);
```

`pipe_pclose` closes a pipe opened by `pipe_popen`. It returns the exit code of the process in the `exit_code` parameter.

**SRS_LINUX_PIPE_42_007: [** If `exit_code` is `NULL` then `pipe_pclose` shall fail and return a non-zero value. **]**

**SRS_LINUX_PIPE_42_003: [** `pipe_pclose` shall call `pclose` with `stream`. **]**

**SRS_LINUX_PIPE_42_004: [** `pipe_pclose` shall return a non-zero value if the return value of `pclose` is `-1`. **]**

**SRS_LINUX_PIPE_42_005: [** Otherwise, `pipe_pclose` shall return 0. **]**

**SRS_LINUX_PIPE_42_006: [** `pipe_pclose` shall store the return value of `pclose` bit-shifted right by 8 to get the exit code from the command to `exit_code`. **]**
