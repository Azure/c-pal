// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#if defined(GB_MEASURE_MEMORY_FOR_THIS)
#undef GB_MEASURE_MEMORY_FOR_THIS
#endif

#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif

#include "azure_macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"
#include "lock.h"
#include "gballoc.h"


#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)~(size_t)0)
#endif

static TEST_MUTEX_HANDLE g_testByTest;

static void* TEST_ALLOC_PTR1 = (void*)0x4242;
static void* TEST_ALLOC_PTR2 = (void*)0x4243;
static void* TEST_REALLOC_PTR = (void*)0x4245;

#define OVERHEAD_SIZE    4096
static const LOCK_HANDLE TEST_LOCK_HANDLE = (LOCK_HANDLE)0x4244;

#define ENABLE_MOCKS

#include "umock_c/umock_c.h"
#include "umock_c/umock_c_prod.h"

IMPLEMENT_UMOCK_C_ENUM_TYPE(LOCK_RESULT, LOCK_RESULT_VALUES);

#ifdef __cplusplus
extern "C" {
#endif
    MOCKABLE_FUNCTION(, void*, mock_malloc, size_t, size);
    MOCKABLE_FUNCTION(, void*, mock_calloc, size_t, nmemb, size_t, size);
    MOCKABLE_FUNCTION(, void*, mock_realloc, void*, ptr, size_t, size);
    MOCKABLE_FUNCTION(, void, mock_free, void*, ptr);

    MOCKABLE_FUNCTION(, LOCK_HANDLE, Lock_Init);
    MOCKABLE_FUNCTION(, LOCK_RESULT, Lock_Deinit, LOCK_HANDLE, handle);
    MOCKABLE_FUNCTION(, LOCK_RESULT, Lock, LOCK_HANDLE, handle);
    MOCKABLE_FUNCTION(, LOCK_RESULT, Unlock, LOCK_HANDLE, handle);
#ifdef __cplusplus
}
#endif

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(GBAlloc_UnitTests)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    int result;

    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    result = umock_c_init(on_umock_c_error);
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_UMOCK_ALIAS_TYPE(LOCK_HANDLE, void*);
    REGISTER_TYPE(LOCK_RESULT, LOCK_RESULT);

    REGISTER_GLOBAL_MOCK_RETURN(mock_malloc, TEST_ALLOC_PTR1);
    REGISTER_GLOBAL_MOCK_RETURN(mock_realloc, TEST_ALLOC_PTR1);
    REGISTER_GLOBAL_MOCK_RETURN(mock_calloc, TEST_ALLOC_PTR1);

    REGISTER_GLOBAL_MOCK_RETURN(Lock_Init, TEST_LOCK_HANDLE);
    REGISTER_GLOBAL_MOCK_RETURN(Lock, LOCK_OK);
    REGISTER_GLOBAL_MOCK_RETURN(Unlock, LOCK_OK);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();
    TEST_MUTEX_DESTROY(g_testByTest);
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
    gballoc_deinit();

    TEST_MUTEX_RELEASE(g_testByTest);
}

/* gballoc_init */

