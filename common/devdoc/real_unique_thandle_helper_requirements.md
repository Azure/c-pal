# real_unique_thandle_helper.h Requirements

## Overview

`real_unique_thandle_helper.h` is a helper for mocking opaque `UNIQUE_THANDLE` types in unit tests.

It defines the `UNIQUE_THANDLE` functions for a stand-in type and hooks them behind the mocked `UNIQUE_THANDLE_INITIALIZE_MOVE` and `UNIQUE_THANDLE_DISPOSE` of the original type. This provides a real implementation of moving and memory management for the original type, but not the behavior of the type itself.

It also registers `UNIQUE_THANDLE(TYPE)` and `const UNIQUE_THANDLE(TYPE)` with `umock_c`, which every unit test that mocks a function with a handle parameter needs.

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

3. Include `real_interlocked_renames.h` to temporarily use the real interlocked functions instead of the mocked ones. The `UNIQUE_THANDLE` functions use interlocked functions, so this avoids setting interlocked expectations when tests create and dispose handles.

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

6. In the test suite initialization, call `REGISTER_UNIQUE_THANDLE_UMOCK_ALIAS_TYPES` to register the handle types with `umock_c`, and `REGISTER_REAL_UNIQUE_THANDLE_MOCK_HOOK` to hook the real implementation behind the mocked `UNIQUE_THANDLE` functions of the type.

```c
    REGISTER_UNIQUE_THANDLE_UMOCK_ALIAS_TYPES(MOCKED_STRUCT);
    REGISTER_REAL_UNIQUE_THANDLE_MOCK_HOOK(MOCKED_STRUCT);
```

Now, calls in the code under test to `UNIQUE_THANDLE_INITIALIZE_MOVE(MOCKED_STRUCT)` and `UNIQUE_THANDLE_DISPOSE(MOCKED_STRUCT)` are recorded by the mocks and run the real functions of `REAL_MOCKED_STRUCT`.

Tests create objects with `UNIQUE_THANDLE_MALLOC(REAL_MOCKED_STRUCT)` and put them in handles with `UNIQUE_THANDLE_ADOPT(REAL_MOCKED_STRUCT)`. A handle of a test is a `UNIQUE_THANDLE(MOCKED_STRUCT)`.

## Expectations on handle parameters

A mocked function stores the address of the handle it receives. Write expectations as follows:

- To expect a call on a given handle, pass the handle itself, as in `STRICT_EXPECTED_CALL(UNIQUE_THANDLE_DISPOSE(MOCKED_STRUCT)(handle))`. The call matches when the code under test passes the same handle, and a call on another handle is a mismatch.
- To expect any handle, pass `IGNORED_ARG`.
- To check the object that a handle holds when the call happens, copy the object pointer and compare it with `ValidateArgumentBuffer`, as in `.ValidateArgumentBuffer(1, &object, sizeof(object))`, where `object` is the `MOCKED_STRUCT*` that the handle holds.
- To have a mocked function fill a handle, for example a mocked create function, use `CopyOutArgumentBuffer` with the object pointer, as in `.CopyOutArgumentBuffer_handle(&object, sizeof(object))`.
- Use the tools above for handle parameters. `ValidateArgumentValue` and `CaptureArgumentValue` work on the address of the handle.

## Exposed API

```c
#define REAL_UNIQUE_THANDLE_DECLARE(TYPE)
#define REAL_UNIQUE_THANDLE_DEFINE(TYPE)
#define REGISTER_REAL_UNIQUE_THANDLE_MOCK_HOOK(TYPE)
#define REGISTER_UNIQUE_THANDLE_UMOCK_ALIAS_TYPES(TYPE)
```

### REAL_UNIQUE_THANDLE_DECLARE(TYPE)

```c
#define REAL_UNIQUE_THANDLE_DECLARE(TYPE)
```

Declares a new `REAL_TYPE` type for the structure `TYPE` and the `UNIQUE_THANDLE` functions for it. The functions use `UNIQUE_THANDLE(TYPE)`, which `UNIQUE_THANDLE_TYPE_DECLARE(TYPE)` introduced.

### REAL_UNIQUE_THANDLE_DEFINE(TYPE)

```c
#define REAL_UNIQUE_THANDLE_DEFINE(TYPE)
```

Defines the `UNIQUE_THANDLE` functions for `REAL_TYPE`, using the "real" gballoc functions.

### REGISTER_REAL_UNIQUE_THANDLE_MOCK_HOOK(TYPE)

```c
#define REGISTER_REAL_UNIQUE_THANDLE_MOCK_HOOK(TYPE)
```

Hooks the mocked `UNIQUE_THANDLE_INITIALIZE_MOVE(TYPE)` and `UNIQUE_THANDLE_DISPOSE(TYPE)` to the real functions of `REAL_TYPE`.

### REGISTER_UNIQUE_THANDLE_UMOCK_ALIAS_TYPES(TYPE)

```c
#define REGISTER_UNIQUE_THANDLE_UMOCK_ALIAS_TYPES(TYPE)
```

Registers `UNIQUE_THANDLE(TYPE)` and `const UNIQUE_THANDLE(TYPE)` with `umock_c` as aliases of `void*`.
