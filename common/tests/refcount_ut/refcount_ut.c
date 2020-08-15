// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#else
#include <stdlib.h>
#include <stddef.h>
#endif

#include "real_gballoc_ll.h"

void* my_malloc(size_t size)
{
    return real_gballoc_ll_malloc(size);
}

void my_free(void* ptr)
{
    real_gballoc_ll_free(ptr);
}

#include "azure_macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"
#include "some_refcount_impl.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"

#define ENABLE_MOCKS
#include "umock_c/umock_c_prod.h"
#include "azure_c_pal/interlocked.h"
#include "azure_c_pal/gballoc_hl.h"
#include "azure_c_pal/gballoc_hl_redirect.h"
#include "azure_c_pal/refcount.h"
#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"
#include "real_interlocked.h"

static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

typedef struct TEST_STRUCT_TAG
{
    int dummy;
} TEST_STRUCT;

MOCK_FUNCTION_WITH_CODE(, void*, test_malloc, size_t, size)
MOCK_FUNCTION_END(my_malloc(size))
MOCK_FUNCTION_WITH_CODE(, void, test_free, void*, ptr)
    my_free(ptr);
MOCK_FUNCTION_END()

/* Tests_SRS_REFCOUNT_01_011: [ DEFINE_REFCOUNT_TYPE_WITH_CUSTOM_ALLOC shall behave like DEFINE_REFCOUNT_TYPE, but use malloc_func and free_func for memory allocation and free.  ]*/
DEFINE_REFCOUNT_TYPE_WITH_CUSTOM_ALLOC(TEST_STRUCT, test_malloc, test_free);