/* Tests_SRS_GBALLOC_01_024: [gballoc_init shall initialize the gballoc module and return 0 upon success.] */
/* Tests_SRS_GBALLOC_01_026: [gballoc_Init shall create a lock handle that will be used to make the other gballoc APIs thread-safe.] */
TEST_FUNCTION(when_gballoc_init_calls_lock_init_and_it_succeeds_then_gballoc_init_succeeds)
{
    // arrange
    int result;
    STRICT_EXPECTED_CALL(Lock_Init());

    // act
    result = gballoc_init();

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_GBALLOC_01_027: [If the Lock creation fails, gballoc_init shall return a non-zero value.] */
TEST_FUNCTION(when_lock_init_fails_gballoc_init_fails)
{
    // arrange
    int result;
    STRICT_EXPECTED_CALL(Lock_Init())
        .SetReturn((LOCK_HANDLE)NULL);

    // act
    result = gballoc_init();

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_GBALLOC_01_025: [Init after Init shall fail and return a non-zero value.] */
TEST_FUNCTION(gballoc_init_after_gballoc_init_fails)
{
    // arrange
    int result;
    STRICT_EXPECTED_CALL(Lock_Init());
    gballoc_init();

    //act
    result = gballoc_init();

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* gballoc_deinit */

/* Tests_SRS_GBALLOC_01_028: [gballoc_deinit shall free all resources allocated by gballoc_init.] */
TEST_FUNCTION(gballoc_deinit_frees_the_lock_when_the_module_was_initialized)
{
    // arrange
    gballoc_init();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock_Deinit(TEST_LOCK_HANDLE));

    // act
    gballoc_deinit();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_GBALLOC_01_029: [if gballoc is not initialized gballoc_deinit shall do nothing.] */
TEST_FUNCTION(gballoc_deinit_after_gballoc_deinit_doesnot_free_lock)
{
    //arrange

    // act
    gballoc_deinit();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* gballoc_malloc */

/* Tests_SRS_GBALLOC_01_048: [If acquiring the lock fails, gballoc_malloc shall return NULL.] */
TEST_FUNCTION(when_acquiring_the_lock_fails_gballoc_malloc_fails)
{
    // arrange
    void* result;
    gballoc_init();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE))
        .SetReturn(LOCK_ERROR);

    // act
    result = gballoc_malloc(1);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_GBALLOC_01_003: [gballoc_malloc shall call the C99 malloc function and return its result.] */
/* Tests_SRS_GBALLOC_01_004: [If the underlying malloc call is successful, gballoc_malloc shall increment the total memory used with the amount indicated by size.] */
/* Tests_SRS_GBALLOC_01_030: [gballoc_malloc shall ensure thread safety by using the lock created by gballoc_Init.] */
TEST_FUNCTION(gballoc_malloc_with_0_Size_Calls_Underlying_malloc)
{
    // arrange
    void* result;
    void* allocation;
    gballoc_init();
    umock_c_reset_all_calls();
    allocation = malloc(OVERHEAD_SIZE);

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    /* don't quite like this, but I'm unsure I want to invest more in this memory counting */
    EXPECTED_CALL(mock_malloc(0))
        .SetReturn(allocation);
    /* This is the call to the underlying malloc with the size we want to allocate */
    STRICT_EXPECTED_CALL(mock_malloc(0));
    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_HANDLE));

    // act
    result = gballoc_malloc(0);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_ALLOC_PTR1, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());


    // cleanup
    gballoc_free(result);
    free(allocation);
}

/* Tests_SRS_GBALLOC_01_003: [gballoc_malloc shall call the C99 malloc function and return its result.] */
/* Tests_SRS_GBALLOC_01_004: [If the underlying malloc call is successful, gballoc_malloc shall increment the total memory used with the amount indicated by size.] */
TEST_FUNCTION(gballoc_malloc_with_1_Size_Calls_Underlying_malloc_And_Increases_Max_Used)
{
    // arrange
    void* result;
    void* allocation;
    gballoc_init();
    umock_c_reset_all_calls();
    allocation = malloc(OVERHEAD_SIZE);

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    /* don't quite like this, but I'm unsure I want to invest more in this memory counting */
    EXPECTED_CALL(mock_malloc(0))
        .SetReturn(allocation);
    /* This is the call to the underlying malloc with the size we want to allocate */
    STRICT_EXPECTED_CALL(mock_malloc(1));
    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_HANDLE));

    // act
    result = gballoc_malloc(1);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_ALLOC_PTR1, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_free(result);
    free(allocation);
}

/* Tests_SRS_GBALLOC_01_012: [When the underlying malloc call fails, gballoc_malloc shall return NULL and size should not be counted towards total memory used.] */
TEST_FUNCTION(When_malloc_Fails_Then_gballoc_malloc_fails_too)
{
    // arrange
    void* result;
    void* allocation;
    gballoc_init();
    umock_c_reset_all_calls();
    allocation = malloc(OVERHEAD_SIZE);

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    /* don't quite like this, but I'm unsure I want to invest more in this memory counting */
    EXPECTED_CALL(mock_malloc(0))
        .SetReturn(allocation);
    /* This is the call to the underlying malloc with the size we want to allocate */
    STRICT_EXPECTED_CALL(mock_malloc(1))
        .SetReturn((void*)NULL);
    STRICT_EXPECTED_CALL(mock_free(allocation));
    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_HANDLE));

    // act
    result = gballoc_malloc(1);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    free(allocation);
}

/* Tests_SRS_GBALLOC_01_013: [When gballoc_malloc fails allocating memory for its internal use, gballoc_malloc shall return NULL.] */
TEST_FUNCTION(When_allocating_memory_for_tracking_information_fails_Then_gballoc_malloc_fails_too)
{
    // arrange
    void* result;
    gballoc_init();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    /* don't quite like this, but I'm unsure I want to invest more in this memory counting */
    EXPECTED_CALL(mock_malloc(0))
        .SetReturn((void*)NULL);
    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_HANDLE));

    // act
    result = gballoc_malloc(1);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_GBALLOC_01_039: [If gballoc was not initialized gballoc_malloc shall simply call malloc without any memory tracking being performed.] */
