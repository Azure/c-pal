// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "real_unique_thandle_helper_ut_pch.h"

#include "real_interlocked_renames.h" // IWYU pragma: keep

typedef struct MOCKED_STRUCT_TAG
{
    uint8_t dummy;
} MOCKED_STRUCT;

REAL_UNIQUE_THANDLE_DECLARE(MOCKED_STRUCT);

REAL_UNIQUE_THANDLE_DEFINE(MOCKED_STRUCT);

#include "real_interlocked_undo_rename.h" // IWYU pragma: keep

static struct G_TAG
{
    const MOCKED_STRUCT* disposed; /*the last object passed to dispose_MOCKED_STRUCT*/
    uint32_t dispose_count;
} g;

static void dispose_MOCKED_STRUCT(REAL_MOCKED_STRUCT* mocked_struct)
{
    g.disposed = mocked_struct;
    g.dispose_count++;
}

/*puts a new object in handle and returns it, or returns NULL on failure*/
static MOCKED_STRUCT* create_object_in(UNIQUE_THANDLE(MOCKED_STRUCT) handle)
{
    MOCKED_STRUCT* result = UNIQUE_THANDLE_MALLOC(REAL_MOCKED_STRUCT)(dispose_MOCKED_STRUCT);
    if (result == NULL)
    {
        LogError("failure in UNIQUE_THANDLE_MALLOC(REAL_MOCKED_STRUCT)(dispose_MOCKED_STRUCT)");
    }
    else
    {
        UNIQUE_THANDLE_ADOPT(REAL_MOCKED_STRUCT)(handle, result);
    }
    return result;
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

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_INTERLOCKED_GLOBAL_MOCK_HOOK();
    REGISTER_UNIQUE_THANDLE_UMOCK_ALIAS_TYPES(MOCKED_STRUCT);
    REGISTER_REAL_UNIQUE_THANDLE_MOCK_HOOK(MOCKED_STRUCT);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();
    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    g.disposed = NULL;
    g.dispose_count = 0;
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
}

TEST_FUNCTION(mocked_UNIQUE_THANDLE_functions_run_the_real_functions) // no-srs
{
    // arrange
    UNIQUE_THANDLE(MOCKED_STRUCT) source = { { NULL } };
    UNIQUE_THANDLE(MOCKED_STRUCT) moved = { { NULL } };
    MOCKED_STRUCT* original = create_object_in(source);
    ASSERT_IS_NOT_NULL(original);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(UNIQUE_THANDLE_INITIALIZE_MOVE(MOCKED_STRUCT)(moved, source));
    STRICT_EXPECTED_CALL(UNIQUE_THANDLE_DISPOSE(MOCKED_STRUCT)(moved));

    // act
    UNIQUE_THANDLE_INITIALIZE_MOVE(MOCKED_STRUCT)(moved, source);
    MOCKED_STRUCT* after_move = UNIQUE_THANDLE_GET_T(REAL_MOCKED_STRUCT)(moved);
    UNIQUE_THANDLE_DISPOSE(MOCKED_STRUCT)(moved);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, original, after_move);
    ASSERT_IS_NULL(UNIQUE_THANDLE_GET_T(REAL_MOCKED_STRUCT)(source));
    ASSERT_IS_NULL(UNIQUE_THANDLE_GET_T(REAL_MOCKED_STRUCT)(moved));
    ASSERT_ARE_EQUAL(uint32_t, 1, g.dispose_count);
    ASSERT_ARE_EQUAL(void_ptr, original, g.disposed);
}

