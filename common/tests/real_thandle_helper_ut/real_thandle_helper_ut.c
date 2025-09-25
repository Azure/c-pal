// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "real_thandle_helper_ut_pch.h"

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

MOCKABLE_FUNCTION_WITH_CODE(, void, a_function_with_mockable_calls)
{
    volatile_atomic int32_t an_interlocked_variable;
    THANDLE(MOCKED_STRUCT) a_thandle_variable = NULL;
    (void)interlocked_exchange(&an_interlocked_variable, 0);
    THANDLE_INITIALIZE(MOCKED_STRUCT)(&a_thandle_variable, g.test_mocked_struct);
    ASSERT_IS_NOT_NULL(a_thandle_variable);
    int32_t result = interlocked_add(&an_interlocked_variable, 2);
    // verify we are using real interlocked_add.
    ASSERT_ARE_EQUAL(int32_t, 2, result);
    THANDLE_ASSIGN(MOCKED_STRUCT)(&a_thandle_variable, NULL);
    result = interlocked_increment(&an_interlocked_variable);
    ASSERT_ARE_EQUAL(int32_t, 3, result);
}
MOCKABLE_FUNCTION_WITH_CODE_END(a_function_with_mockable_calls);

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

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_charptr_register_types");

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_INTERLOCKED_GLOBAL_MOCK_HOOK();
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

TEST_FUNCTION(function_call_is_mocked_correctly)
{
    // arrange
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(MOCKED_STRUCT)(IGNORED_ARG, g.test_mocked_struct));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 2));
    STRICT_EXPECTED_CALL(THANDLE_ASSIGN(MOCKED_STRUCT)(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));

    // act
    a_function_with_mockable_calls();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // ablution
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)