# interlocked_hl requirements

## Overview

`interlocked_hl` is a collection of interlocked high level routines.

### Exposed API

```c
#define INTERLOCKED_HL_RESULT_VALUES \
    INTERLOCKED_HL_OK, \
    INTERLOCKED_HL_ERROR, \
    INTERLOCKED_HL_CHANGED

MU_DEFINE_ENUM(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);

typedef bool (*INTERLOCKED_COMPARE_EXCHANGE_IF)(int32_t target, int32_t exchange);
typedef bool (*INTERLOCKED_COMPARE_EXCHANGE_64_IF)(int64_t target, int64_t exchange);

MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_Add64WithCeiling, int64_t volatile_atomic*, Addend, int64_t, Ceiling, int64_t, Value, int64_t*, originalAddend)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_SetAndWake, int32_t volatile_atomic*, address, int32_t, value)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_SetAndWake64, int64_t volatile_atomic*, address, int64_t, value)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_SetAndWakeAll, int32_t volatile_atomic*, address, int32_t, value)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_SetAndWakeAll64, int64_t volatile_atomic*, address, int64_t, value)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_WaitForValue, int32_t volatile_atomic*, address, int32_t, value, uint32_t, milliseconds)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_WaitForValue64, int64_t volatile_atomic*, address, int64_t, value, uint32_t, milliseconds)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_WaitForNotValue, int32_t volatile_atomic*, address, int32_t, value, uint32_t, milliseconds)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_WaitForNotValue64, int64_t volatile_atomic*, address, int64_t, value, uint32_t, milliseconds)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_CompareExchangeIf, int32_t volatile_atomic*, target, int32_t, exchange, INTERLOCKED_COMPARE_EXCHANGE_IF, compare, int32_t*, original_target)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_CompareExchange64If, int64_t volatile_atomic*, target, int64_t, exchange, INTERLOCKED_COMPARE_EXCHANGE_64_IF, compare, int64_t*, original_target)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_DecrementAndWake, int32_t volatile_atomic*, address)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_DecrementAndWake64, int64_t_ volatile_atomic*, address)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
```

###  InterlockedHL_Add64WithCeiling

```c
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_Add64WithCeiling, int64_t volatile_atomic*, Addend, int64_t, Ceiling, int64_t, Value, int64_t*, originalAddend)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
```

`InterlockedHL_Add64WithCeiling` computes the sum of `Addend` and `Value` updates `Addend` with it and writes in `originalAddend` the original value of `Addend`.
If `Addend` + `Value` would result in a undefined behavior or if the result would be greater than `Ceiling`
then `InterlockedHL_Add64WithCeiling` fails and returns `INTERLOCKED_HL_ERROR`.

**SRS_INTERLOCKED_HL_02_001: [** If `Addend` is `NULL` then `InterlockedHL_Add64WithCeiling` shall fail and return `INTERLOCKED_HL_ERROR`. **]**

**SRS_INTERLOCKED_HL_02_006: [** If `originalAddend` is `NULL` then `InterlockedHL_Add64WithCeiling` shall fail and return `INTERLOCKED_HL_ERROR`. **]**

**SRS_INTERLOCKED_HL_02_002: [** If `Addend` + `Value` would underflow then `InterlockedHL_Add64WithCeiling` shall fail and return `INTERLOCKED_HL_ERROR`. **]**

**SRS_INTERLOCKED_HL_02_003: [** If `Addend` + `Value` would overflow then `InterlockedHL_Add64WithCeiling` shall fail and return `INTERLOCKED_HL_ERROR`. **]**

**SRS_INTERLOCKED_HL_02_004: [** If `Addend` + `Value` would be greater than `Ceiling` then `InterlockedHL_Add64WithCeiling` shall fail and return `INTERLOCKED_HL_ERROR`. **]**

**SRS_INTERLOCKED_HL_02_005: [** Otherwise, `InterlockedHL_Add64WithCeiling` shall atomically write in `Addend` the sum of `Addend` and `Value`, succeed and return `INTERLOCKED_HL_OK`. **]**

**SRS_INTERLOCKED_HL_02_007: [** In all failure cases `InterlockedHL_Add64WithCeiling` shall not modify `Addend` or `originalAddend`. **]**

###  InterlockedHL_WaitForValue

```c
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_WaitForValue, int32_t volatile_atomic*, address_to_check, int32_t, value_to_wait, uint32_t, timeout_ms)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
```