TEST_FUNCTION(gballoc_malloc_after_deinit_calls_crt_malloc)
{
    // arrange
    void* result;
    STRICT_EXPECTED_CALL(mock_malloc(1));

    //act
    result = gballoc_malloc(1);

    //assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* gballoc_calloc */

/* Tests_SRS_GBALLOC_01_046: [If acquiring the lock fails, gballoc_calloc shall return NULL.] */
TEST_FUNCTION(when_acquiring_the_lock_fails_gballoc_calloc_fails)
{
    // arrange
    void* result;
    gballoc_init();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE))
    .SetReturn(LOCK_ERROR);

    // act
    result = gballoc_calloc(1,1);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_GBALLOC_01_020: [gballoc_calloc shall call the C99 calloc function and return its result.] */
/* Tests_SRS_GBALLOC_01_021: [If the underlying calloc call is successful, gballoc_calloc shall increment the total memory used with nmemb*size.] */
/* Tests_SRS_GBALLOC_01_031: [gballoc_calloc shall ensure thread safety by using the lock created by gballoc_Init] */
TEST_FUNCTION(gballoc_calloc_with_0_Size_And_ItemCount_Calls_Underlying_calloc)
{
    // arrange
    void* result;
    void* allocation;
    gballoc_init();
    umock_c_reset_all_calls();
    allocation = malloc(OVERHEAD_SIZE);

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    /* don't quite like this, but I'm unsure I want to invest more in this memory counting */
    EXPECTED_CALL(mock_malloc(0))
        .SetReturn(allocation);
    /* This is the call to the underlying malloc with the size we want to allocate */
    STRICT_EXPECTED_CALL(mock_calloc(0, 0));
    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_HANDLE));

    // act
    result = gballoc_calloc(0, 0);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_ALLOC_PTR1, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_free(result);
    free(allocation);
}

/* Tests_SRS_GBALLOC_01_020: [gballoc_calloc shall call the C99 calloc function and return its result.] */
/* Tests_SRS_GBALLOC_01_021: [If the underlying calloc call is successful, gballoc_calloc shall increment the total memory used with nmemb*size.] */
TEST_FUNCTION(gballoc_calloc_with_1_Item_Of_1_Size_Calls_Underlying_malloc_And_Increases_Max_Used)
{
    // arrange
    void* result;
    void* allocation;
    gballoc_init();
    umock_c_reset_all_calls();
    allocation = malloc(OVERHEAD_SIZE);

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    /* don't quite like this, but I'm unsure I want to invest more in this memory counting */
    EXPECTED_CALL(mock_malloc(0))
        .SetReturn(allocation);
    /* This is the call to the underlying malloc with the size we want to allocate */
    STRICT_EXPECTED_CALL(mock_calloc(1, 1));
    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_HANDLE));

    // act
    result = gballoc_calloc(1, 1);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_ALLOC_PTR1, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_free(result);
    free(allocation);
}

/* Tests_SRS_GBALLOC_01_020: [gballoc_calloc shall call the C99 calloc function and return its result.] */
/* Tests_SRS_GBALLOC_01_021: [If the underlying calloc call is successful, gballoc_calloc shall increment the total memory used with nmemb*size.] */
TEST_FUNCTION(gballoc_calloc_with_1_Item_Of_0_Size_Calls_Underlying_malloc_And_Does_Not_Increase_Max_Used)
{
    // arrange
    void* result;
    void* allocation;
    gballoc_init();
    umock_c_reset_all_calls();
    allocation = malloc(OVERHEAD_SIZE);

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    /* don't quite like this, but I'm unsure I want to invest more in this memory counting */
    EXPECTED_CALL(mock_malloc(0))
        .SetReturn(allocation);
    /* This is the call to the underlying malloc with the size we want to allocate */
    STRICT_EXPECTED_CALL(mock_calloc(1, 0));
    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_HANDLE));

    // act
    result = gballoc_calloc(1, 0);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_ALLOC_PTR1, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_free(result);
    free(allocation);
}

