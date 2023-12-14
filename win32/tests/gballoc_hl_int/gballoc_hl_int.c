// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
}

TEST_FUNCTION(gballoc_hl_init_works)
{
    ///arrange
    gballoc_hl_deinit();

    int result;

    ///act
    result = gballoc_hl_init(NULL, NULL);

    ///assert - doesn't crash
    ASSERT_ARE_EQUAL(int, 0, result);
}

TEST_FUNCTION(gballoc_hl_deinit_works)
{
    ///act
    gballoc_hl_deinit();

    ///assert - doesn't crash

    ///clean
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));
}

TEST_FUNCTION(gballoc_hl_malloc_works)
{
    ///act (1)
    unsigned char* ptr = gballoc_hl_malloc(1);

    ///assert (1)
    ASSERT_IS_NOT_NULL(ptr);

    ///act(2)
    (void)memset(ptr, '3', 1); /*can be written*/

    ///assert (2) - doesn't crash

    ///clean
    gballoc_hl_free(ptr);
}

TEST_FUNCTION(gballoc_hl_malloc_1MB_works)
{
    ///act (1)
    unsigned char* ptr = gballoc_hl_malloc(1024 * 1024);

    ///assert (1)
    ASSERT_IS_NOT_NULL(ptr);

    ///act(2)
    (void)memset(ptr, '3', 1); /*can be written*/

    ///assert (2) - doesn't crash

    ///clean
    gballoc_hl_free(ptr);
}

TEST_FUNCTION(gballoc_hl_malloc_2_works)
{
    ///act (1)
    unsigned char* ptr = gballoc_hl_malloc_2(2,3);

    ///assert (1)
    ASSERT_IS_NOT_NULL(ptr);

    ///act(2)
    (void)memset(ptr, '3', 2*3); /*can be written*/

    ///assert (2) - doesn't crash

    ///clean
    gballoc_hl_free(ptr);
}

TEST_FUNCTION(gballoc_hl_malloc_2_1MB_works)
{
    ///act (1)
    unsigned char* ptr = gballoc_hl_malloc_2(1024, 1024);

    ///assert (1)
    ASSERT_IS_NOT_NULL(ptr);

    ///act(2)
    (void)memset(ptr, '3', 1024*1024); /*can be written*/

    ///assert (2) - doesn't crash

    ///clean
    gballoc_hl_free(ptr);
}

TEST_FUNCTION(gballoc_hl_malloc_flex_works)
{
    ///act (1)
    unsigned char* ptr = gballoc_hl_malloc_flex(2, 3, 5);

    ///assert (1)
    ASSERT_IS_NOT_NULL(ptr);

    ///act(2)
    (void)memset(ptr, '3', 2 + 3 * 5); /*can be written*/

    ///assert (2) - doesn't crash

    ///clean
    gballoc_hl_free(ptr);
}

TEST_FUNCTION(gballoc_hl_malloc_flex_1MB_works)
{
    ///act (1)
    unsigned char* ptr = gballoc_hl_malloc_flex(1024, 1024, 1024);

    ///assert (1)
    ASSERT_IS_NOT_NULL(ptr);

    ///act(2)
    (void)memset(ptr, '3', 1024 + 1024 * 1024); /*can be written*/

    ///assert (2) - doesn't crash

    ///clean
    gballoc_hl_free(ptr);
}

TEST_FUNCTION(gballoc_hl_free_works)
{
    ///arrange
    unsigned char* ptr = gballoc_hl_malloc(1);
    ASSERT_IS_NOT_NULL(ptr);

    ///act 
    gballoc_hl_free(ptr);

    ///assert - doesn't crash
}

TEST_FUNCTION(gballoc_hl_realloc_works)
{
    ///arrange
    unsigned char* ptr1 = gballoc_hl_malloc(1);
    ASSERT_IS_NOT_NULL(ptr1);
    unsigned char* ptr2;

    ///act 
    ptr2 = gballoc_hl_realloc(ptr1, 2);

    ///assert - doesn't crash
    ASSERT_IS_NOT_NULL(ptr2);

    ///clean
    gballoc_hl_free(ptr2);
}

TEST_FUNCTION(gballoc_hl_realloc_2_works)
{
    ///arrange
    unsigned char* ptr1 = gballoc_hl_malloc(1);
    ASSERT_IS_NOT_NULL(ptr1);
    unsigned char* ptr2;

    ///act 
    ptr2 = gballoc_hl_realloc_2(ptr1, 2, 3);

    ///assert - doesn't crash
    ASSERT_IS_NOT_NULL(ptr2);

    ///clean
    gballoc_hl_free(ptr2);
}

TEST_FUNCTION(gballoc_hl_realloc_flex_works)
{
    ///arrange
    unsigned char* ptr1 = gballoc_hl_malloc(1);
    ASSERT_IS_NOT_NULL(ptr1);
    unsigned char* ptr2;

    ///act 
    ptr2 = gballoc_hl_realloc_flex(ptr1, 2, 3, 5);

    ///assert - doesn't crash
    ASSERT_IS_NOT_NULL(ptr2);

    ///clean
    gballoc_hl_free(ptr2);
}

TEST_FUNCTION(gballoc_hl_calloc_works)
{
    ///arrange
    unsigned char* ptr;

    ///act 
    ptr = gballoc_hl_calloc(1, 1);

    ///assert - doesn't crash
    ASSERT_IS_NOT_NULL(ptr);
    ASSERT_IS_TRUE(0 == ptr[0]);

    ///clean
    gballoc_hl_free(ptr);
}

TEST_FUNCTION(gballoc_hl_size_returns_the_size_of_the_allocated_block)
{
    ///arrange
    unsigned char* ptr = gballoc_hl_malloc(1);
    ASSERT_IS_NOT_NULL(ptr);

    ///act 
    size_t size = gballoc_hl_size(ptr);

    ///assert - doesn't crash
    ASSERT_ARE_EQUAL(size_t, 1, size);

    ///clean
    gballoc_hl_free(ptr);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