`InterlockedHL_WaitForValue` waits for the value at the given `address_to_check` to be equal to the target `value_to_wait`.

**SRS_INTERLOCKED_HL_01_002: [** If `address_to_check` is `NULL`, `InterlockedHL_WaitForValue` shall fail and return `INTERLOCKED_HL_ERROR`. **]**

**SRS_INTERLOCKED_HL_01_003: [** If the value at `address_to_check` is equal to `value_to_wait`, `InterlockedHL_WaitForValue` shall return `INTERLOCKED_HL_OK`. **]**

**SRS_INTERLOCKED_HL_01_004: [** If the value at `address_to_check` is not equal to `value_to_wait`, `InterlockedHL_WaitForValue` shall wait until the value at `address_to_check` changes using `wait_on_address`. **]**

**SRS_INTERLOCKED_HL_01_007: [** When `wait_on_address` succeeds, `InterlockedHL_WaitForValue` shall again compare the value at `address_to_check` with `value_to_wait`. **]**

**SRS_INTERLOCKED_HL_11_001: [** If `wait_on_address` hits the timeout specified in `timeout_ms`, `InterlockedHL_WaitForValue` shall fail and return `INTERLOCKED_HL_TIMEOUT`. **]**

**SRS_INTERLOCKED_HL_01_006: [** If `wait_on_address` fails, `InterlockedHL_WaitForValue` shall fail and return `INTERLOCKED_HL_ERROR`. **]**

###  InterlockedHL_WaitForValue64

```c
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_WaitForValue64, int64_t volatile_atomic*, address_to_check, int64_t, value_to_wait, uint32_t, timeout_ms)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
```

`InterlockedHL_WaitForValue64` waits for the value at the given `address_to_check` to be equal to the target `value_to_wait`.

**SRS_INTERLOCKED_HL_05_001: [** If `address_to_check` is `NULL`, `InterlockedHL_WaitForValue64` shall fail and return `INTERLOCKED_HL_ERROR`. **]**

**SRS_INTERLOCKED_HL_05_002: [** If the value at `address_to_check` is equal to `value_to_wait`, `InterlockedHL_WaitForValue64` shall return `INTERLOCKED_HL_OK`. **]**

**SRS_INTERLOCKED_HL_05_003: [** If the value at `address_to_check` is not equal to `value_to_wait`, `InterlockedHL_WaitForValue64` shall wait until the value at `address_to_check` changes using `wait_on_address_64`. **]**

**SRS_INTERLOCKED_HL_05_004: [** When `wait_on_address_64` succeeds, `InterlockedHL_WaitForValue64` shall again compare the value at `address_to_check` with `value_to_wait`. **]**

**SRS_INTERLOCKED_HL_05_005: [** If `wait_on_address_64` hits the timeout specified in timeout_ms, `InterlockedHL_WaitForValue64` shall fail and return `INTERLOCKED_HL_TIMEOUT`. **]**

**SRS_INTERLOCKED_HL_05_006: [** If `wait_on_address_64` fails, `InterlockedHL_WaitForValue64` shall fail and return `INTERLOCKED_HL_ERROR`. **]**


### InterlockedHL_WaitForNotValue

```c
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_WaitForNotValue, int32_t volatile_atomic*, address_to_check, int32_t, value_to_wait, uint32_t, timeout_ms)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
```

`InterlockedHL_WaitForNotValue` waits for the value at the given `address_to_check` to be not equal to the target `value_to_wait`.

**SRS_INTERLOCKED_HL_42_001: [** If `address_to_check` is `NULL`, `InterlockedHL_WaitForNotValue` shall fail and return `INTERLOCKED_HL_ERROR`. **]**

**SRS_INTERLOCKED_HL_42_002: [** If the value at `address_to_check` is not equal to `value_to_wait`, `InterlockedHL_WaitForNotValue` shall return `INTERLOCKED_HL_OK`. **]**

**SRS_INTERLOCKED_HL_42_003: [** If the value at `address_to_check` is equal to `value_to_wait`, `InterlockedHL_WaitForNotValue` shall wait until the value at `address_to_check` changes by using `wait_on_address`. **]**

**SRS_INTERLOCKED_HL_42_005: [** When `wait_on_address` succeeds, `InterlockedHL_WaitForNotValue64` shall again compare the value at `address_to_check` with `value_to_wait`. **]**