BEGIN_TEST_SUITE(refcount_unittests)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        
        ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

        g_testByTest = TEST_MUTEX_CREATE();
        ASSERT_IS_NOT_NULL(g_testByTest);

        umock_c_init(on_umock_c_error);

        REGISTER_UMOCK_ALIAS_TYPE(POS_HANDLE, void*);

        REGISTER_GLOBAL_MOCK_HOOK(malloc, my_malloc);
        REGISTER_GLOBAL_MOCK_HOOK(free, my_free);

        ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types());

        REGISTER_INTERLOCKED_GLOBAL_MOCK_HOOK();
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        umock_c_deinit();

        TEST_MUTEX_DESTROY(g_testByTest);

        real_gballoc_hl_deinit();
    }

    TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
    {
        if (TEST_MUTEX_ACQUIRE(g_testByTest))
        {
            ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
        }

        umock_c_reset_all_calls();
    }

    TEST_FUNCTION_CLEANUP(TestMethodCleanup)
    {
        TEST_MUTEX_RELEASE(g_testByTest);
    }

    /* REFCOUNT_TYPE_CREATE */

    /* Tests_SRS_REFCOUNT_01_002: [ REFCOUNT_TYPE_CREATE shall allocate memory for the type that is ref counted. ]*/
    /* Tests_SRS_REFCOUNT_01_003: [ On success it shall return a non-NULL handle to the allocated ref counted type type. ]*/
    /* Tests_SRS_REFCOUNT_01_010: [ Memory allocation/free shall be performed by using the functions malloc and free. ]*/
    TEST_FUNCTION(refcount_create_returns_non_NULL)
    {
        ///arrange
        POS_HANDLE p;
        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
        STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1));

        ///act
        p = Pos_Create(4);

        ///assert
        ASSERT_IS_NOT_NULL(p);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Pos_Destroy(p);
    }

    /* Tests_SRS_REFCOUNT_01_004: [ If any error occurrs, REFCOUNT_TYPE_CREATE shall return NULL. ]*/
    TEST_FUNCTION(when_malloc_fails_refcount_create_fails)
    {
        ///arrange
        POS_HANDLE p;
        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
            .SetReturn(NULL);

        ///act
        p = Pos_Create(4);

        ///assert
        ASSERT_IS_NULL(p);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    /* REFCOUNT_TYPE_CREATE_WITH_EXTRA_SIZE */

    /* Tests_SRS_REFCOUNT_01_005: [ REFCOUNT_TYPE_CREATE_WITH_EXTRA_SIZE shall allocate memory for the type that is ref counted (type) plus extra memory enough to hold size bytes. ]*/
    /* Tests_SRS_REFCOUNT_01_006: [ On success it shall return a non-NULL handle to the allocated ref counted type type. ]*/
    TEST_FUNCTION(refcount_create_with_extra_size_returns_non_NULL)
    {
        ///arrange
        POS_HANDLE p;
        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
        STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1));

        ///act
        p = Pos_Create_With_Extra_Size(4, 42);

        ///assert
        ASSERT_IS_NOT_NULL(p);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Pos_Destroy(p);
    }

    /* Tests_SRS_REFCOUNT_01_007: [ If any error occurrs, REFCOUNT_TYPE_CREATE_WITH_EXTRA_SIZE shall return NULL. ]*/
    TEST_FUNCTION(when_malloc_fails_refcount_create_with_extra_size_also_fails)
    {
        ///arrange
        POS_HANDLE p;
        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
            .SetReturn(NULL);

        ///act
        p = Pos_Create_With_Extra_Size(4, 42);

        ///assert
        ASSERT_IS_NULL(p);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    /* REFCOUNT_TYPE_DESTROY */

    /* Tests_SRS_REFCOUNT_01_008: [ REFCOUNT_TYPE_DESTROY shall free the memory allocated by REFCOUNT_TYPE_CREATE or REFCOUNT_TYPE_CREATE_WITH_EXTRA_SIZE. ]*/
    /* Tests_SRS_REFCOUNT_01_010: [ Memory allocation/free shall be performed by using the functions malloc and free. ]*/
    TEST_FUNCTION(refcount_DEC_REF_after_create_says_we_should_free)
    {
        ///arrange
        POS_HANDLE p;
        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
        STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1));
        p = Pos_Create(4);
        umock_c_reset_all_calls();

        ///act
        STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
        STRICT_EXPECTED_CALL(free(IGNORED_ARG));
        Pos_Destroy(p);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_REFCOUNT_01_009: [ If counted_type is NULL, REFCOUNT_TYPE_DESTROY shall return. ]*/
    TEST_FUNCTION(refcount_DESTROY_with_NULL_returns)
    {
        ///arrange

        ///act
        Pos_Destroy(NULL);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    TEST_FUNCTION(refcount_INC_REF_and_DEC_REF_after_create_says_we_should_not_free)
    {
        ///arrange
        POS_HANDLE p, clone_of_p;
        p = Pos_Create(2);
        clone_of_p = Pos_Clone(p);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
        ///act
        Pos_Destroy(p);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        Pos_Destroy(p);
    }

    TEST_FUNCTION(refcount_after_clone_it_takes_2_destroys_to_free)
    {
        ///arrange
        POS_HANDLE p, clone_of_p;
        p = Pos_Create(2);
        clone_of_p = Pos_Clone(p);
        Pos_Destroy(p);
        umock_c_reset_all_calls();

        ///act
        STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
        STRICT_EXPECTED_CALL(free(IGNORED_ARG));
        Pos_Destroy(clone_of_p);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    /* Tests_SRS_REFCOUNT_01_011: [ DEFINE_REFCOUNT_TYPE_WITH_CUSTOM_ALLOC shall behave like DEFINE_REFCOUNT_TYPE, but use malloc_func and free_func for memory allocation and free.  ]*/
    TEST_FUNCTION(the_specified_malloc_function_from_the_define_is_used)
    {
        ///arrange
        STRICT_EXPECTED_CALL(test_malloc(IGNORED_ARG));
        STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1));

        ///act
        TEST_STRUCT* result = REFCOUNT_TYPE_CREATE(TEST_STRUCT);

        ///assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    /* Tests_SRS_REFCOUNT_01_011: [ DEFINE_REFCOUNT_TYPE_WITH_CUSTOM_ALLOC shall behave like DEFINE_REFCOUNT_TYPE, but use malloc_func and free_func for memory allocation and free.  ]*/
    TEST_FUNCTION(the_specified_malloc_function_from_the_define_is_used_by_create_with_extra_size)
    {
        ///arrange
        STRICT_EXPECTED_CALL(test_malloc(IGNORED_ARG));
        STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1));

        ///act
        TEST_STRUCT* result = REFCOUNT_TYPE_CREATE_WITH_EXTRA_SIZE(TEST_STRUCT, 1);

        ///assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    /* Tests_SRS_REFCOUNT_01_011: [ DEFINE_REFCOUNT_TYPE_WITH_CUSTOM_ALLOC shall behave like DEFINE_REFCOUNT_TYPE, but use malloc_func and free_func for memory allocation and free.  ]*/
    TEST_FUNCTION(the_specified_free_function_from_the_define_is_used)
    {
        ///arrange
        TEST_STRUCT* result = REFCOUNT_TYPE_CREATE(TEST_STRUCT);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(test_free(IGNORED_ARG));

        ///act
        REFCOUNT_TYPE_DESTROY(TEST_STRUCT, result);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

END_TEST_SUITE(refcount_unittests)
