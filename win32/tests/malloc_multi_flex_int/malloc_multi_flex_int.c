// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdint.h>
#include <stddef.h>

#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/malloc_multi_flex.h"

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
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

DECLARE_MALLOC_MULTI_FLEX_STRUCT(PARENT_STRUCT,
    FIELDS(uint64_t, int_1, uint32_t, int_2, uint32_t, int_3),
    ARRAY_FIELDS(uint32_t, array_1, uint64_t, array_2, INNER_STRUCT, array_3))

DEFINE_MALLOC_MULTI_FLEX_STRUCT(PARENT_STRUCT,
    FIELDS(uint64_t, int_1, uint32_t, int_2, uint32_t, int_3),
    ARRAY_FIELDS(uint32_t, array_1, uint64_t, array_2, INNER_STRUCT, array_3))

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

TEST_FUNCTION(test_malloc_multi_flex_allocates_memory_and_assigns_address_ptr_for_parent_and_member_arrays_of_different_types)
{
    //arrange

    //act
    PARENT_STRUCT* test_struct_handle = MALLOC_MULTI_FLEX_STRUCT(PARENT_STRUCT)(sizeof(PARENT_STRUCT), array1_size, array2_size, array3_size);
    size_t bytes_to_set = (array1_size * sizeof(uint32_t)) + alignof(uint32_t) - 1 + (array2_size * sizeof(uint64_t)) + alignof(uint64_t) - 1 + (array3_size * sizeof(INNER_STRUCT)) + alignof(INNER_STRUCT) - 1;
    (void)memset((void*)((uintptr_t)test_struct_handle + sizeof(PARENT_STRUCT)), 0xFF, bytes_to_set);
    assign_struct(test_struct_handle);

    //assert
    assert_struct(test_struct_handle);

    //cleanup
    free(test_struct_handle);
}

DECLARE_MALLOC_MULTI_FLEX_STRUCT(PARENT_STRUCT2,
    FIELDS(uint32_t, int_1, uint32_t, int_2),
    ARRAY_FIELDS(uint32_t, array_1, uint32_t, array_2))

DEFINE_MALLOC_MULTI_FLEX_STRUCT(PARENT_STRUCT2,
    FIELDS(uint32_t, int_1, uint32_t, int_2),
    ARRAY_FIELDS(uint32_t, array_1, uint32_t, array_2))

TEST_FUNCTION(test_malloc_multi_flex_allocates_memory_and_assigns_address_ptr_for_parent_and_member_arrays_of_same_type)
{
    //arrange

    //act
    PARENT_STRUCT2* test_struct_handle = MALLOC_MULTI_FLEX_STRUCT(PARENT_STRUCT2)(sizeof(PARENT_STRUCT2), array1_size, array2_size);
    size_t bytes_to_set = (array1_size * sizeof(uint32_t)) + alignof(uint32_t) - 1 + (array2_size * sizeof(uint32_t)) + alignof(uint32_t) - 1;
    (void)memset((void*)((uintptr_t)test_struct_handle + sizeof(PARENT_STRUCT2)), 0xFF, bytes_to_set);
    test_struct_handle->int_1 = 3;
    test_struct_handle->int_2 = 6;
    for (int i = 0; i < array1_size; i++)
    {
        test_struct_handle->array_1[i] = i;
    }
    for (int i = 0; i < array2_size; i++)
    {
        test_struct_handle->array_2[i] = i + 100;
    }

    //assert
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

    //cleanup
    free(test_struct_handle);
}

DECLARE_MALLOC_MULTI_FLEX_STRUCT(PARENT_STRUCT3,
    FIELDS(),
    ARRAY_FIELDS(uint32_t, array_1))

DEFINE_MALLOC_MULTI_FLEX_STRUCT(PARENT_STRUCT3,
    FIELDS(),
    ARRAY_FIELDS(uint32_t, array_1))

TEST_FUNCTION(test_malloc_multi_flex_allocates_memory_and_assigns_address_ptr_for_parent_and_single_member_array)
{
    //arrange

    //act
    PARENT_STRUCT3* test_struct_handle = MALLOC_MULTI_FLEX_STRUCT(PARENT_STRUCT3)(sizeof(PARENT_STRUCT3), array1_size);
    size_t bytes_to_set = (array1_size * sizeof(uint32_t)) + alignof(uint32_t) - 1;
    (void)memset((void*)((uintptr_t)test_struct_handle + sizeof(PARENT_STRUCT3)), 0xFF, bytes_to_set);
    for (int i = 0; i < array1_size; i++)
    {
        test_struct_handle->array_1[i] = i;
    }

    //assert
    for (int i = 0; i < array1_size; i++)
    {
        ASSERT_ARE_EQUAL(int, i, test_struct_handle->array_1[i]);
    }

    //cleanup
    free(test_struct_handle);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