**SRS_INTERLOCKED_HL_11_002: [** If `wait_on_address` hits the timeout specified in timeout_ms, `InterlockedHL_WaitForNotValue` shall fail and return `INTERLOCKED_HL_TIMEOUT`. **]**

**SRS_INTERLOCKED_HL_42_007: [** If `wait_on_address` fails, `InterlockedHL_WaitForNotValue` shall fail and return `INTERLOCKED_HL_ERROR`. **]**

###  InterlockedHL_WaitForNotValue64

```c
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_WaitForNotValue64, int64_t volatile_atomic*, address_to_check, int64_t, value_to_wait, uint32_t, timeout_ms)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
```

`InterlockedHL_WaitForValue64` waits for the value at the given `address_to_check` to be not equal to the target `value_to_wait`.

**SRS_INTERLOCKED_HL_05_007: [** If `address_to_check` is `NULL`, `InterlockedHL_WaitForNotValue64` shall fail and return `INTERLOCKED_HL_ERROR`. **]**

**SRS_INTERLOCKED_HL_05_008: [** If the value at `address_to_check` is not equal to `value_to_wait`, `InterlockedHL_WaitForNotValue64` shall return `INTERLOCKED_HL_OK`. **]**

**SRS_INTERLOCKED_HL_05_009: [** If the value at `address_to_check` is equal to `value_to_wait`, `InterlockedHL_WaitForNotValue64` shall wait until the value at `address_to_check` changes using `wait_on_address_64`. **]**

**SRS_INTERLOCKED_HL_05_010: [** When `wait_on_address_64` succeeds, `InterlockedHL_WaitForNotValue64` shall again compare the value at `address_to_check` with `value_to_wait`. **]**

**SRS_INTERLOCKED_HL_05_011: [** If `wait_on_address_64` hits the timeout specified in timeout_ms, `InterlockedHL_WaitForNotValue64` shall fail and return `INTERLOCKED_HL_TIMEOUT`. **]**

**SRS_INTERLOCKED_HL_05_012: [** If `wait_on_address_64` fails, `InterlockedHL_WaitForNotValue64` shall fail and return `INTERLOCKED_HL_ERROR`. **]**


### InterlockedHL_CompareExchangeIf
```c
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_CompareExchangeIf, int32_t volatile_atomic*, target, int32_t, exchange, INTERLOCKED_COMPARE_EXCHANGE_32_IF, compare, int32_t*, original_target)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
```

`InterlockedHL_CompareExchangeIf` attempts to change `target` to `exchange` if `compare` that takes the value of `target` and `exchange` evaluates to `true`. `InterlockedHL_CompareExchangeIf` will write `original_target` with the initial value of `target`.
If `target` changes while the function executes then `InterlockedHL_CompareExchangeIf` returns `INTERLOCKED_HL_CHANGED` and the calling code can decide to retry/timeout/exponential backoff (for example).

**SRS_INTERLOCKED_HL_01_009: [** If `target` is `NULL` then `InterlockedHL_CompareExchangeIf` shall return fail and return `INTERLOCKED_HL_ERROR`. **]**

**SRS_INTERLOCKED_HL_01_010: [** If `compare` is `NULL` then `InterlockedHL_CompareExchangeIf` shall return fail and return `INTERLOCKED_HL_ERROR`. **]**

**SRS_INTERLOCKED_HL_01_011: [** If `original_target` is `NULL` then `InterlockedHL_CompareExchangeIf` shall return fail and return `INTERLOCKED_HL_ERROR`. **]**

**SRS_INTERLOCKED_HL_01_012: [** `InterlockedHL_CompareExchangeIf` shall acquire the initial value of `target`. **]**

**SRS_INTERLOCKED_HL_01_013: [** If `compare`(`target`, `exchange`) returns `true` then `InterlockedHL_CompareExchangeIf` shall exchange `target` with `exchange`. **]**

**SRS_INTERLOCKED_HL_01_014: [** If `target` changed meanwhile then `InterlockedHL_CompareExchangeIf` shall return `INTERLOCKED_HL_CHANGED` and shall not peform any exchange of values. **]**

**SRS_INTERLOCKED_HL_01_015: [** If `target` did not change meanwhile then `InterlockedHL_CompareExchangeIf` shall return `INTERLOCKED_HL_OK` and shall peform the exchange of values. **]**

