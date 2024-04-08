// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdint.h>

#include "testrunnerswitcher.h"

#include "c_pal/malloc_multi_flex.h"

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
}

static int array1_size = 10;
static int array2_size = 20;
static int array3_size = 30;

typedef struct INNER_STURCT_TAG
{
    uint32_t inner_int_1;
    uint64_t inner_int_2;
}INNER_STRUCT;

typedef struct PARENT_STRUCT_TAG
{
    uint64_t int_1;
    uint32_t* array_1;
    uint32_t int_2;
    uint64_t* array_2;
    INNER_STRUCT* array_3;
    uint32_t int_3;
}PARENT_STRUCT;

DEFINE_MALLOC_MULTI_FLEX(PARENT_STRUCT, uint32_t, array_1, uint64_t, array_2, INNER_STRUCT, array_3)

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
}

static void assign_struct(PARENT_STRUCT* test_struct_handle)
{
    test_struct_handle->int_1 = 3;
    test_struct_handle->int_2 = 6;
    test_struct_handle->int_3 = 9;

    for (int i = 0; i < array1_size; i++)
    {
        test_struct_handle->array_1[i] = i;
    }

    for (int i = 0; i < array2_size; i++)
    {
        test_struct_handle->array_2[i] = i + 100;
    }

    for (int i = 0; i < array3_size; i++)
    {
        test_struct_handle->array_3[i].inner_int_1 = i + 1000;
        test_struct_handle->array_3[i].inner_int_2 = i + 2000;
    }
}

static void assert_struct(PARENT_STRUCT* test_struct_handle)
{
    for (int i = 0; i < array1_size; i++)
    {
        ASSERT_ARE_EQUAL(int, i, test_struct_handle->array_1[i]);
    }

    for (int i = 0; i < array2_size; i++)
    {
        ASSERT_ARE_EQUAL(int, i + 100, test_struct_handle->array_2[i]);
    }

    ASSERT_ARE_EQUAL(int, 3, test_struct_handle->int_1);
    ASSERT_ARE_EQUAL(int, 6, test_struct_handle->int_2);
    ASSERT_ARE_EQUAL(int, 9, test_struct_handle->int_3);

    for (int i = 0; i < array3_size; i++)
    {
        ASSERT_ARE_EQUAL(int, i + 1000, test_struct_handle->array_3[i].inner_int_1);
        ASSERT_ARE_EQUAL(int, i + 2000, test_struct_handle->array_3[i].inner_int_2);
    }
}

/* Tests_SRS_MALLOC_MULTI_FLEX_24_001: [ If the total amount of memory required to allocate the type along with its members exceeds SIZE_MAX then DEFINE_MALLOC_MULTI_FLEX shall fail and return NULL. ]*/
/* Tests_SRS_MALLOC_MULTI_FLEX_24_002: [ DEFINE_MALLOC_MULTI_FLEX shall call malloc to allocate memory for the struct and its members. ]*/
/* Tests_SRS_MALLOC_MULTI_FLEX_24_003: [ DEFINE_MALLOC_MULTI_FLEX shall assign address pointers to all the member arrays. ]*/
/* Tests_SRS_MALLOC_MULTI_FLEX_24_004: [ DEFINE_MALLOC_MULTI_FLEX shall succeed and return the address returned by malloc. ]*/
/* Tests_MALLOC_MULTI_FLEX_24_005: [ MALLOC_MULTI_FLEX shall expand type to the name of the malloc function in the format of: MALLOC_MULTI_FLEX_type. ]*/
TEST_FUNCTION(test_malloc_multi_flex_allocates_memory_and_assigns_address_ptr_for_parent_and_member_arrays)
{
    //arrange

    //act
    PARENT_STRUCT* test_struct_handle = MALLOC_MULTI_FLEX(PARENT_STRUCT)(sizeof(PARENT_STRUCT), 10, 20, 30);
    assign_struct(test_struct_handle);

    //assert
    assert_struct(test_struct_handle);

    //cleanup
    free(test_struct_handle);
}

DEFINE_MALLOC_MULTI_FLEX(INNER_STRUCT)

/* Tests_SRS_MALLOC_MULTI_FLEX_24_001: [ If the total amount of memory required to allocate the type along with its members exceeds SIZE_MAX then DEFINE_MALLOC_MULTI_FLEX shall fail and return NULL. ]*/
/* Tests_SRS_MALLOC_MULTI_FLEX_24_002: [ DEFINE_MALLOC_MULTI_FLEX shall call malloc to allocate memory for the struct and its members. ]*/
/* Tests_SRS_MALLOC_MULTI_FLEX_24_003: [ DEFINE_MALLOC_MULTI_FLEX shall assign address pointers to all the member arrays. ]*/
/* Tests_SRS_MALLOC_MULTI_FLEX_24_004: [ DEFINE_MALLOC_MULTI_FLEX shall succeed and return the address returned by malloc. ]*/
/* Tests_MALLOC_MULTI_FLEX_24_005: [ MALLOC_MULTI_FLEX shall expand type to the name of the malloc function in the format of: MALLOC_MULTI_FLEX_type. ]*/
TEST_FUNCTION(test_malloc_multi_flex_works_for_struct_with_no_array_members)
{
    //arrange

    //act
    INNER_STRUCT* test_struct_handle = MALLOC_MULTI_FLEX(INNER_STRUCT)(sizeof(INNER_STRUCT));
    test_struct_handle->inner_int_1 = 3;
    test_struct_handle->inner_int_2 = 6;

    //assert
    ASSERT_ARE_EQUAL(int, 3, test_struct_handle->inner_int_1);
    ASSERT_ARE_EQUAL(int, 6, test_struct_handle->inner_int_2);

    //cleanup
    free(test_struct_handle);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
