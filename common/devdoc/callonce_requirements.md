# call_once requirements

## Overview

call_once is a module that provides a pair of functions callonce_begin/callonce_end to be used to have some code executed just once. 

Typical usage is:

```c

call_once_t wasInit = CALL_ONCE_NOT_CALLED;

...

void initialize(...)
{
    /*one time attempt*/
    if(call_once_begin(&wasInit) == CALL_ONCE_PROCEED)
    {
        /*here is user code called from 1 thread, this one*/
        call_once_end(&wasInit, true); /*or call_once_end(&wasInit, false)*/
    }
}

or

void initialize(...)
{
    /*with retries.. */
    while(call_once_begin(&wasInit) == CALL_ONCE_PROCEED)
    {
        /*here is user code called from 1 thread, this one*/
        call_once_end(&wasInit, true); /*or call_once_end(&wasInit, false)*/
    }
}
```


## References


## Exposed API

```c
#define CALL_ONCE_NOT_CALLED 0 /*to only be used in static initialization, rest of initializations need to use interlocked_exchange*/

#define CALL_ONCE_RESULT_VALUES \
    CALL_ONCE_PROCEED, \
    CALL_ONCE_ALREADY_CALLED \

    MOCKABLE_FUNCTION(, CALL_ONCE_RESULT, call_once_begin, call_once_t*, state);
    MOCKABLE_FUNCTION(, void, call_once_end, call_once_t*, state, bool, success);
```

### call_once_begin, call_once_t*, state);
```c
MOCKABLE_FUNCTION(, CALL_ONCE_RESULT, call_once_begin, call_once_t*, state);
```

`call_once_begin` returns `CALL_ONCE_PROCEED` when the user is allowed to call the "execute once" code.


**SRS_CALL_ONCE_02_001: [** `call_once_begin` shall use `interlocked_compare_exchange(state, 1, 0)` to determine if user has alredy indicated that the init code was executed with success. **]**

**SRS_CALL_ONCE_02_002: [** If `interlocked_compare_exchange` returns `2` then `call_once_begin` shall return  `CALL_ONCE_ALREADY_CALLED`. **]**

**SRS_CALL_ONCE_02_003: [** If `interlocked_compare_exchange` returns `1` then `call_once_begin` shall call `wait_on_address(state)` with timeout `UINT32_MAX` and call again `interlocked_compare_exchange(state, 1, 0)`. **]**

**SRS_CALL_ONCE_02_004: [** If `interlocked_compare_exchange` returns `0` then `call_once_begin` shall return `CALL_ONCE_PROCEED`. **]**


### call_once_end
```c
MOCKABLE_FUNCTION(, void, call_once_end, call_once_t*, state, bool, success);
```

`call_once_end` is called by the user to signal a succesful or failure of the call once code. If `success` is `false` then `state` is reset to its initialized state thus allowing another attempt. If `success` is `true` then all ongoing and all further calls to `call_once_begin` return `CALL_ONCE_ALREADY_CALLED`.


**SRS_CALL_ONCE_02_005: [** If `success` is `true` then `call_once_end` shall call `interlocked_exchange` setting `state` to `CALL_ONCE_CALLED` and shall call `wake_by_address_all(state)`. **]**

**SRS_CALL_ONCE_02_006: [** If `success` is `false` then `call_once_end` shall call `interlocked_exchange` setting `state` to `CALL_ONCE_NOT_CALLED` and shall call `wake_by_address_all(state)`. **]**