TEST_FUNCTION(an_expected_call_on_a_handle_does_not_match_a_call_on_another_handle) // no-srs
{
    // arrange
    UNIQUE_THANDLE(MOCKED_STRUCT) expected_handle = { { NULL } };
    UNIQUE_THANDLE(MOCKED_STRUCT) other_handle = { { NULL } };
    ASSERT_ARE_NOT_EQUAL(void_ptr, (void*)expected_handle, (void*)other_handle);

    STRICT_EXPECTED_CALL(UNIQUE_THANDLE_DISPOSE(MOCKED_STRUCT)(expected_handle));

    // act
    UNIQUE_THANDLE_DISPOSE(MOCKED_STRUCT)(other_handle);

    // assert
    ASSERT_ARE_NOT_EQUAL(char_ptr, "", umock_c_get_expected_calls());
    ASSERT_ARE_NOT_EQUAL(char_ptr, "", umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IGNORED_ARG_matches_a_call_on_any_handle) // no-srs
{
    // arrange
    UNIQUE_THANDLE(MOCKED_STRUCT) handle = { { NULL } };
    MOCKED_STRUCT* object = create_object_in(handle);
    ASSERT_IS_NOT_NULL(object);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_struct_get_value(IGNORED_ARG))
        .SetReturn(5);
    STRICT_EXPECTED_CALL(UNIQUE_THANDLE_DISPOSE(MOCKED_STRUCT)(IGNORED_ARG));

    // act
    int value = mocked_struct_get_value(handle);
    UNIQUE_THANDLE_DISPOSE(MOCKED_STRUCT)(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 5, value);
    ASSERT_ARE_EQUAL(uint32_t, 1, g.dispose_count);
    ASSERT_ARE_EQUAL(void_ptr, object, g.disposed);
}

TEST_FUNCTION(CopyOutArgumentBuffer_puts_an_object_in_the_handle_of_a_mocked_create_function) // no-srs
{
    // arrange
    UNIQUE_THANDLE(MOCKED_STRUCT) handle = { { NULL } };
    MOCKED_STRUCT* object = UNIQUE_THANDLE_MALLOC(REAL_MOCKED_STRUCT)(dispose_MOCKED_STRUCT);
    ASSERT_IS_NOT_NULL(object);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_struct_create(42, handle))
        .CopyOutArgumentBuffer_mocked_struct(&object, sizeof(object))
        .SetReturn(0);

    // act
    int result = mocked_struct_create(42, handle);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, object, UNIQUE_THANDLE_GET_T(REAL_MOCKED_STRUCT)(handle));

    // cleanup
    UNIQUE_THANDLE_DISPOSE(MOCKED_STRUCT)(handle);
}

TEST_FUNCTION(ValidateArgumentBuffer_checks_the_object_that_a_const_handle_holds) // no-srs
{
    // arrange
    UNIQUE_THANDLE(MOCKED_STRUCT) handle = { { NULL } };
    MOCKED_STRUCT* object = create_object_in(handle);
    ASSERT_IS_NOT_NULL(object);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_struct_get_value(handle))
        .ValidateArgumentBuffer(1, &object, sizeof(object))
        .SetReturn(7);

    // act
    int value = mocked_struct_get_value(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 7, value);

    // cleanup
    UNIQUE_THANDLE_DISPOSE(MOCKED_STRUCT)(handle);
}

TEST_FUNCTION(ValidateArgumentBuffer_reports_a_handle_that_holds_another_object) // no-srs
{
    // arrange
    UNIQUE_THANDLE(MOCKED_STRUCT) handle = { { NULL } };
    UNIQUE_THANDLE(MOCKED_STRUCT) other_handle = { { NULL } };
    MOCKED_STRUCT* object = create_object_in(handle);
    ASSERT_IS_NOT_NULL(object);
    ASSERT_IS_NOT_NULL(create_object_in(other_handle));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_struct_get_value(IGNORED_ARG))
        .ValidateArgumentBuffer(1, &object, sizeof(object));

    // act
    (void)mocked_struct_get_value(other_handle);

    // assert
    ASSERT_ARE_NOT_EQUAL(char_ptr, "", umock_c_get_expected_calls());
    ASSERT_ARE_NOT_EQUAL(char_ptr, "", umock_c_get_actual_calls());

    // cleanup
    UNIQUE_THANDLE_DISPOSE(MOCKED_STRUCT)(handle);
    UNIQUE_THANDLE_DISPOSE(MOCKED_STRUCT)(other_handle);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