**SRS_INTERLOCKED_HL_01_017: [** If `compare` returns `false` then  `InterlockedHL_CompareExchangeIf` shall not perform any exchanges and return `INTERLOCKED_HL_OK`. **]**

**SRS_INTERLOCKED_HL_01_016: [** `original_target` shall be set to the original value of `target`. **]**

### InterlockedHL_CompareExchange64If
```c
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_CompareExchange64If, int64_t volatile_atomic*, target, int64_t, exchange, INTERLOCKED_COMPARE_EXCHANGE_64_IF, compare, int64_t*, original_target)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
```

`InterlockedHL_CompareExchange64If` attempts to change `target` to `exchange` if `compare` that takes the value of `target` and `exchange` evaluates to `true`. `InterlockedHL_CompareExchange64If` will write `original_target` with the initial value of `target`.
If `target` changes while the function executes then `InterlockedHL_CompareExchange64If` returns `INTERLOCKED_HL_CHANGED` and the calling code can decide to retry/timeout/exponential backoff (for example).

**SRS_INTERLOCKED_HL_02_008: [** If `target` is `NULL` then `InterlockedHL_CompareExchange64If` shall return fail and return `INTERLOCKED_HL_ERROR`. **]**

**SRS_INTERLOCKED_HL_02_009: [** If `compare` is `NULL` then `InterlockedHL_CompareExchange64If` shall return fail and return `INTERLOCKED_HL_ERROR`. **]**

**SRS_INTERLOCKED_HL_02_010: [** If `original_target` is `NULL` then `InterlockedHL_CompareExchange64If` shall return fail and return `INTERLOCKED_HL_ERROR`. **]**

**SRS_INTERLOCKED_HL_02_011: [** `InterlockedHL_CompareExchange64If` shall acquire the initial value of `target`. **]**

**SRS_INTERLOCKED_HL_02_012: [** If `compare`(`target`, `exchange`) returns `true` then `InterlockedHL_CompareExchange64If` shall exchange `target` with `exchange`. **]**

**SRS_INTERLOCKED_HL_02_013: [** If `target` changed meanwhile then `InterlockedHL_CompareExchange64If` shall return `INTERLOCKED_HL_CHANGED` and shall not peform any exchange of values. **]**

**SRS_INTERLOCKED_HL_02_014: [** If `target` did not change meanwhile then `InterlockedHL_CompareExchange64If` shall return `INTERLOCKED_HL_OK` and shall peform the exchange of values. **]**

**SRS_INTERLOCKED_HL_02_015: [** If `compare` returns `false` then  `InterlockedHL_CompareExchange64If` shall not perform any exchanges and return `INTERLOCKED_HL_OK`. **]**

**SRS_INTERLOCKED_HL_02_016: [** `original_target` shall be set to the original value of `target`. **]**

### InterlockedHL_SetAndWake
```c
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_SetAndWake, int32_t volatile_atomic*, address, int32_t, value)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
```

`InterlockedHL_SetAndWake` set the value at `address` to `value` and signals the change of value in `address`. This can be commonly used with `InterlockedHL_WaitForValue` to signal a waiting thread.

**SRS_INTERLOCKED_HL_02_020: [** If `address` is `NULL` then `InterlockedHL_SetAndWake` shall fail and return `INTERLOCKED_HL_ERROR`. **]**

**SRS_INTERLOCKED_HL_02_017: [** `InterlockedHL_SetAndWake` shall set `address` to `value`. **]**

**SRS_INTERLOCKED_HL_02_018: [** `InterlockedHL_SetAndWake` shall call `wake_by_address_single`. **]**

**SRS_INTERLOCKED_HL_02_019: [** `InterlockedHL_SetAndWake` shall succeed and return `INTERLOCKED_HL_OK`. **]**

### InterlockedHL_SetAndWake64
```c
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_SetAndWake64, int64_t volatile_atomic*, address, int64_t, value)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
```

`InterlockedHL_SetAndWake64` set the value at `address` to `value` and signals the change of value in `address`. This can be commonly used with `InterlockedHL_WaitForValue64` to signal a waiting thread.

**SRS_INTERLOCKED_HL_05_013: [** If `address` is `NULL` then `InterlockedHL_SetAndWake64` shall fail and return `INTERLOCKED_HL_ERROR`. **]**

**SRS_INTERLOCKED_HL_05_014: [** `InterlockedHL_SetAndWake64` shall set `value` at `address`. **]**