/* Tests_SRS_GBALLOC_01_020: [gballoc_calloc shall call the C99 calloc function and return its result.] */
/* Tests_SRS_GBALLOC_01_021: [If the underlying calloc call is successful, gballoc_calloc shall increment the total memory used with nmemb*size.] */
TEST_FUNCTION(gballoc_calloc_with_0_Items_Of_1_Size_Calls_Underlying_malloc_And_Does_Not_Increase_Max_Used)
{
    // arrange
    void* result;
    void* allocation;
    gballoc_init();
    umock_c_reset_all_calls();
    allocation = malloc(OVERHEAD_SIZE);

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    /* don't quite like this, but I'm unsure I want to invest more in this memory counting */
    EXPECTED_CALL(mock_malloc(0))
        .SetReturn(allocation);
    /* This is the call to the underlying malloc with the size we want to allocate */
    STRICT_EXPECTED_CALL(mock_calloc(0, 1));
    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_HANDLE));

    // act
    result = gballoc_calloc(0, 1);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_ALLOC_PTR1, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_free(result);
    free(allocation);
}

/* Tests_SRS_GBALLOC_01_020: [gballoc_calloc shall call the C99 calloc function and return its result.] */
/* Tests_SRS_GBALLOC_01_021: [If the underlying calloc call is successful, gballoc_calloc shall increment the total memory used with nmemb*size.] */
TEST_FUNCTION(gballoc_calloc_with_42_Items_Of_2_Size_Calls_Underlying_malloc_And_Increases_Max_Size)
{
    // arrange
    void* result;
    void* allocation;
    gballoc_init();
    umock_c_reset_all_calls();
    allocation = malloc(OVERHEAD_SIZE);

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    /* don't quite like this, but I'm unsure I want to invest more in this memory counting */
    EXPECTED_CALL(mock_malloc(0))
        .SetReturn(allocation);
    /* This is the call to the underlying malloc with the size we want to allocate */
    STRICT_EXPECTED_CALL(mock_calloc(42, 2));
    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_HANDLE));

    // act
    result = gballoc_calloc(42, 2);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_ALLOC_PTR1, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_free(result);
    free(allocation);
}

/* Tests_SRS_GBALLOC_01_022: [When the underlying calloc call fails, gballoc_calloc shall return NULL and size should not be counted towards total memory used.] */
TEST_FUNCTION(When_calloc_Fails_Then_gballoc_calloc_fails_too)
{
    // arrange
    void* result;
    void* allocation;
    gballoc_init();
    umock_c_reset_all_calls();
    allocation = malloc(OVERHEAD_SIZE);

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    /* don't quite like this, but I'm unsure I want to invest more in this memory counting */
    EXPECTED_CALL(mock_malloc(0))
        .SetReturn(allocation);
    /* This is the call to the underlying malloc with the size we want to allocate */
    STRICT_EXPECTED_CALL(mock_calloc(1, 1))
        .SetReturn((void*)NULL);
    STRICT_EXPECTED_CALL(mock_free(allocation));
    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_HANDLE));

    // act
    result = gballoc_calloc(1, 1);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    free(allocation);
}

/* Tests_SRS_GBALLOC_01_023: [When gballoc_calloc fails allocating memory for its internal use, gballoc_calloc shall return NULL.] */
TEST_FUNCTION(When_allocating_memory_for_tracking_information_fails_Then_gballoc_calloc_fails_too)
{
    // arrange
    void* result;
    gballoc_init();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    /* don't quite like this, but I'm unsure I want to invest more in this memory counting */
    EXPECTED_CALL(mock_malloc(0))
        .SetReturn((void*)NULL);
    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_HANDLE));

    // act
    result = gballoc_calloc(1, 1);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_GBALLOC_01_040: [If gballoc was not initialized gballoc_calloc shall simply call calloc without any memory tracking being performed.] */
TEST_FUNCTION(gballoc_calloc_after_deinit_calls_crt_calloc)
{
    // arrange
    void* result;
    STRICT_EXPECTED_CALL(mock_calloc(1, 1));

    // act
    result = gballoc_calloc(1, 1);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);
}


/* gballoc_realloc */

/* Tests_SRS_GBALLOC_01_047: [If acquiring the lock fails, gballoc_realloc shall return NULL.]*/
TEST_FUNCTION(when_acquiring_the_lock_fails_gballoc_realloc_fails)
{
    // arrange
    void* result;
    gballoc_init();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE))
        .SetReturn(LOCK_ERROR);

    // act
    result = gballoc_realloc(NULL, 1);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(result);
}

