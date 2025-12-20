# lazy_init requirements

## Overview

`lazy_init` is a module that provides the functionality of `lazy_init`, that is, some code called just once for the lifetime of the module. It is threaded: multiple calls to `lazy_init` will allow just one execution of the code that performs the initialization.

## References


## Exposed API

```c
#define LAZY_INIT_NOT_DONE CALL_ONCE_NOT_CALLED /*to only be used in static initialization, rest of initializations need to use interlocked_exchange*/

#define LAZY_INIT_RESULT_VALUES \
    LAZY_INIT_OK, \
    LAZY_INIT_ERROR

MU_DEFINE_ENUM(LAZY_INIT_RESULT, LAZY_INIT_RESULT_VALUES)

    typedef int (*LAZY_INIT_FUNCTION)(void* params);

    MOCKABLE_FUNCTION(, LAZY_INIT_RESULT, lazy_init, call_once_t*, lazy, LAZY_INIT_FUNCTION, do_init, void*, init_params);

```

### lazy_init
```c
MOCKABLE_FUNCTION(, LAZY_INIT_RESULT, lazy_init, call_once_t*, lazy, LAZY_INIT_FUNCTION, do_init, void*, init_params);
```

`lazy_init` executes `do_init` function with `init_params` only once. 

**SRS_LAZY_INIT_02_001: [** If `lazy` is `NULL` then `lazy_init` shall fail and return `LAZY_INIT_ERROR`. **]**

**SRS_LAZY_INIT_02_002: [** If `do_init` is `NULL` then `lazy_init` shall fail and return `LAZY_INIT_ERROR`. **]**

**SRS_LAZY_INIT_02_003: [** `lazy_init` shall call `call_once_begin(lazy)`. **]**

**SRS_LAZY_INIT_02_004: [** If `call_once_begin` returns `CALL_ONCE_ALREADY_CALLED` then `lazy_init` shall succeed and return `LAZY_INIT_OK`. **]**

**SRS_LAZY_INIT_02_005: [** If `call_once_begin` returns `CALL_ONCE_PROCEED` then `lazy_init` shall call `do_init(init_params)`. **]**

**SRS_LAZY_INIT_02_006: [** If `do_init` returns 0 then `lazy_init` shall call `call_once_end(lazy, true)`, succeed and return `LAZY_INIT_OK`. **]**

**SRS_LAZY_INIT_02_007: [** If `do_init` returns different than 0 then `lazy_init` shall call `call_once_end(lazy, false)`, fail and return `LAZY_INIT_ERROR`. **]**

