# real_unique_thandle_helper.h Requirements

## Overview

`real_unique_thandle_helper.h` is a helper for mocking opaque `UNIQUE_THANDLE` types in unit tests. It is the `UNIQUE_THANDLE` counterpart of `real_thandle_helper.h`.

It defines the `UNIQUE_THANDLE` functions for a stand-in type and hooks them behind the mocked `UNIQUE_THANDLE_MOVE`, `UNIQUE_THANDLE_INITIALIZE_MOVE` and `UNIQUE_THANDLE_DISPOSE` of the original type. This provides a real implementation of moving and memory management for the original type, but not the behavior of the type itself.

## How to use:

See the example in `common\tests\real_unique_thandle_helper_ut\real_unique_thandle_helper_ut.c`.

1. In the unit test, enable mocks and include `gballoc_hl.h`, `gballoc_hl_redirect.h`, `interlocked.h`, `interlocked_hl.h` and `unique_thandle.h`, along with the header of the type you want to mock.

```c
#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/unique_thandle.h"

#include "mocked_struct.h" // A file that declares an incomplete type MOCKED_STRUCT and UNIQUE_THANDLE_TYPE_DECLARE(MOCKED_STRUCT)

#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS
```

2. After disabling mocks, include `real_gballoc_hl.h`, `real_interlocked.h`, `real_interlocked_hl.h` and `real_unique_thandle_helper.h`.

```c
#include "real_gballoc_hl.h"
#include "real_interlocked.h"
#include "real_interlocked_hl.h"

#include "real_unique_thandle_helper.h"
```

3. Include `real_interlocked_renames.h` to temporarily use the real interlocked functions instead of the mocked ones. The `THANDLE` functions under `UNIQUE_THANDLE` use interlocked functions, so this avoids setting interlocked expectations for moves and disposes.

```c
#include "real_interlocked_renames.h" // IWYU pragma: keep
```

4. Define a stand-in type to use in the tests, then call `REAL_UNIQUE_THANDLE_DECLARE` and `REAL_UNIQUE_THANDLE_DEFINE` to define the `UNIQUE_THANDLE` functions for it.

```c
typedef struct MOCKED_STRUCT_TAG
{
    uint8_t dummy;
} MOCKED_STRUCT;

REAL_UNIQUE_THANDLE_DECLARE(MOCKED_STRUCT);

REAL_UNIQUE_THANDLE_DEFINE(MOCKED_STRUCT);
```

5. Include `real_interlocked_undo_rename.h` to go back to the mocked interlocked functions.

```c
#include "real_interlocked_undo_rename.h" // IWYU pragma: keep
```

6. In the test suite initialization, call `REGISTER_REAL_UNIQUE_THANDLE_MOCK_HOOK` to hook the real implementation behind the mocked `UNIQUE_THANDLE` functions of the type.

```c
    REGISTER_REAL_UNIQUE_THANDLE_MOCK_HOOK(MOCKED_STRUCT);
```

Now, calls in the code under test to `UNIQUE_THANDLE_MOVE(MOCKED_STRUCT)`, `UNIQUE_THANDLE_INITIALIZE_MOVE(MOCKED_STRUCT)` and `UNIQUE_THANDLE_DISPOSE(MOCKED_STRUCT)` are recorded by the mocks and run the real functions of `REAL_MOCKED_STRUCT`.

Tests create instances with `UNIQUE_THANDLE_MALLOC(REAL_MOCKED_STRUCT)`. The mocked `UNIQUE_THANDLE` functions only take `UNIQUE_THANDLE(MOCKED_STRUCT)*` arguments, which `umock_c` treats as `void*`. If other mocked functions take or return `UNIQUE_THANDLE(MOCKED_STRUCT)` by value, for example a mocked create function, register it with `REGISTER_UMOCK_ALIAS_TYPE(UNIQUE_THANDLE(MOCKED_STRUCT), void*)`.

## Exposed API

```c
#define REAL_UNIQUE_THANDLE_DECLARE(TYPE)
#define REAL_UNIQUE_THANDLE_DEFINE(TYPE)
#define REGISTER_REAL_UNIQUE_THANDLE_MOCK_HOOK(TYPE)
```

### REAL_UNIQUE_THANDLE_DECLARE(TYPE)

```c
#define REAL_UNIQUE_THANDLE_DECLARE(TYPE)
```

Declares a new `REAL_TYPE` type for the structure `TYPE` and the `UNIQUE_THANDLE` functions for it.

### REAL_UNIQUE_THANDLE_DEFINE(TYPE)

```c
#define REAL_UNIQUE_THANDLE_DEFINE(TYPE)
```

Defines the `UNIQUE_THANDLE` functions for `REAL_TYPE`, using the "real" gballoc functions.

### REGISTER_REAL_UNIQUE_THANDLE_MOCK_HOOK(TYPE)

```c
#define REGISTER_REAL_UNIQUE_THANDLE_MOCK_HOOK(TYPE)
```

Hooks the mocked `UNIQUE_THANDLE_MOVE(TYPE)`, `UNIQUE_THANDLE_INITIALIZE_MOVE(TYPE)` and `UNIQUE_THANDLE_DISPOSE(TYPE)` to the real functions of `REAL_TYPE`.