/* Tests_SRS_GBALLOC_01_041: [If gballoc was not initialized gballoc_realloc shall shall simply call realloc without any memory tracking being performed.] */
TEST_FUNCTION(gballoc_realloc_after_deinit_fails)
{
    // arrange
    void* result;
    STRICT_EXPECTED_CALL(mock_realloc(NULL, 1));

    // act
    result = gballoc_realloc(NULL, 1);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);
}

/* Tests_SRS_GBALLOC_01_005: [gballoc_realloc shall call the C99 realloc function and return its result.] */
/* Tests_SRS_GBALLOC_01_017: [When ptr is NULL, gballoc_realloc shall call the underlying realloc with ptr being NULL and the realloc result shall be tracked by gballoc.] */
/* Tests_SRS_GBALLOC_01_032: [gballoc_realloc shall ensure thread safety by using the lock created by gballoc_Init.] */
TEST_FUNCTION(gballoc_realloc_with_NULL_Arg_And_0_Size_Calls_Underlying_realloc)
{
    // arrange
    void* result;
    void* allocation;
    gballoc_init();
    umock_c_reset_all_calls();
    allocation = malloc(OVERHEAD_SIZE);

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    /* don't quite like this, but I'm unsure I want to invest more in this memory counting */
    EXPECTED_CALL(mock_malloc(0))
        .SetReturn(allocation);
    /* This is the call to the underlying malloc with the size we want to allocate */
    STRICT_EXPECTED_CALL(mock_realloc(NULL, 0));
    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_HANDLE));

    // act
    result = gballoc_realloc(NULL, 0);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_ALLOC_PTR1, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_free(result);
    free(allocation);
}

/* Tests_SRS_GBALLOC_01_005: [gballoc_realloc shall call the C99 realloc function and return its result.] */
/* Tests_SRS_GBALLOC_01_017: [When ptr is NULL, gballoc_realloc shall call the underlying realloc with ptr being NULL and the realloc result shall be tracked by gballoc.] */
TEST_FUNCTION(gballoc_realloc_with_NULL_Arg_And_1_Size_Calls_Underlying_realloc)
{
    // arrange
    void* result;
    void* allocation;
    gballoc_init();
    umock_c_reset_all_calls();
    allocation = malloc(OVERHEAD_SIZE);

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    /* don't quite like this, but I'm unsure I want to invest more in this memory counting */
    EXPECTED_CALL(mock_malloc(0))
        .SetReturn(allocation);
    /* This is the call to the underlying malloc with the size we want to allocate */
    STRICT_EXPECTED_CALL(mock_realloc(NULL, 1));
    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_HANDLE));

    // act
    result = gballoc_realloc(NULL, 1);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_ALLOC_PTR1, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_free(result);
    free(allocation);
}

/* Tests_SRS_GBALLOC_01_005: [gballoc_realloc shall call the C99 realloc function and return its result.] */
TEST_FUNCTION(gballoc_realloc_with_Previous_1_Byte_Block_Ptr_And_2_Size_Calls_Underlying_realloc_And_Increases_Max_Used_Memory)
{
    // arrange
    void* result;
    void* allocation;
    gballoc_init();
    umock_c_reset_all_calls();
    allocation = malloc(OVERHEAD_SIZE);

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    /* don't quite like this, but I'm unsure I want to invest more in this memory counting */
    EXPECTED_CALL(mock_malloc(0))
        .SetReturn(allocation);
    /* This is the call to the underlying malloc with the size we want to allocate */
    STRICT_EXPECTED_CALL(mock_realloc(NULL, 1));
    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_HANDLE));
    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    STRICT_EXPECTED_CALL(mock_realloc(TEST_ALLOC_PTR1, 2))
        .SetReturn(TEST_ALLOC_PTR2);
    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_HANDLE));

    result = gballoc_realloc(NULL, 1);

    // act
    result = gballoc_realloc(result, 2);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_ALLOC_PTR2, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_free(result);
    free(allocation);
}

