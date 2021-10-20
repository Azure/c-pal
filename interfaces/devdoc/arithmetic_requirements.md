# arithmetic
================

## Overview

The `arithmetic` module provides a platform/compiler-independent API for arithemtic operations on 128 bit numbers.

-`umul64x64`: multiplies two uint64_t.

## Exposed API

```c
MOCKABLE_FUNCTION(, uint64_t, umul64x64, uint64_t, left, uint64_t, right, uint64_t* out);
```

## umul64x64

```c
MOCKABLE_FUNCTION(, uint64_t, umul64x64, uint64_t, left, uint64_t, right, uint64_t* high);
```

`umul64x64` multiplies `left` with `right`, outputs in `high` the most significant 64 bits of the multiplication and returns the least significant 64bits of the multiplication. 

**SRS_ARITHMETIC_02_001: [** `umul64x64` shall call `_umul128` and return the result as `PAL_UINT128`. **]**
