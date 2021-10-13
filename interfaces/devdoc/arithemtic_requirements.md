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

    MOCKABLE_FUNCTION(, PAL_UINT128, umul64x64, uint64_t, left, uint64_t, right);

```

## umul64x64

```c
MOCKABLE_FUNCTION(, PAL_UINT128, umul64x64, uint64_t, left, uint64_t, right);
```

`umul64x64` multiplies `left` with `right`, outputs in `high` field of return the most significant 64 bits of the multiplication and in `low` field of the return the least 64 significant bits of the multiplication.

