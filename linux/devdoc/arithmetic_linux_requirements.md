# arithmetic_win32
================

## Overview

Linux implementation of the `arithmetic` module.

## Exposed API

```c
    MOCKABLE_FUNCTION(, PAL_UINT128, umul64x64, uint64_t, left, uint64_t, right);
```

## file_create

```c
MOCKABLE_FUNCTION(, PAL_UINT128, umul64x64, uint64_t, left, uint64_t, right);
```

`umul64x64` calls `_umul128` from <intrin.h>.

**SRS_ARITHMETIC_02_001: [** `umul64x64` shall cast `left` to `_uint128_t`, multiply it with `right` and return the result. **]**