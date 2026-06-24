// Copyright (c) Microsoft. All rights reserved.



#include "malloc_multi_flex_ut_pch.h"

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}


BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types failed");

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_reset_all_calls();
    umock_c_negative_tests_init();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    umock_c_negative_tests_deinit();
}

/* Tests_SRS_MALLOC_MULTI_FLEX_STRUCT_24_002: [ DEFINE_MALLOC_MULTI_FLEX_STRUCT shall call malloc to allocate memory for the struct and its members. ]*/
/* Tests_SRS_MALLOC_MULTI_FLEX_STRUCT_24_003: [ DEFINE_MALLOC_MULTI_FLEX_STRUCT shall assign address pointers to all the member arrays. ]*/
/* Tests_SRS_MALLOC_MULTI_FLEX_STRUCT_24_004: [ DEFINE_MALLOC_MULTI_FLEX_STRUCT shall succeed and return the address returned by malloc function. ]*/
/* Tests_SRS_MALLOC_MULTI_FLEX_STRUCT_24_005: [ MALLOC_MULTI_FLEX_STRUCT shall expand type to the name of the malloc function in the format of: MALLOC_MULTI_FLEX_STRUCT_type. ]*/
TEST_FUNCTION(malloc_multi_flex_succeeds)
{
    // arrange
    size_t total_size = sizeof(PARENT_STRUCT) + 10 * sizeof(uint32_t) + 20 * sizeof(uint64_t) + 30 * (sizeof(INNER_STRUCT)) + alignof(uint32_t) - 1 + alignof(INNER_STRUCT)-1 + alignof(uint64_t) - 1;
    STRICT_EXPECTED_CALL(malloc(total_size));

    // act
    PARENT_STRUCT* parent_struct = create_parent_struct(10, 20, 30);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(parent_struct);
    for (int i = 0; i < 10; i++)
    {
        ASSERT_ARE_EQUAL(int, i, parent_struct->array_1[i]);
    }

    for (int i = 0; i < 20; i++)
    {
        ASSERT_ARE_EQUAL(int, i + 100, parent_struct->array_2[i]);
    }

    ASSERT_ARE_EQUAL(int, 3, parent_struct->int_1);
    ASSERT_ARE_EQUAL(int, 6, parent_struct->int_2);
    ASSERT_ARE_EQUAL(int, 9, parent_struct->int_3);

    for (int i = 0; i < 30; i++)
    {
        ASSERT_ARE_EQUAL(int, i + 1000, parent_struct->array_3[i].inner_int_1);
        ASSERT_ARE_EQUAL(int, i + 2000, parent_struct->array_3[i].inner_int_2);
    }

    //cleanup
    free(parent_struct);
}

/* Tests_SRS_MALLOC_MULTI_FLEX_STRUCT_24_006: [ If malloc fails, DEFINE_MALLOC_MULTI_FLEX_STRUCT shall fail and return NULL. ]*/
TEST_FUNCTION(malloc_multi_flex_fails_when_malloc_fails)
{
    // arrange
    size_t total_size = sizeof(PARENT_STRUCT) + 10 * sizeof(uint32_t) + 20 * sizeof(uint64_t) + 30 * (sizeof(INNER_STRUCT)) + alignof(uint32_t) - 1 + alignof(INNER_STRUCT) - 1 + alignof(uint64_t) - 1;
    STRICT_EXPECTED_CALL(malloc(total_size))
        .SetReturn(NULL);

    // act
    PARENT_STRUCT* parent_struct = create_parent_struct(10, 20, 30);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(parent_struct);
}

/* Tests_SRS_MALLOC_MULTI_FLEX_STRUCT_24_001: [ If the total amount of memory required to allocate the type along with its members exceeds SIZE_MAX then DEFINE_MALLOC_MULTI_FLEX_STRUCT shall fail and return NULL. ]*/
TEST_FUNCTION(malloc_multi_flex_fails_when_size_exceeds_SIZE_MAX)
{
    // arrange

    // act
    PARENT_STRUCT* parent_struct = create_parent_struct(UINT64_MAX, 20, 30);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(parent_struct);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