/* Tests_SRS_GBALLOC_01_014: [When the underlying realloc call fails, gballoc_realloc shall return NULL and no change should be made to the counted total memory usage.] */
TEST_FUNCTION(When_realloc_fails_then_gballoc_realloc_Fails_Too_And_No_Change_Is_Made_To_Memory_Counters)
{
    // arrange
    void* result;
    void* allocation;
    gballoc_init();
    umock_c_reset_all_calls();
    allocation = malloc(OVERHEAD_SIZE);

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    /* don't quite like this, but I'm unsure I want to invest more in this memory counting */
    EXPECTED_CALL(mock_malloc(0))
        .SetReturn(allocation);
    /* This is the call to the underlying malloc with the size we want to allocate */
    STRICT_EXPECTED_CALL(mock_realloc(NULL, 1));
    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_HANDLE));
    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    STRICT_EXPECTED_CALL(mock_realloc(TEST_ALLOC_PTR1, 2))
        .SetReturn((void*)NULL);
    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_HANDLE));

    result = gballoc_realloc(NULL, 1);

    // act
    result = gballoc_realloc(result, 2);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_free(TEST_ALLOC_PTR1);
    free(allocation);
}

/* Tests_SRS_GBALLOC_01_015: [When allocating memory used for tracking by gballoc_realloc fails, gballoc_realloc shall return NULL and no change should be made to the counted total memory usage.] */
TEST_FUNCTION(When_Allocating_Memory_For_tracking_fails_gballoc_realloc_fails)
{
    // arrange
    void* result;
    gballoc_init();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    /* don't quite like this, but I'm unsure I want to invest more in this memory counting */
    EXPECTED_CALL(mock_malloc(0))
        .SetReturn((void*)NULL);
    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_HANDLE));

    // act
    result = gballoc_realloc(NULL, 1);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_GBALLOC_01_016: [When the ptr pointer cannot be found in the pointers tracked by gballoc, gballoc_realloc shall return NULL and the underlying realloc shall not be called.] */
TEST_FUNCTION(When_The_Pointer_Is_Not_Tracked_gballoc_realloc_Returns_NULL)
{
    // arrange
    void* result1;
    void* result2;
    void* allocation;
    gballoc_init();
    umock_c_reset_all_calls();
    allocation = malloc(OVERHEAD_SIZE);

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    /* don't quite like this, but I'm unsure I want to invest more in this memory counting */
    EXPECTED_CALL(mock_malloc(0))
        .SetReturn(allocation);
    /* This is the call to the underlying malloc with the size we want to allocate */
    STRICT_EXPECTED_CALL(mock_realloc(NULL, 1));
    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_HANDLE));
    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_HANDLE));

    result1 = gballoc_realloc(NULL, 1);

    // act
    result2 = gballoc_realloc(TEST_REALLOC_PTR, 2);

    // assert
    ASSERT_IS_NULL(result2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_free(result1);
    free(allocation);
}

/* Tests_SRS_GBALLOC_01_014: [When the underlying realloc call fails, gballoc_realloc shall return NULL and no change should be made to the counted total memory usage.] */
TEST_FUNCTION(When_ptr_is_null_and_the_underlying_realloc_fails_then_the_memory_used_for_tracking_is_freed)
{
    // arrange
    void* result;
    void* allocation;
    gballoc_init();
    umock_c_reset_all_calls();
    allocation = malloc(OVERHEAD_SIZE);

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    /* don't quite like this, but I'm unsure I want to invest more in this memory counting */
    EXPECTED_CALL(mock_malloc(0))
        .SetReturn(allocation);
    /* This is the call to the underlying malloc with the size we want to allocate */
    STRICT_EXPECTED_CALL(mock_realloc(NULL, 1))
        .SetReturn((void*)NULL);
    STRICT_EXPECTED_CALL(mock_free(allocation));
    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_HANDLE));

    // act
    result = gballoc_realloc(NULL, 1);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    free(allocation);
}

/* gballoc_free */

/* Tests_SRS_GBALLOC_01_042: [If gballoc was not initialized gballoc_free shall shall simply call free.] */
TEST_FUNCTION(gballoc_free_after_deinit_calls_crt_free)
{
    // arrange
    STRICT_EXPECTED_CALL(mock_free((void*)0x4242));

    // act
    gballoc_free((void*)0x4242);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_GBALLOC_01_049: [If acquiring the lock fails, gballoc_free shall do nothing.] */
TEST_FUNCTION(when_acquiring_the_lock_fails_then_gballoc_free_does_nothing)
{
    // arrange
    void* block;
    void* allocation;
    gballoc_init();
    umock_c_reset_all_calls();
    allocation = malloc(OVERHEAD_SIZE);

    /* don't quite like this, but I'm unsure I want to invest more in this memory counting */
    EXPECTED_CALL(mock_malloc(0))
        .SetReturn(allocation);
    /* This is the call to the underlying malloc with the size we want to allocate */
    STRICT_EXPECTED_CALL(mock_realloc(NULL, 1));
    block = gballoc_realloc(NULL, 1);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE))
        .SetReturn(LOCK_ERROR);

    // act
    gballoc_free(block);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_free(block);
    free(allocation);
}

