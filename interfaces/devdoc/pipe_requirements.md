# pipe
================

## Overview

`pipe` provides platform-independent primitives to work with pipes. Specifically, it supports the POSIX `popen` and `pclose` (as well as the Windows equivalents).

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

**SRS_PIPE_42_001: [** `pipe_popen` shall execute the command `command` and pipe its output to the returned `FILE*`. **]**

**SRS_PIPE_42_002: [** If any error occurs then `pipe_popen` shall fail and return `NULL`. **]**

### pipe_pclose

```c
MOCKABLE_FUNCTION(, int, pipe_pclose, FILE*, stream, int*, exit_code);
```

`pipe_pclose` closes a pipe opened by `pipe_popen`. It returns the exit code of the process in the `exit_code` parameter.

**SRS_PIPE_42_003: [** `pipe_pclose` shall close the pipe `stream`. **]**

**SRS_PIPE_42_004: [** If any error occurs then `pipe_pclose` shall fail and return a non-zero value. **]**

**SRS_PIPE_42_005: [** `pipe_pclose` shall store the result of the executed command in `exit_code`. **]**

**SRS_PIPE_42_006: [** `pipe_pclose` shall succeed and return 0. **]**

