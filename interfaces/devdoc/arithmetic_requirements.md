# arithmetic
================

## Overview

The `arithmetic` module provides a platform/compiler-independent API for arithemtic operations on 128 bit numbers.

-`umul64x64`: multiplies two uint64_t.

## Exposed API

```c
typedef struct PAL_UINT128_TAG
{
    uint64_t high;
    uint64_t low;
}PAL_UINT128;

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C"
{
#endif

    MOCKABLE_FUNCTION(, PAL_UINT128, umul64x64, uint64_t, left, uint64_t, right);

#ifdef __cplusplus
}
#endif
```

## umul64x64

```c
MOCKABLE_FUNCTION(, PAL_UINT128, umul64x64, uint64_t, left, uint64_t, right);
```

`umul64x64` multiplies `left` with `right` and outputs a `PAL_UINT128` that contains the high and low parts of the resulting 128 bit value. 

**SRS_ARITHMETIC_02_001: [** `umul64x64` shall mutiply `left` and `right` and returna `PAL_UINT128` as result. **]**
