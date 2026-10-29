// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "unique_thandle_ut_pch.h"

/*a type whose UNIQUE_THANDLE functions are declared without mocks and named after a name other than the type*/
typedef struct LL_STRUCT_TAG LL_STRUCT;
UNIQUE_THANDLE_LL_TYPE_DECLARE(LL_NAME, LL_STRUCT);

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
}

TEST_FUNCTION(UNIQUE_THANDLE_TYPE_DECLARE_declares_mockable_initialize_move_and_dispose) // no-srs
{
    // arrange
    UNIQUE_THANDLE(MOCKED_STRUCT) t1 = NULL;
    UNIQUE_THANDLE(MOCKED_STRUCT) t2 = NULL;
    UNIQUE_THANDLE(LL_STRUCT) ll = NULL;

    STRICT_EXPECTED_CALL(UNIQUE_THANDLE_INITIALIZE_MOVE(MOCKED_STRUCT)(&t1, &t2));
    STRICT_EXPECTED_CALL(UNIQUE_THANDLE_DISPOSE(MOCKED_STRUCT)(&t1));

    // act
    UNIQUE_THANDLE_INITIALIZE_MOVE(MOCKED_STRUCT)(&t1, &t2);
    UNIQUE_THANDLE_DISPOSE(MOCKED_STRUCT)(&t1);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(t1);
    ASSERT_IS_NULL(t2);
    ASSERT_IS_NULL(ll);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