/* Tests_SRS_GBALLOC_01_008: [gballoc_free shall call the C99 free function.] */
/* Tests_SRS_GBALLOC_01_009: [gballoc_free shall also look up the size associated with the ptr pointer and decrease the total memory used with the associated size amount.] */
/* Tests_SRS_GBALLOC_01_033: [gballoc_free shall ensure thread safety by using the lock created by gballoc_Init.] */
TEST_FUNCTION(gballoc_free_calls_the_underlying_free)
{
    // arrange
    void* block;
    void* allocation;
    gballoc_init();
    umock_c_reset_all_calls();
    allocation = malloc(OVERHEAD_SIZE);

    /* don't quite like this, but I'm unsure I want to invest more in this memory counting */
    EXPECTED_CALL(mock_malloc(0))
        .SetReturn(allocation);
    /* This is the call to the underlying malloc with the size we want to allocate */
    STRICT_EXPECTED_CALL(mock_realloc(NULL, 1));
    block = gballoc_realloc(NULL, 1);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    STRICT_EXPECTED_CALL(mock_free(TEST_ALLOC_PTR1));
    STRICT_EXPECTED_CALL(mock_free(allocation));
    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_HANDLE));

    // act
    gballoc_free(block);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    free(allocation);
}

/* Tests_SRS_GBALLOC_01_008: [gballoc_free shall call the C99 free function.] */
/* Tests_SRS_GBALLOC_01_009: [gballoc_free shall also look up the size associated with the ptr pointer and decrease the total memory used with the associated size amount.] */
TEST_FUNCTION(gballoc_malloc_free_2_times_with_1_byte_yields_1_byte_as_max)
{
    // arrange
    void* block;
    void* allocation;
    gballoc_init();
    umock_c_reset_all_calls();
    allocation = malloc(OVERHEAD_SIZE);

    /* don't quite like this, but I'm unsure I want to invest more in this memory counting */
    EXPECTED_CALL(mock_malloc(0))
        .SetReturn(allocation);
    STRICT_EXPECTED_CALL(mock_realloc(NULL, 1));
    block = gballoc_realloc(NULL, 1);
    gballoc_free(block);

    /* don't quite like this, but I'm unsure I want to invest more in this memory counting */
    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    EXPECTED_CALL(mock_malloc(0))
        .SetReturn(allocation);
    block = gballoc_realloc(NULL, 1);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    STRICT_EXPECTED_CALL(mock_free(TEST_ALLOC_PTR1));
    STRICT_EXPECTED_CALL(mock_free(allocation));
    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_HANDLE));

    // act
    gballoc_free(block);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    free(allocation);
}

/* Tests_SRS_GBALLOC_01_019:[When the ptr pointer cannot be found in the pointers tracked by gballoc, gballoc_free shall not free any memory.] */
TEST_FUNCTION(gballoc_free_with_an_untracked_pointer_does_not_alter_total_memory_used)
{
    // arrange
    void* allocation;
    gballoc_init();
    umock_c_reset_all_calls();
    allocation = malloc(OVERHEAD_SIZE);

    /* don't quite like this, but I'm unsure I want to invest more in this memory counting */
    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    EXPECTED_CALL(mock_malloc(0))
        .SetReturn(allocation);

    gballoc_realloc(NULL, 1);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_HANDLE));

    // act
    gballoc_free(TEST_REALLOC_PTR);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    gballoc_realloc(TEST_ALLOC_PTR1, 2);

    // cleanup
    gballoc_free(TEST_ALLOC_PTR1);
    free(allocation);
}

