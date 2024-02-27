// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdint.h>
#include <stdlib.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "umock_c/umock_c.h"

#include "testrunnerswitcher.h"

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h" // IWYU pragma: keep
#include "c_pal/interlocked.h" // IWYU pragma: keep
#include "c_pal/interlocked_hl.h" // IWYU pragma: keep
#include "c_pal/thandle.h"
#include "c_pal/thandle_ll.h"

struct MOCKED_STRUCT_TAG; // IWYU pragma: private
typedef struct MOCKED_STRUCT_TAG MOCKED_STRUCT; // IWYU pragma: private
THANDLE_TYPE_DECLARE(MOCKED_STRUCT);

#undef ENABLE_MOCKS

#include "umock_c/umock_c_prod.h"

#include "real_gballoc_hl.h"
#include "real_interlocked.h" // IWYU pragma: keep
#include "real_interlocked_hl.h" // IWYU pragma: keep

#include "real_thandle_helper.h"

#include "real_interlocked_renames.h" // IWYU pragma: keep

typedef struct MOCKED_STRUCT_TAG
{
    uint8_t dummy;
} MOCKED_STRUCT;

REAL_THANDLE_DECLARE(MOCKED_STRUCT);

REAL_THANDLE_DEFINE(MOCKED_STRUCT);

#include "real_interlocked_undo_rename.h" // IWYU pragma: keep

static struct G_TAG /*g comes from "global*/
{
    THANDLE(MOCKED_STRUCT) test_mocked_struct;
} g;

static void dispose_MOCKED_STRUCT_do_nothing(REAL_MOCKED_STRUCT* nothing)
{
    (void)nothing;
}

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    umock_c_init(on_umock_c_error);
    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_REAL_THANDLE_MOCK_HOOK(MOCKED_STRUCT);

    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(MOCKED_STRUCT), void*);

    THANDLE(MOCKED_STRUCT) temp = THANDLE_MALLOC(REAL_MOCKED_STRUCT)(dispose_MOCKED_STRUCT_do_nothing);
    ASSERT_IS_NOT_NULL(temp);
    THANDLE_MOVE(REAL_MOCKED_STRUCT)(&g.test_mocked_struct, &temp);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    THANDLE_ASSIGN(REAL_MOCKED_STRUCT)(&g.test_mocked_struct, NULL);

    umock_c_deinit();
    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION(thandle_test_helper_can_register_for_a_real_thandle)
{
    // arrange
    THANDLE(MOCKED_STRUCT) upcounted_MOCKED_STRUCT = NULL;
    // act
    THANDLE_INITIALIZE(MOCKED_STRUCT)(&upcounted_MOCKED_STRUCT, g.test_mocked_struct);
    // assert
    ASSERT_IS_NOT_NULL(upcounted_MOCKED_STRUCT);
    // ablution
    THANDLE_ASSIGN(MOCKED_STRUCT)(&upcounted_MOCKED_STRUCT, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