**SRS_INTERLOCKED_HL_05_015: [** `InterlockedHL_SetAndWake64` shall wake a single thread listening on `address`. **]**

**SRS_INTERLOCKED_HL_05_016: [** `InterlockedHL_SetAndWake64` shall succeed and return `INTERLOCKED_HL_OK`. **]**

### InterlockedHL_SetAndWakeAll
```c
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_SetAndWakeAll, int32_t volatile_atomic*, address, int32_t, value)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
```

`InterlockedHL_SetAndWakeAll` set the value at `address` to `value` and signals the change of value in `address` to all waiting threads. This can be commonly used with `InterlockedHL_WaitForValue` to signal a waiting thread.

**SRS_INTERLOCKED_HL_02_028: [** If `address` is `NULL` then `InterlockedHL_SetAndWakeAll` shall fail and return `INTERLOCKED_HL_ERROR`. **]**

**SRS_INTERLOCKED_HL_02_029: [** `InterlockedHL_SetAndWakeAll` shall set `address` to `value`. **]**

**SRS_INTERLOCKED_HL_02_030: [** `InterlockedHL_SetAndWakeAll` shall call `wake_by_address_all`. **]**

**SRS_INTERLOCKED_HL_02_031: [** `InterlockedHL_SetAndWakeAll` shall succeed and return `INTERLOCKED_HL_OK`. **]**

### InterlockedHL_SetAndWakeAll64
```c
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_SetAndWakeAll64, int64_t volatile_atomic*, address, int64_t, value)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
```

`InterlockedHL_SetAndWakeAll64` set the value at `address` to `value` and signals the change of value in `address` to all waiting threads. This can be commonly used with `InterlockedHL_WaitForValue64` to signal a waiting thread.

**SRS_INTERLOCKED_HL_05_017: [** If `address` is `NULL` then `InterlockedHL_SetAndWakeAll64` shall fail and return `INTERLOCKED_HL_ERROR`. **]**

**SRS_INTERLOCKED_HL_05_018: [** `InterlockedHL_SetAndWakeAll64` shall set `value` at `address`. **]**

**SRS_INTERLOCKED_HL_05_019: [** `InterlockedHL_SetAndWakeAll64` shall wake all threads listening on `address`. **]**

**SRS_INTERLOCKED_HL_05_020: [** `InterlockedHL_SetAndWakeAll64` shall succeed and return `INTERLOCKED_HL_OK`. **]**

### InterlockedHL_DecrementAndWake
```c
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_DecrementAndWake, int32_t volatile_atomic*, address)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
```

`InterlockedHL_DecrementAndWake` decrements the value at `address` by 1 and signals the change of value in `address`. This can be commonly used with `InterlockedHL_WaitForValue` to signal a waiting thread.

**SRS_INTERLOCKED_HL_44_001: [** If `address` is `NULL` then `InterlockedHL_DecrementAndWake` shall fail and return `INTERLOCKED_HL_ERROR`. **]**

**SRS_INTERLOCKED_HL_44_002: [** `InterlockedHL_DecrementAndWake` shall decrement the value at `address` by 1. **]**

**SRS_INTERLOCKED_HL_44_003: [** `InterlockedHL_DecrementAndWake` shall call `wake_by_address_single`. **]**

**SRS_INTERLOCKED_HL_44_004: [** `InterlockedHL_DecrementAndWake` shall succeed and return `INTERLOCKED_HL_OK`. **]**

### InterlockedHL_DecrementAndWake64
```c
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_DecrementAndWake64, int64_t_ volatile_atomic*, address)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
```

`InterlockedHL_DecrementAndWake64` decrements the value at `address` by 1 and signals the change of value in `address`. This can be commonly used with `InterlockedHL_WaitForValue64` to signal a waiting thread.

**SRS_INTERLOCKED_HL_05_021: [** If `address` is `NULL` then `InterlockedHL_DecrementAndWake64` shall fail and return `INTERLOCKED_HL_ERROR`. **]**

**SRS_INTERLOCKED_HL_05_022: [** `InterlockedHL_DecrementAndWake64` shall decrement the `value` at `address` by 1. **]**

**SRS_INTERLOCKED_HL_05_023: [** `InterlockedHL_DecrementAndWake64` shall wake a single thread listening on `address`. **]**

**SRS_INTERLOCKED_HL_05_024: [** `InterlockedHL_DecrementAndWake64` shall succeed and return `INTERLOCKED_HL_OK`. **]**