/* Tests_SRS_GBALLOC_07_001: [ If gballoc was not initialized gballoc_getAllocationCount shall return SIZE_MAX. ] */
TEST_FUNCTION(gballoc_getAllocationCount_without_init_fail)
{
    // arrange
    size_t result;

    // act
    result = gballoc_getAllocationCount();

    // assert
    ASSERT_ARE_EQUAL(size_t, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_GBALLOC_07_003: [ If the lock cannot be acquired, gballoc_getAllocationCount shall return SIZE_MAX. ] */
TEST_FUNCTION(gballoc_getAllocationCount_lock_fail)
{
    // arrange
    size_t result;
    gballoc_init();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE))
        .SetReturn(LOCK_ERROR);

    // act
    result = gballoc_getAllocationCount();

    // assert
    ASSERT_ARE_EQUAL(size_t, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_GBALLOC_07_004: [ gballoc_getAllocationCount shall return the currently number of allocations. ] */
/* Tests_SRS_GBALLOC_07_002: [ gballoc_getAllocationCount shall ensure thread safety by using the lock created by gballoc_Init ] */
TEST_FUNCTION(gballoc_getAllocationCount_success)
{
    // arrange
    void* allocation;
    void* toBeFreed;
    size_t result;
    gballoc_init();
    umock_c_reset_all_calls();

    allocation = malloc(OVERHEAD_SIZE);

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    EXPECTED_CALL(mock_malloc(0))
        .SetReturn(allocation);
    STRICT_EXPECTED_CALL(mock_malloc(1));
    toBeFreed = gballoc_malloc(1);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_HANDLE));

    // act
    result = gballoc_getAllocationCount();

    // assert
    ASSERT_ARE_EQUAL(size_t, 1, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    gballoc_free(toBeFreed);
    free(allocation);
}

TEST_FUNCTION(gballoc_resetMetrics_without_init_fail)
{
    // arrange

    // act
    gballoc_resetMetrics();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_GBALLOC_07_003: [ If the lock cannot be acquired, gballoc_getAllocationCount shall return SIZE_MAX. ] */
TEST_FUNCTION(gballoc_resetMetrics_lock_fail)
{
    // arrange
    gballoc_init();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE))
        .SetReturn(LOCK_ERROR);

    // act
    gballoc_resetMetrics();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_GBALLOC_07_004: [ gballoc_getAllocationCount shall return the currently number of allocations. ] */
/* Tests_SRS_GBALLOC_07_002: [ gballoc_getAllocationCount shall ensure thread safety by using the lock created by gballoc_Init ] */
/* Tests_SRS_GBALLOC_07_006: [ gballoc_resetMetrics shall ensure thread safety by using the lock created by gballoc_Init ]*/
/* Tests_SRS_GBALLOC_07_008: [ gballoc_resetMetrics shall reset the total allocation size, max allocation size and number of allocation to zero. ] */
TEST_FUNCTION(gballoc_resetMetrics_success)
{
    // arrange
    void* allocation;
    void* toBeFreed;
    size_t mem_used;
    size_t alloc_count;
    gballoc_init();
    umock_c_reset_all_calls();

    allocation = malloc(OVERHEAD_SIZE);

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    EXPECTED_CALL(mock_malloc(0))
        .SetReturn(allocation);
    STRICT_EXPECTED_CALL(mock_malloc(1));
    toBeFreed = gballoc_malloc(1);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_HANDLE));

    alloc_count = gballoc_getAllocationCount();
    ASSERT_ARE_EQUAL(size_t, 1, alloc_count);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_HANDLE));
    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_HANDLE));
    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_HANDLE));

    // act
    gballoc_resetMetrics();
    alloc_count = gballoc_getAllocationCount();

    // assert
    ASSERT_ARE_EQUAL(size_t, 0, alloc_count);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    gballoc_free(toBeFreed);
    free(allocation);
}

/* Tests_SRS_GBALLOC_07_007: [** If the lock cannot be acquired, gballoc_reset Metrics shall do nothing.]*/
TEST_FUNCTION(when_lock_cannot_be_acquired_gballoc_resetMetrics_returns)
{
    // arrange
    void* allocation;
    void* toBeFreed;
    size_t alloc_count;
    gballoc_init();
    umock_c_reset_all_calls();

    allocation = malloc(OVERHEAD_SIZE);

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    EXPECTED_CALL(mock_malloc(0))
        .SetReturn(allocation);
    STRICT_EXPECTED_CALL(mock_malloc(1));
    toBeFreed = gballoc_malloc(1);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE));
    STRICT_EXPECTED_CALL(Unlock(TEST_LOCK_HANDLE));

    alloc_count = gballoc_getAllocationCount();
    ASSERT_ARE_EQUAL(size_t, 1, alloc_count);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Lock(TEST_LOCK_HANDLE))
        .SetReturn(LOCK_ERROR);

    // act
    gballoc_resetMetrics();
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    alloc_count = gballoc_getAllocationCount();

    // assert
    ASSERT_ARE_EQUAL(size_t, 1, alloc_count);

    // Cleanup
    gballoc_free(toBeFreed);
    free(allocation);
}

END_TEST_SUITE(GBAlloc_UnitTests)
