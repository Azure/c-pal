# real_thandle_helper.h Requirements

## Overview

`real_thandle_helper.h` is a helper to make the pattern of mocking opaque THANDLE types in Unit Tests easier. It provides a set of macros to define THANDLE functions for a dummy type that can be hooked into the mocked THANDLE calls. This provide a real implementation of the memory management for the original THANDLE type, but not the behavior of the type itself.

## How to use:

See example in `common\tests\real_thandle_helper_ut\real_thandle_helper_ut.c`

1. In the Unit Test, set `ENABLE_MOCKS`, make sure to include `gballoc_hl.h`, `gballoc_hl_redirect.h`, interlocked.H`, `interlocked_hl.h`, and `thandle.h`, along with the type you want to mock.

```c
#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/thandle.h"

#include "mocked_struct.h" // A file that defines an incomplete type of `MOCKED_STRUCT` and `THANDLE_TYPE_DECLARE(MOCKED_STRUCT)`
```

2. Unset `ENABLE_MOCKS` then include `umock_c_prod.h`, `real_gballoc_hl.h`, `real_interlocked.h`, `real_interlocked_hl.h`, and `real_thandle_helper.h`

```c
#undef ENABLE_MOCKS

#include "umock_c/umock_c_prod.h"

#include "real_gballoc_hl.h"
#include "real_interlocked.h"
#include "real_interlocked_hl.h"

#include "real_thandle_helper.h"
```

3. Define a mocked type to use in the tests, then call `REAL_THANDLE_DECLARE` and `REAL_THANDLE_TYPE_DEFINE` to define the THANDLE functions for the mocked type.

```c
typedef struct MOCKED_STRUCT_TAG
{
    uint8_t dummy;
} MOCKED_STRUCT;

REAL_THANDLE_DECLARE(MOCKED_STRUCT);

REAL_THANDLE_DEFINE(MOCKED_STRUCT);
```

4. In The test suite initialization, call `REGISTER_REAL_THANDLE_MOCK_HOOK` to register the mocked type.

```c
    REGISTER_REAL_THANDLE_MOCK_HOOK(MOCKED_STRUCT);
```

Now, calls in the code to `THANDLE` functions will be redirected to the mocked type.

## Exposed API

```c
#define REAL_THANDLE_DECLARE(TYPE)
#define REAL_THANDLE_DEFINE(TYPE)
#define REGISTER_REAL_THANDLE_MOCK_HOOK(TYPE)
```

### REAL_THANDLE_DECLARE(TYPE)

```c
#define REAL_THANDLE_DECLARE(TYPE)
```

Declares a new `REAL_TYPE` incomplete type for the structure `TYPE` and the THANDLE functions for it.

### REAL_THANDLE_DEFINE(TYPE)

```c
#define REAL_THANDLE_DEFINE(TYPE)
```

Defines the `THANDLE` functions for the `REAL_TYPE` of the structure `TYPE` using the "real" gballoc functions.

### REGISTER_REAL_THANDLE_MOCK_HOOK(TYPE)

```c
#define REGISTER_REAL_THANDLE_MOCK_HOOK(TYPE)
```

Registers the mocked hooks from `TYPE` to `to be used in the tests.