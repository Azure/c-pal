// Copyright (c) Microsoft. All rights reserved.

#ifdef __cplusplus
#include <cstddef>
#include <cinttypes>
#else
#include <stddef.h>
#include <inttypes.h>
#endif

#include "azure_macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"
#include "windows.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_windows.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS
#include "azure_c_pal/timer.h"
#undef ENABLE_MOCKS

#include "azure_c_pal/gballoc_hl.h"

#if defined(_MSC_VER)
#pragma warning(disable : 4189)
#endif

static TEST_MUTEX_HANDLE test_serialize_mutex;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

#ifdef __cplusplus
extern "C" {
#endif

MOCK_FUNCTION_WITH_CODE(, HANDLE, mock_HeapCreate, DWORD, flOptions, SIZE_T, dwInitialSize, SIZE_T, dwMaximumSize)
MOCK_FUNCTION_END(HeapCreate(flOptions, dwInitialSize, dwMaximumSize));
MOCK_FUNCTION_WITH_CODE(, BOOL, mock_HeapDestroy, HANDLE, hHeap)
MOCK_FUNCTION_END(HeapDestroy(hHeap));
MOCK_FUNCTION_WITH_CODE(, LPVOID, mock_HeapAlloc, HANDLE, hHeap, DWORD, dwFlags, SIZE_T, dwBytes)
MOCK_FUNCTION_END(HeapAlloc(hHeap, dwFlags, dwBytes));
MOCK_FUNCTION_WITH_CODE(, LPVOID, mock_HeapReAlloc, HANDLE, hHeap, DWORD, dwFlags, LPVOID, lpMem, SIZE_T, dwBytes)
MOCK_FUNCTION_END(HeapReAlloc(hHeap, dwFlags, lpMem, dwBytes));
MOCK_FUNCTION_WITH_CODE(, BOOL, mock_HeapFree, HANDLE, hHeap, DWORD, dwFlags, LPVOID, lpMem)
MOCK_FUNCTION_END(HeapFree(hHeap, dwFlags, lpMem));
MOCK_FUNCTION_WITH_CODE(, SIZE_T, mock_HeapSize, HANDLE, hHeap, DWORD, dwFlags, LPCVOID, lpMem)
MOCK_FUNCTION_END(HeapSize(hHeap, dwFlags, lpMem));

#ifdef __cplusplus
}
#endif

BEGIN_TEST_SUITE(gballoc_win32_heap_unittests)

TEST_SUITE_INITIALIZE(suite_init)
{
    test_serialize_mutex = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(test_serialize_mutex);

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");

    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_windows_register_types(), "umocktypes_windows_register_types");

    REGISTER_UMOCK_ALIAS_TYPE(SIZE_T, size_t);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(test_serialize_mutex);
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    int result;

    if (TEST_MUTEX_ACQUIRE(test_serialize_mutex))
    {
        ASSERT_FAIL("Could not acquire test serialization mutex.");
    }

    umock_c_reset_all_calls();
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result, "umock_c_negative_tests_init");
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    umock_c_negative_tests_deinit();
    TEST_MUTEX_RELEASE(test_serialize_mutex);
}

/* gballoc_hl_init */

/* Tests_SRS_GBALLOC_HL_METRICS_01_001: [ If the module is already initialized, gballoc_hl_init shall fail and return a non-zero value. ]*/
TEST_FUNCTION(gballoc_hl_init_after_init_fails)
{
    // arrange
    int result;
    (void)gballoc_hl_init(NULL, NULL);
    umock_c_reset_all_calls();

    // act
    result = gballoc_hl_init(NULL, NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_002: [ Otherwise, gballoc_hl_init shall call HeapCreate to create a new heap with the initial size and maximum size set to 0. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_003: [ On success, gballoc_hl_init shall return 0. ]*/
TEST_FUNCTION(gballoc_hl_init_succeeds)
{
    // arrange
    int result;

    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0));

    // act
    result = gballoc_hl_init(NULL, NULL);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_004: [ If any error occurs, gballoc_hl_init shall fail and return a non-zero value. ]*/
TEST_FUNCTION(when_HeapCreate_fails_gballoc_hl_init_fails)
{
    // arrange
    int result;

    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .SetReturn(NULL);

    // act
    result = gballoc_hl_init(NULL, NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_deinit();
}

/* gballoc_hl_deinit */

/* Tests_SRS_GBALLOC_HL_METRICS_01_006: [ Otherwise it shall call HeapDestroy to destroy the heap created in gballoc_hl_init. ]*/
TEST_FUNCTION(gballoc_hl_deinit_calls_HeapDestroy)
{
    // arrange
    HANDLE heap_handle;
    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mock_HeapDestroy(heap_handle));

    // act
    gballoc_hl_deinit();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_005: [ If gballoc_hl_deinit is called while not initialized, gballoc_hl_deinit shall return. ]*/
TEST_FUNCTION(gballoc_hl_deinit_when_not_initialized_returns)
{
    // arrange

    // act
    gballoc_hl_deinit();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* gballoc_hl_malloc */

/* Tests_SRS_GBALLOC_HL_METRICS_01_028: [ gballoc_hl_malloc shall call timer_global_get_elapsed_us to obtain the start time of the allocate. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_007: [ gballoc_hl_malloc shall call HeapAlloc for the heap created in gballoc_hl_init, allocating size bytes and return the result of HeapAlloc. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_029: [ gballoc_hl_malloc shall call timer_global_get_elapsed_us to obtain the end time of the allocate. ]*/
TEST_FUNCTION(gballoc_hl_malloc_calls_HeapAlloc_and_returns_the_result)
{
    // arrange
    HANDLE heap_handle;
    void* result;
    LPVOID heap_alloc_result;
    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());
    STRICT_EXPECTED_CALL(mock_HeapAlloc(heap_handle, 0, 42))
        .CaptureReturn(&heap_alloc_result);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());

    // act
    result = gballoc_hl_malloc(42);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(void_ptr, result, heap_alloc_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_free(result);
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_028: [ gballoc_hl_malloc shall call timer_global_get_elapsed_us to obtain the start time of the allocate. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_007: [ gballoc_hl_malloc shall call HeapAlloc for the heap created in gballoc_hl_init, allocating size bytes and return the result of HeapAlloc. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_029: [ gballoc_hl_malloc shall call timer_global_get_elapsed_us to obtain the end time of the allocate. ]*/
TEST_FUNCTION(gballoc_hl_malloc_with_1_byte_calls_HeapAlloc_and_returns_the_result)
{
    // arrange
    HANDLE heap_handle;
    void* result;
    LPVOID heap_alloc_result;
    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());
    STRICT_EXPECTED_CALL(mock_HeapAlloc(heap_handle, 0, 1))
        .CaptureReturn(&heap_alloc_result);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());

    // act
    result = gballoc_hl_malloc(1);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(void_ptr, result, heap_alloc_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_free(result);
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_028: [ gballoc_hl_malloc shall call timer_global_get_elapsed_us to obtain the start time of the allocate. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_007: [ gballoc_hl_malloc shall call HeapAlloc for the heap created in gballoc_hl_init, allocating size bytes and return the result of HeapAlloc. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_029: [ gballoc_hl_malloc shall call timer_global_get_elapsed_us to obtain the end time of the allocate. ]*/
TEST_FUNCTION(gballoc_hl_malloc_with_0_bytes_calls_HeapAlloc_and_returns_the_result)
{
    // arrange
    HANDLE heap_handle;
    void* result;
    LPVOID heap_alloc_result;
    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());
    STRICT_EXPECTED_CALL(mock_HeapAlloc(heap_handle, 0, 0))
        .CaptureReturn(&heap_alloc_result);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());

    // act
    result = gballoc_hl_malloc(0);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(void_ptr, result, heap_alloc_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_free(result);
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_008: [ If the module was not initialized, gballoc_hl_malloc shall return NULL. ]*/
TEST_FUNCTION(gballoc_hl_malloc_when_not_initialized_returns_NULL)
{
    // arrange
    void* result;

    // act
    result = gballoc_hl_malloc(1);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* gballoc_hl_calloc */

/* Tests_SRS_GBALLOC_HL_METRICS_01_030: [ gballoc_hl_calloc shall call timer_global_get_elapsed_us to obtain the start time of the allocate. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_009: [ gballoc_hl_calloc shall call HeapAlloc for the heap created in gballoc_hl_init, allocating size * nmemb bytes. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_031: [ gballoc_hl_calloc shall call timer_global_get_elapsed_us to obtain the end time of the allocate. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_010: [ If HeapAlloc succeeds, gballoc_hl_calloc shall zero the allocated memory and return the pointer to it. ]*/
TEST_FUNCTION(gballoc_hl_calloc_calls_HeapAlloc_clears_and_returns_the_result)
{
    // arrange
    HANDLE heap_handle;
    void* result;
    size_t i;
    LPVOID heap_alloc_result;
    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());
    STRICT_EXPECTED_CALL(mock_HeapAlloc(heap_handle, 0, 42))
        .CaptureReturn(&heap_alloc_result);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());

    // act
    result = gballoc_hl_calloc(1, 42);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(void_ptr, result, heap_alloc_result);
    for (i = 0; i < 42; i++)
    {
        ASSERT_ARE_EQUAL(uint8_t, 0, ((uint8_t*)result)[i]);
    }
    
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_free(result);
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_030: [ gballoc_hl_calloc shall call timer_global_get_elapsed_us to obtain the start time of the allocate. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_009: [ gballoc_hl_calloc shall call HeapAlloc for the heap created in gballoc_hl_init, allocating size * nmemb bytes. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_031: [ gballoc_hl_calloc shall call timer_global_get_elapsed_us to obtain the end time of the allocate. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_010: [ If HeapAlloc succeeds, gballoc_hl_calloc shall zero the allocated memory and return the pointer to it. ]*/
TEST_FUNCTION(gballoc_hl_calloc_with_3_times_4_calls_HeapAlloc_clears_and_returns_the_result)
{
    // arrange
    HANDLE heap_handle;
    void* result;
    size_t i;
    LPVOID heap_alloc_result;
    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());
    STRICT_EXPECTED_CALL(mock_HeapAlloc(heap_handle, 0, 12))
        .CaptureReturn(&heap_alloc_result);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());

    // act
    result = gballoc_hl_calloc(3, 4);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(void_ptr, result, heap_alloc_result);
    for (i = 0; i < 12; i++)
    {
        ASSERT_ARE_EQUAL(uint8_t, 0, ((uint8_t*)result)[i]);
    }

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_free(result);
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_030: [ gballoc_hl_calloc shall call timer_global_get_elapsed_us to obtain the start time of the allocate. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_009: [ gballoc_hl_calloc shall call HeapAlloc for the heap created in gballoc_hl_init, allocating size * nmemb bytes. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_031: [ gballoc_hl_calloc shall call timer_global_get_elapsed_us to obtain the end time of the allocate. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_010: [ If HeapAlloc succeeds, gballoc_hl_calloc shall zero the allocated memory and return the pointer to it. ]*/
TEST_FUNCTION(gballoc_hl_calloc_with_1_byte_calls_HeapAlloc_clears_and_returns_the_result)
{
    // arrange
    HANDLE heap_handle;
    void* result;
    LPVOID heap_alloc_result;
    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());
    STRICT_EXPECTED_CALL(mock_HeapAlloc(heap_handle, 0, 1))
        .CaptureReturn(&heap_alloc_result);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());

    // act
    result = gballoc_hl_calloc(1, 1);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(void_ptr, result, heap_alloc_result);
    ASSERT_ARE_EQUAL(uint8_t, 0, *((uint8_t*)result));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_free(result);
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_030: [ gballoc_hl_calloc shall call timer_global_get_elapsed_us to obtain the start time of the allocate. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_009: [ gballoc_hl_calloc shall call HeapAlloc for the heap created in gballoc_hl_init, allocating size * nmemb bytes. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_031: [ gballoc_hl_calloc shall call timer_global_get_elapsed_us to obtain the end time of the allocate. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_010: [ If HeapAlloc succeeds, gballoc_hl_calloc shall zero the allocated memory and return the pointer to it. ]*/
TEST_FUNCTION(gballoc_hl_calloc_with_0_size_calls_HeapAlloc_and_returns_the_result)
{
    // arrange
    HANDLE heap_handle;
    void* result;
    LPVOID heap_alloc_result;
    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());
    STRICT_EXPECTED_CALL(mock_HeapAlloc(heap_handle, 0, 0))
        .CaptureReturn(&heap_alloc_result);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());

    // act
    result = gballoc_hl_calloc(1, 0);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(void_ptr, result, heap_alloc_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_free(result);
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_030: [ gballoc_hl_calloc shall call timer_global_get_elapsed_us to obtain the start time of the allocate. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_009: [ gballoc_hl_calloc shall call HeapAlloc for the heap created in gballoc_hl_init, allocating size * nmemb bytes. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_031: [ gballoc_hl_calloc shall call timer_global_get_elapsed_us to obtain the end time of the allocate. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_010: [ If HeapAlloc succeeds, gballoc_hl_calloc shall zero the allocated memory and return the pointer to it. ]*/
TEST_FUNCTION(gballoc_hl_calloc_with_0_items_calls_HeapAlloc_and_returns_the_result)
{
    // arrange
    HANDLE heap_handle;
    void* result;
    LPVOID heap_alloc_result;
    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());
    STRICT_EXPECTED_CALL(mock_HeapAlloc(heap_handle, 0, 0))
        .CaptureReturn(&heap_alloc_result);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());

    // act
    result = gballoc_hl_calloc(0, 1);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(void_ptr, result, heap_alloc_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_free(result);
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_011: [ If the module was not initialized, gballoc_hl_calloc shall return NULL. ]*/
TEST_FUNCTION(gballoc_hl_calloc_when_not_initialized_fails)
{
    // arrange
    void* result;

    // act
    result = gballoc_hl_calloc(1, 1);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_012: [ If HeapAlloc fails, gballoc_hl_calloc shall return NULL. ]*/
TEST_FUNCTION(when_HeapAlloc_fails_gballoc_hl_calloc_also_fails)
{
    // arrange
    HANDLE heap_handle;
    void* result;
    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());
    STRICT_EXPECTED_CALL(mock_HeapAlloc(heap_handle, 0, 1))
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());

    // act
    result = gballoc_hl_calloc(1, 1);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_deinit();
}

/* gballoc_hl_realloc */

/* Tests_SRS_GBALLOC_HL_METRICS_01_032: [ gballoc_hl_realloc shall call timer_global_get_elapsed_us to obtain the start time of the allocate. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_013: [ If ptr is NULL, gballoc_hl_realloc shall call HeapAlloc for the heap created in gballoc_hl_init, allocating size bytes and return the result of HeapAlloc. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_033: [ gballoc_hl_realloc shall call timer_global_get_elapsed_us to obtain the end time of the allocate. ]*/
TEST_FUNCTION(gballoc_hl_realloc_with_NULL_calls_HeapAlloc_and_returns_the_result)
{
    // arrange
    HANDLE heap_handle;
    void* result;
    LPVOID heap_alloc_result;
    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());
    STRICT_EXPECTED_CALL(mock_HeapAlloc(heap_handle, 0, 42))
        .CaptureReturn(&heap_alloc_result);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());

    // act
    result = gballoc_hl_realloc(NULL, 42);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(void_ptr, result, heap_alloc_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_free(result);
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_032: [ gballoc_hl_realloc shall call timer_global_get_elapsed_us to obtain the start time of the allocate. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_013: [ If ptr is NULL, gballoc_hl_realloc shall call HeapAlloc for the heap created in gballoc_hl_init, allocating size bytes and return the result of HeapAlloc. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_033: [ gballoc_hl_realloc shall call timer_global_get_elapsed_us to obtain the end time of the allocate. ]*/
TEST_FUNCTION(gballoc_hl_realloc_with_1_byte_size_with_NULL_ptr_calls_HeapAlloc_and_returns_the_result)
{
    // arrange
    HANDLE heap_handle;
    void* result;
    LPVOID heap_alloc_result;
    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());
    STRICT_EXPECTED_CALL(mock_HeapAlloc(heap_handle, 0, 1))
        .CaptureReturn(&heap_alloc_result);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());

    // act
    result = gballoc_hl_realloc(NULL, 1);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(void_ptr, result, heap_alloc_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_free(result);
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_032: [ gballoc_hl_realloc shall call timer_global_get_elapsed_us to obtain the start time of the allocate. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_013: [ If ptr is NULL, gballoc_hl_realloc shall call HeapAlloc for the heap created in gballoc_hl_init, allocating size bytes and return the result of HeapAlloc. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_033: [ gballoc_hl_realloc shall call timer_global_get_elapsed_us to obtain the end time of the allocate. ]*/
TEST_FUNCTION(gballoc_hl_realloc_with_0_bytes_size_with_NULL_ptr_calls_HeapAlloc_and_returns_the_result)
{
    // arrange
    HANDLE heap_handle;
    void* result;
    LPVOID heap_alloc_result;
    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());
    STRICT_EXPECTED_CALL(mock_HeapAlloc(heap_handle, 0, 0))
        .CaptureReturn(&heap_alloc_result);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());

    // act
    result = gballoc_hl_realloc(NULL, 0);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(void_ptr, result, heap_alloc_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_free(result);
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_014: [ If ptr is not NULL, gballoc_hl_realloc shall call HeapReAlloc for the heap created in gballoc_hl_init, passing ptr and size as arguments and return the result of HeapReAlloc. ]*/
TEST_FUNCTION(gballoc_hl_realloc_with_non_NULL_ptr_calls_HeapReAlloc_and_returns_the_result)
{
    // arrange
    HANDLE heap_handle;
    void* result;
    void* ptr;
    LPVOID heap_alloc_result;
    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    ptr = gballoc_hl_malloc(42);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());
    STRICT_EXPECTED_CALL(mock_HeapReAlloc(heap_handle, 0, ptr, 43))
        .CaptureReturn(&heap_alloc_result);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());

    // act
    result = gballoc_hl_realloc(ptr, 43);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(void_ptr, result, heap_alloc_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_free(result);
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_014: [ If ptr is not NULL, gballoc_hl_realloc shall call HeapReAlloc for the heap created in gballoc_hl_init, passing ptr and size as arguments and return the result of HeapReAlloc. ]*/
TEST_FUNCTION(gballoc_hl_realloc_with_non_NULL_ptr_and_1_size_calls_HeapReAlloc_and_returns_the_result)
{
    // arrange
    HANDLE heap_handle;
    void* result;
    void* ptr;
    LPVOID heap_alloc_result;
    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    ptr = gballoc_hl_malloc(42);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());
    STRICT_EXPECTED_CALL(mock_HeapReAlloc(heap_handle, 0, ptr, 1))
        .CaptureReturn(&heap_alloc_result);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());

    // act
    result = gballoc_hl_realloc(ptr, 1);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(void_ptr, result, heap_alloc_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_free(result);
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_014: [ If ptr is not NULL, gballoc_hl_realloc shall call HeapReAlloc for the heap created in gballoc_hl_init, passing ptr and size as arguments and return the result of HeapReAlloc. ]*/
TEST_FUNCTION(gballoc_hl_realloc_with_non_NULL_ptr_and_0_size_calls_HeapReAlloc_and_returns_the_result)
{
    // arrange
    HANDLE heap_handle;
    void* result;
    void* ptr;
    LPVOID heap_alloc_result;
    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    ptr = gballoc_hl_malloc(42);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());
    STRICT_EXPECTED_CALL(mock_HeapReAlloc(heap_handle, 0, ptr, 0))
        .CaptureReturn(&heap_alloc_result);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());

    // act
    result = gballoc_hl_realloc(ptr, 0);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(void_ptr, result, heap_alloc_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_free(result);
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_015: [ If the module was not initialized, gballoc_hl_realloc shall return NULL. ]*/
TEST_FUNCTION(gballoc_hl_realloc_when_not_initialized_fails)
{
    // arrange
    void* result;
    void* ptr;
    (void)gballoc_hl_init(NULL, NULL);
    ptr = gballoc_hl_malloc(42);
    gballoc_hl_free(ptr);
    gballoc_hl_deinit();
    umock_c_reset_all_calls();

    // act
    result = gballoc_hl_realloc(ptr, 1);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_018: [ If any error occurs, gballoc_hl_realloc shall fail and return NULL. ]*/
TEST_FUNCTION(when_HeapAlloc_fails_gballoc_hl_realloc_also_fails)
{
    // arrange
    HANDLE heap_handle;
    void* result;
    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());
    STRICT_EXPECTED_CALL(mock_HeapAlloc(heap_handle, 0, 1))
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());

    // act
    result = gballoc_hl_realloc(NULL, 1);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_018: [ If any error occurs, gballoc_hl_realloc shall fail and return NULL. ]*/
TEST_FUNCTION(when_HeapReAlloc_fails_gballoc_hl_realloc_also_fails)
{
    // arrange
    HANDLE heap_handle;
    void* result;
    void* ptr;
    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    ptr = gballoc_hl_malloc(42);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());
    STRICT_EXPECTED_CALL(mock_HeapReAlloc(heap_handle, 0, ptr, 1))
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());

    // act
    result = gballoc_hl_realloc(ptr, 1);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_free(ptr);
    gballoc_hl_deinit();
}

/* gballoc_hl_free */

/* Tests_SRS_GBALLOC_HL_METRICS_01_034: [ gballoc_hl_free shall call timer_global_get_elapsed_us to obtain the start time of the free. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_019: [ gballoc_hl_free shall call HeapSize to obtain the size of the allocation (used for latency counters). ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_017: [ gballoc_hl_free shall call HeapFree for the heap created in gballoc_hl_init, freeing the memory at ptr. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_035: [ gballoc_hl_free shall call timer_global_get_elapsed_us to obtain the end time of the free. ]*/
TEST_FUNCTION(gballoc_hl_free_on_malloc_block_calls_HeapFree)
{
    // arrange
    HANDLE heap_handle;
    void* ptr;
    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    ptr = gballoc_hl_malloc(42);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());
    STRICT_EXPECTED_CALL(mock_HeapSize(heap_handle, 0, ptr));
    STRICT_EXPECTED_CALL(mock_HeapFree(heap_handle, 0, ptr));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());

    // act
    gballoc_hl_free(ptr);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_034: [ gballoc_hl_free shall call timer_global_get_elapsed_us to obtain the start time of the free. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_019: [ gballoc_hl_free shall call HeapSize to obtain the size of the allocation (used for latency counters). ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_017: [ gballoc_hl_free shall call HeapFree for the heap created in gballoc_hl_init, freeing the memory at ptr. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_035: [ gballoc_hl_free shall call timer_global_get_elapsed_us to obtain the end time of the free. ]*/
TEST_FUNCTION(gballoc_hl_free_on_calloc_block_calls_HeapFree)
{
    // arrange
    HANDLE heap_handle;
    void* ptr;
    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    ptr = gballoc_hl_calloc(3, 4);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());
    STRICT_EXPECTED_CALL(mock_HeapSize(heap_handle, 0, ptr));
    STRICT_EXPECTED_CALL(mock_HeapFree(heap_handle, 0, ptr));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());

    // act
    gballoc_hl_free(ptr);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_034: [ gballoc_hl_free shall call timer_global_get_elapsed_us to obtain the start time of the free. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_019: [ gballoc_hl_free shall call HeapSize to obtain the size of the allocation (used for latency counters). ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_017: [ gballoc_hl_free shall call HeapFree for the heap created in gballoc_hl_init, freeing the memory at ptr. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_035: [ gballoc_hl_free shall call timer_global_get_elapsed_us to obtain the end time of the free. ]*/
TEST_FUNCTION(gballoc_hl_free_on_realloc_block_calls_HeapFree)
{
    // arrange
    HANDLE heap_handle;
    void* ptr;
    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    ptr = gballoc_hl_calloc(3, 4);
    ptr = gballoc_hl_realloc(ptr, 1);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());
    STRICT_EXPECTED_CALL(mock_HeapSize(heap_handle, 0, ptr));
    STRICT_EXPECTED_CALL(mock_HeapFree(heap_handle, 0, ptr));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us());

    // act
    gballoc_hl_free(ptr);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_016: [ If the module was not initialized, gballoc_hl_free shall return. ]*/
TEST_FUNCTION(gballoc_hl_free_when_not_initialized_returns)
{
    // arrange
    HANDLE heap_handle;
    void* ptr;
    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    ptr = gballoc_hl_calloc(3, 4);
    ptr = gballoc_hl_realloc(ptr, 1);
    gballoc_hl_free(ptr);
    gballoc_hl_deinit();
    umock_c_reset_all_calls();

    // act
    gballoc_hl_free(ptr);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_036: [ gballoc_hl_reset_counters shall reset the latency counters for all buckets for the APIs (malloc, calloc, realloc and free). ]*/
TEST_FUNCTION(gballoc_hl_reset_counters_resets_the_counters)
{
    // arrange
    HANDLE heap_handle;
    void* ptr;
    size_t i;

    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    ptr = gballoc_hl_malloc(1);
    gballoc_hl_free(ptr);
    ptr = gballoc_hl_calloc(3, 4);
    ptr = gballoc_hl_realloc(ptr, 1);
    gballoc_hl_free(ptr);
    gballoc_hl_reset_counters();
    umock_c_reset_all_calls();

    GBALLOC_LATENCY_BUCKETS malloc_latency_buckets;
    GBALLOC_LATENCY_BUCKETS calloc_latency_buckets;
    GBALLOC_LATENCY_BUCKETS realloc_latency_buckets;
    GBALLOC_LATENCY_BUCKETS free_latency_buckets;

    // act
    (void)gballoc_hl_get_malloc_latency_buckets(&malloc_latency_buckets);
    (void)gballoc_hl_get_calloc_latency_buckets(&calloc_latency_buckets);
    (void)gballoc_hl_get_realloc_latency_buckets(&realloc_latency_buckets);
    (void)gballoc_hl_get_free_latency_buckets(&free_latency_buckets);

    for (i = 0; i < GBALLOC_LATENCY_BUCKET_COUNT; i++)
    {
        ASSERT_ARE_EQUAL(uint32_t, 0, malloc_latency_buckets.buckets[i].count);
        ASSERT_ARE_EQUAL(uint32_t, 0, calloc_latency_buckets.buckets[i].count);
        ASSERT_ARE_EQUAL(uint32_t, 0, realloc_latency_buckets.buckets[i].count);
        ASSERT_ARE_EQUAL(uint32_t, 0, free_latency_buckets.buckets[i].count);
    }

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* gballoc_hl_get_malloc_latency_buckets */

/* Tests_SRS_GBALLOC_HL_METRICS_01_020: [ If latency_buckets_out is NULL, gballoc_hl_get_malloc_latency_buckets shall fail and return a non-zero value. ]*/
TEST_FUNCTION(gballoc_hl_get_malloc_latency_buckets_with_NULL_latency_buckets_out_fails)
{
    // arrange
    HANDLE heap_handle;
    void* ptr;

    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    ptr = gballoc_hl_malloc(1);
    gballoc_hl_free(ptr);
    umock_c_reset_all_calls();

    // act
    int result = gballoc_hl_get_malloc_latency_buckets(NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_021: [ Otherwise, gballoc_hl_get_malloc_latency_buckets shall copy the latency stats maintained by the module for the malloc API into latency_buckets_out. ]*/
TEST_FUNCTION(gballoc_hl_get_malloc_latency_buckets_with_one_call_returns_the_correct_data)
{
    // arrange
    HANDLE heap_handle;
    void* ptr;

    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
        .SetReturn(1.0);
    STRICT_EXPECTED_CALL(mock_HeapAlloc(heap_handle, 0, IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
        .SetReturn(43.0);
    ptr = gballoc_hl_malloc(1);
    umock_c_reset_all_calls();

    GBALLOC_LATENCY_BUCKETS malloc_latency_buckets;

    // act
    int result = gballoc_hl_get_malloc_latency_buckets(&malloc_latency_buckets);

    ASSERT_ARE_EQUAL(uint32_t, 1, malloc_latency_buckets.buckets[0].count);
    ASSERT_ARE_EQUAL(uint32_t, 42, malloc_latency_buckets.buckets[0].latency_min);
    ASSERT_ARE_EQUAL(uint32_t, 42, malloc_latency_buckets.buckets[0].latency_max);
    ASSERT_ARE_EQUAL(uint32_t, 42, malloc_latency_buckets.buckets[0].latency_avg);

    for (size_t i = 1; i < GBALLOC_LATENCY_BUCKET_COUNT; i++)
    {
        ASSERT_ARE_EQUAL(uint32_t, 0, malloc_latency_buckets.buckets[i].count);
    }

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_free(ptr);

    // cleanup
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_021: [ Otherwise, gballoc_hl_get_malloc_latency_buckets shall copy the latency stats maintained by the module for the malloc API into latency_buckets_out. ]*/
TEST_FUNCTION(gballoc_hl_get_malloc_latency_buckets_with_2_calls_returns_the_average)
{
    // arrange
    HANDLE heap_handle;
    void* ptr1;
    void* ptr2;

    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
        .SetReturn(1.0);
    STRICT_EXPECTED_CALL(mock_HeapAlloc(heap_handle, 0, IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
        .SetReturn(43.0);
    ptr1 = gballoc_hl_malloc(1);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
        .SetReturn(3.0);
    STRICT_EXPECTED_CALL(mock_HeapAlloc(heap_handle, 0, IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
        .SetReturn(5.0);
    ptr2 = gballoc_hl_malloc(1);
    umock_c_reset_all_calls();

    GBALLOC_LATENCY_BUCKETS malloc_latency_buckets;

    // act
    int result = gballoc_hl_get_malloc_latency_buckets(&malloc_latency_buckets);

    ASSERT_ARE_EQUAL(uint32_t, 2, malloc_latency_buckets.buckets[0].count);
    ASSERT_ARE_EQUAL(uint32_t, 2, malloc_latency_buckets.buckets[0].latency_min);
    ASSERT_ARE_EQUAL(uint32_t, 42, malloc_latency_buckets.buckets[0].latency_max);
    ASSERT_ARE_EQUAL(uint32_t, 22, malloc_latency_buckets.buckets[0].latency_avg);

    for (size_t i = 1; i < GBALLOC_LATENCY_BUCKET_COUNT; i++)
    {
        ASSERT_ARE_EQUAL(uint32_t, 0, malloc_latency_buckets.buckets[i].count);
    }

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_free(ptr1);
    gballoc_hl_free(ptr2);

    // cleanup
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_021: [ Otherwise, gballoc_hl_get_malloc_latency_buckets shall copy the latency stats maintained by the module for the malloc API into latency_buckets_out. ]*/
TEST_FUNCTION(gballoc_hl_get_malloc_latency_buckets_with_one_call_in_each_bucket_returns_the_data)
{
    // arrange
    HANDLE heap_handle;
    void* ptr;

    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);

    for (size_t i = 0; i < GBALLOC_LATENCY_BUCKET_COUNT; i++)
    {
        umock_c_reset_all_calls();
        STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
            .SetReturn(1.0);
        STRICT_EXPECTED_CALL(mock_HeapAlloc(heap_handle, 0, IGNORED_ARG));
        STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
            .SetReturn(43.0);
        ptr = gballoc_hl_malloc(((size_t)1 << (9 + i)) - 1);
        gballoc_hl_free(ptr);
    }
    umock_c_reset_all_calls();

    GBALLOC_LATENCY_BUCKETS malloc_latency_buckets;

    // act
    int result = gballoc_hl_get_malloc_latency_buckets(&malloc_latency_buckets);

    for (size_t i = 0; i < GBALLOC_LATENCY_BUCKET_COUNT; i++)
    {
        ASSERT_ARE_EQUAL(uint32_t, 1, malloc_latency_buckets.buckets[i].count);
        ASSERT_ARE_EQUAL(uint32_t, 42, malloc_latency_buckets.buckets[i].latency_min);
        ASSERT_ARE_EQUAL(uint32_t, 42, malloc_latency_buckets.buckets[i].latency_max);
        ASSERT_ARE_EQUAL(uint32_t, 42, malloc_latency_buckets.buckets[i].latency_avg);
    }

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_deinit();
}

/* gballoc_hl_get_calloc_latency_buckets */

/* Tests_SRS_GBALLOC_HL_METRICS_01_022: [ If latency_buckets_out is NULL, gballoc_hl_get_calloc_latency_buckets shall fail and return a non-zero value. ]*/
TEST_FUNCTION(gballoc_hl_get_calloc_latency_buckets_with_NULL_latency_buckets_out_fails)
{
    // arrange
    HANDLE heap_handle;
    void* ptr;

    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    ptr = gballoc_hl_calloc(1, 1);
    gballoc_hl_free(ptr);
    umock_c_reset_all_calls();

    // act
    int result = gballoc_hl_get_calloc_latency_buckets(NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_023: [ Otherwise, gballoc_hl_get_calloc_latency_buckets shall copy the latency stats maintained by the module for the calloc API into latency_buckets_out. ]*/
TEST_FUNCTION(gballoc_hl_get_calloc_latency_buckets_with_one_call_returns_the_correct_data)
{
    // arrange
    HANDLE heap_handle;
    void* ptr;

    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
        .SetReturn(1.0);
    STRICT_EXPECTED_CALL(mock_HeapAlloc(heap_handle, 0, IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
        .SetReturn(43.0);
    ptr = gballoc_hl_calloc(1, 1);
    umock_c_reset_all_calls();

    GBALLOC_LATENCY_BUCKETS calloc_latency_buckets;

    // act
    int result = gballoc_hl_get_calloc_latency_buckets(&calloc_latency_buckets);

    ASSERT_ARE_EQUAL(uint32_t, 1, calloc_latency_buckets.buckets[0].count);
    ASSERT_ARE_EQUAL(uint32_t, 42, calloc_latency_buckets.buckets[0].latency_min);
    ASSERT_ARE_EQUAL(uint32_t, 42, calloc_latency_buckets.buckets[0].latency_max);
    ASSERT_ARE_EQUAL(uint32_t, 42, calloc_latency_buckets.buckets[0].latency_avg);

    for (size_t i = 1; i < GBALLOC_LATENCY_BUCKET_COUNT; i++)
    {
        ASSERT_ARE_EQUAL(uint32_t, 0, calloc_latency_buckets.buckets[i].count);
    }

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_free(ptr);

    // cleanup
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_023: [ Otherwise, gballoc_hl_get_calloc_latency_buckets shall copy the latency stats maintained by the module for the calloc API into latency_buckets_out. ]*/
TEST_FUNCTION(gballoc_hl_get_calloc_latency_buckets_with_2_calls_returns_the_average)
{
    // arrange
    HANDLE heap_handle;
    void* ptr1;
    void* ptr2;

    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
        .SetReturn(1.0);
    STRICT_EXPECTED_CALL(mock_HeapAlloc(heap_handle, 0, IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
        .SetReturn(43.0);
    ptr1 = gballoc_hl_calloc(1, 1);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
        .SetReturn(3.0);
    STRICT_EXPECTED_CALL(mock_HeapAlloc(heap_handle, 0, IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
        .SetReturn(5.0);
    ptr2 = gballoc_hl_calloc(1, 1);
    umock_c_reset_all_calls();

    GBALLOC_LATENCY_BUCKETS calloc_latency_buckets;

    // act
    int result = gballoc_hl_get_calloc_latency_buckets(&calloc_latency_buckets);

    ASSERT_ARE_EQUAL(uint32_t, 2, calloc_latency_buckets.buckets[0].count);
    ASSERT_ARE_EQUAL(uint32_t, 2, calloc_latency_buckets.buckets[0].latency_min);
    ASSERT_ARE_EQUAL(uint32_t, 42, calloc_latency_buckets.buckets[0].latency_max);
    ASSERT_ARE_EQUAL(uint32_t, 22, calloc_latency_buckets.buckets[0].latency_avg);

    for (size_t i = 1; i < GBALLOC_LATENCY_BUCKET_COUNT; i++)
    {
        ASSERT_ARE_EQUAL(uint32_t, 0, calloc_latency_buckets.buckets[i].count);
    }

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_free(ptr1);
    gballoc_hl_free(ptr2);

    // cleanup
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_023: [ Otherwise, gballoc_hl_get_calloc_latency_buckets shall copy the latency stats maintained by the module for the calloc API into latency_buckets_out. ]*/
TEST_FUNCTION(gballoc_hl_get_calloc_latency_buckets_with_one_call_in_each_bucket_returns_the_data)
{
    // arrange
    HANDLE heap_handle;
    void* ptr;

    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);

    for (size_t i = 0; i < GBALLOC_LATENCY_BUCKET_COUNT; i++)
    {
        umock_c_reset_all_calls();
        STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
            .SetReturn(1.0);
        STRICT_EXPECTED_CALL(mock_HeapAlloc(heap_handle, 0, IGNORED_ARG));
        STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
            .SetReturn(43.0);
        ptr = gballoc_hl_calloc(((size_t)1 << (9 + i)) - 1, 1);
        gballoc_hl_free(ptr);
    }
    umock_c_reset_all_calls();

    GBALLOC_LATENCY_BUCKETS calloc_latency_buckets;

    // act
    int result = gballoc_hl_get_calloc_latency_buckets(&calloc_latency_buckets);

    for (size_t i = 0; i < GBALLOC_LATENCY_BUCKET_COUNT; i++)
    {
        ASSERT_ARE_EQUAL(uint32_t, 1, calloc_latency_buckets.buckets[i].count);
        ASSERT_ARE_EQUAL(uint32_t, 42, calloc_latency_buckets.buckets[i].latency_min);
        ASSERT_ARE_EQUAL(uint32_t, 42, calloc_latency_buckets.buckets[i].latency_max);
        ASSERT_ARE_EQUAL(uint32_t, 42, calloc_latency_buckets.buckets[i].latency_avg);
    }

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_deinit();
}

/* gballoc_hl_get_realloc_latency_buckets */

/* Tests_SRS_GBALLOC_HL_METRICS_01_024: [ If latency_buckets_out is NULL, gballoc_hl_get_realloc_latency_buckets shall fail and return a non-zero value. ]*/
TEST_FUNCTION(gballoc_hl_get_realloc_latency_buckets_with_NULL_latency_buckets_out_fails)
{
    // arrange
    HANDLE heap_handle;
    void* ptr;

    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    ptr = gballoc_hl_realloc(NULL, 1);
    gballoc_hl_free(ptr);
    umock_c_reset_all_calls();

    // act
    int result = gballoc_hl_get_realloc_latency_buckets(NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_025: [ Otherwise, gballoc_hl_get_realloc_latency_buckets shall copy the latency stats maintained by the module for the realloc API into latency_buckets_out. ]*/
TEST_FUNCTION(gballoc_hl_get_realloc_latency_buckets_with_one_call_returns_the_correct_data)
{
    // arrange
    HANDLE heap_handle;
    void* ptr;

    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
        .SetReturn(1.0);
    STRICT_EXPECTED_CALL(mock_HeapAlloc(heap_handle, 0, IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
        .SetReturn(43.0);
    ptr = gballoc_hl_realloc(NULL, 1);
    umock_c_reset_all_calls();

    GBALLOC_LATENCY_BUCKETS realloc_latency_buckets;

    // act
    int result = gballoc_hl_get_realloc_latency_buckets(&realloc_latency_buckets);

    ASSERT_ARE_EQUAL(uint32_t, 1, realloc_latency_buckets.buckets[0].count);
    ASSERT_ARE_EQUAL(uint32_t, 42, realloc_latency_buckets.buckets[0].latency_min);
    ASSERT_ARE_EQUAL(uint32_t, 42, realloc_latency_buckets.buckets[0].latency_max);
    ASSERT_ARE_EQUAL(uint32_t, 42, realloc_latency_buckets.buckets[0].latency_avg);

    for (size_t i = 1; i < GBALLOC_LATENCY_BUCKET_COUNT; i++)
    {
        ASSERT_ARE_EQUAL(uint32_t, 0, realloc_latency_buckets.buckets[i].count);
    }

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_free(ptr);

    // cleanup
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_025: [ Otherwise, gballoc_hl_get_realloc_latency_buckets shall copy the latency stats maintained by the module for the realloc API into latency_buckets_out. ]*/
TEST_FUNCTION(gballoc_hl_get_realloc_latency_buckets_with_2_calls_returns_the_average)
{
    // arrange
    HANDLE heap_handle;
    void* ptr1;
    void* ptr2;

    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
        .SetReturn(1.0);
    STRICT_EXPECTED_CALL(mock_HeapAlloc(heap_handle, 0, IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
        .SetReturn(43.0);
    ptr1 = gballoc_hl_realloc(NULL, 1);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
        .SetReturn(3.0);
    STRICT_EXPECTED_CALL(mock_HeapAlloc(heap_handle, 0, IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
        .SetReturn(5.0);
    ptr2 = gballoc_hl_realloc(NULL, 1);
    umock_c_reset_all_calls();

    GBALLOC_LATENCY_BUCKETS realloc_latency_buckets;

    // act
    int result = gballoc_hl_get_realloc_latency_buckets(&realloc_latency_buckets);

    ASSERT_ARE_EQUAL(uint32_t, 2, realloc_latency_buckets.buckets[0].count);
    ASSERT_ARE_EQUAL(uint32_t, 2, realloc_latency_buckets.buckets[0].latency_min);
    ASSERT_ARE_EQUAL(uint32_t, 42, realloc_latency_buckets.buckets[0].latency_max);
    ASSERT_ARE_EQUAL(uint32_t, 22, realloc_latency_buckets.buckets[0].latency_avg);

    for (size_t i = 1; i < GBALLOC_LATENCY_BUCKET_COUNT; i++)
    {
        ASSERT_ARE_EQUAL(uint32_t, 0, realloc_latency_buckets.buckets[i].count);
    }

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_free(ptr1);
    gballoc_hl_free(ptr2);

    // cleanup
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_025: [ Otherwise, gballoc_hl_get_realloc_latency_buckets shall copy the latency stats maintained by the module for the realloc API into latency_buckets_out. ]*/
TEST_FUNCTION(gballoc_hl_get_realloc_latency_buckets_with_one_call_in_each_bucket_returns_the_data)
{
    // arrange
    HANDLE heap_handle;
    void* ptr;

    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);

    for (size_t i = 0; i < GBALLOC_LATENCY_BUCKET_COUNT; i++)
    {
        umock_c_reset_all_calls();
        STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
            .SetReturn(1.0);
        STRICT_EXPECTED_CALL(mock_HeapAlloc(heap_handle, 0, IGNORED_ARG));
        STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
            .SetReturn(43.0);
        ptr = gballoc_hl_realloc(NULL, ((size_t)1 << (9 + i)) - 1);
        gballoc_hl_free(ptr);
    }
    umock_c_reset_all_calls();

    GBALLOC_LATENCY_BUCKETS realloc_latency_buckets;

    // act
    int result = gballoc_hl_get_realloc_latency_buckets(&realloc_latency_buckets);

    for (size_t i = 0; i < GBALLOC_LATENCY_BUCKET_COUNT; i++)
    {
        ASSERT_ARE_EQUAL(uint32_t, 1, realloc_latency_buckets.buckets[i].count);
        ASSERT_ARE_EQUAL(uint32_t, 42, realloc_latency_buckets.buckets[i].latency_min);
        ASSERT_ARE_EQUAL(uint32_t, 42, realloc_latency_buckets.buckets[i].latency_max);
        ASSERT_ARE_EQUAL(uint32_t, 42, realloc_latency_buckets.buckets[i].latency_avg);
    }

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_deinit();
}

/* gballoc_hl_get_free_latency_buckets */

/* Tests_SRS_GBALLOC_HL_METRICS_01_026: [ If latency_buckets_out is NULL, gballoc_hl_get_free_latency_buckets shall fail and return a non-zero value. ]*/
TEST_FUNCTION(gballoc_hl_get_free_latency_buckets_with_NULL_latency_buckets_out_fails)
{
    // arrange
    HANDLE heap_handle;
    void* ptr;

    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    ptr = gballoc_hl_malloc(1);
    gballoc_hl_free(ptr);
    umock_c_reset_all_calls();

    // act
    int result = gballoc_hl_get_free_latency_buckets(NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_027: [ Otherwise, gballoc_hl_get_free_latency_buckets shall copy the latency stats maintained by the module for the free API into latency_buckets_out. ]*/
TEST_FUNCTION(gballoc_hl_get_free_latency_buckets_with_one_call_returns_the_correct_data)
{
    // arrange
    HANDLE heap_handle;
    void* ptr;

    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    ptr = gballoc_hl_malloc(1);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
        .SetReturn(1.0);
    STRICT_EXPECTED_CALL(mock_HeapSize(heap_handle, 0, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mock_HeapFree(heap_handle, 0, IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
        .SetReturn(43.0);
    gballoc_hl_free(ptr);
    umock_c_reset_all_calls();

    GBALLOC_LATENCY_BUCKETS free_latency_buckets;

    // act
    int result = gballoc_hl_get_free_latency_buckets(&free_latency_buckets);

    ASSERT_ARE_EQUAL(uint32_t, 1, free_latency_buckets.buckets[0].count);
    ASSERT_ARE_EQUAL(uint32_t, 42, free_latency_buckets.buckets[0].latency_min);
    ASSERT_ARE_EQUAL(uint32_t, 42, free_latency_buckets.buckets[0].latency_max);
    ASSERT_ARE_EQUAL(uint32_t, 42, free_latency_buckets.buckets[0].latency_avg);

    for (size_t i = 1; i < GBALLOC_LATENCY_BUCKET_COUNT; i++)
    {
        ASSERT_ARE_EQUAL(uint32_t, 0, free_latency_buckets.buckets[i].count);
    }

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_027: [ Otherwise, gballoc_hl_get_free_latency_buckets shall copy the latency stats maintained by the module for the free API into latency_buckets_out. ]*/
TEST_FUNCTION(gballoc_hl_get_free_latency_buckets_with_2_calls_returns_the_average)
{
    // arrange
    HANDLE heap_handle;
    void* ptr1;
    void* ptr2;

    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);
    ptr1 = gballoc_hl_malloc(1);
    ptr2 = gballoc_hl_malloc(1);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
        .SetReturn(1.0);
    STRICT_EXPECTED_CALL(mock_HeapSize(heap_handle, 0, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mock_HeapFree(heap_handle, 0, IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
        .SetReturn(43.0);
    gballoc_hl_free(ptr1);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
        .SetReturn(3.0);
    STRICT_EXPECTED_CALL(mock_HeapSize(heap_handle, 0, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mock_HeapFree(heap_handle, 0, IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
        .SetReturn(5.0);
    gballoc_hl_free(ptr2);
    umock_c_reset_all_calls();

    GBALLOC_LATENCY_BUCKETS free_latency_buckets;

    // act
    int result = gballoc_hl_get_free_latency_buckets(&free_latency_buckets);

    ASSERT_ARE_EQUAL(uint32_t, 2, free_latency_buckets.buckets[0].count);
    ASSERT_ARE_EQUAL(uint32_t, 2, free_latency_buckets.buckets[0].latency_min);
    ASSERT_ARE_EQUAL(uint32_t, 42, free_latency_buckets.buckets[0].latency_max);
    ASSERT_ARE_EQUAL(uint32_t, 22, free_latency_buckets.buckets[0].latency_avg);

    for (size_t i = 1; i < GBALLOC_LATENCY_BUCKET_COUNT; i++)
    {
        ASSERT_ARE_EQUAL(uint32_t, 0, free_latency_buckets.buckets[i].count);
    }

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_027: [ Otherwise, gballoc_hl_get_free_latency_buckets shall copy the latency stats maintained by the module for the free API into latency_buckets_out. ]*/
TEST_FUNCTION(gballoc_hl_get_free_latency_buckets_with_one_call_in_each_bucket_returns_the_data)
{
    // arrange
    HANDLE heap_handle;
    void* ptr;

    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .CaptureReturn(&heap_handle);
    (void)gballoc_hl_init(NULL, NULL);

    for (size_t i = 0; i < GBALLOC_LATENCY_BUCKET_COUNT; i++)
    {
        ptr = gballoc_hl_malloc(((size_t)1 << (9 + i)) - 1);
        umock_c_reset_all_calls();
        STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
            .SetReturn(1.0);
        STRICT_EXPECTED_CALL(mock_HeapSize(heap_handle, 0, IGNORED_ARG));
        STRICT_EXPECTED_CALL(mock_HeapFree(heap_handle, 0, IGNORED_ARG));
        STRICT_EXPECTED_CALL(timer_global_get_elapsed_us())
            .SetReturn(43.0);
        gballoc_hl_free(ptr);
    }
    umock_c_reset_all_calls();

    GBALLOC_LATENCY_BUCKETS free_latency_buckets;

    // act
    int result = gballoc_hl_get_free_latency_buckets(&free_latency_buckets);

    for (size_t i = 0; i < GBALLOC_LATENCY_BUCKET_COUNT; i++)
    {
        ASSERT_ARE_EQUAL(uint32_t, 1, free_latency_buckets.buckets[i].count);
        ASSERT_ARE_EQUAL(uint32_t, 42, free_latency_buckets.buckets[i].latency_min);
        ASSERT_ARE_EQUAL(uint32_t, 42, free_latency_buckets.buckets[i].latency_max);
        ASSERT_ARE_EQUAL(uint32_t, 42, free_latency_buckets.buckets[i].latency_avg);
    }

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_METRICS_01_037: [ gballoc_hl_get_latency_bucket_metadata shall return an array of size GBALLOC_LATENCY_BUCKET_COUNT that contains the metadata for each latency bucket. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_038: [ The first latency bucket shall be [0-511]. ]*/
/* Tests_SRS_GBALLOC_HL_METRICS_01_039: [ Each consecutive bucket shall be [1 << n, (1 << (n + 1)) - 1], where n starts at 8. ]*/
TEST_FUNCTION(gballoc_hl_get_latency_bucket_metadata_returns_the_array_with_the_latency_buckets_metadata)
{
    // arrange

    // act
    const GBALLOC_LATENCY_BUCKET_METADATA* latency_buckets_metadata = gballoc_hl_get_latency_bucket_metadata();

    // assert
    for (size_t i = 0; i < GBALLOC_LATENCY_BUCKET_COUNT; i++)
    {
        ASSERT_IS_NOT_NULL(latency_buckets_metadata[i].bucket_name);
        if (i == 0)
        {
            ASSERT_ARE_EQUAL(uint32_t, 0, latency_buckets_metadata[i].size_range_low);
            ASSERT_ARE_EQUAL(uint32_t, 511, latency_buckets_metadata[i].size_range_high);
        }
        else
        {
            ASSERT_ARE_EQUAL(uint32_t, (uint32_t)1 << (8 + i), latency_buckets_metadata[i].size_range_low);
            ASSERT_ARE_EQUAL(uint32_t, ((uint64_t)1 << (9 + i)) - 1, latency_buckets_metadata[i].size_range_high);
        }
    }

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(gballoc_win32_heap_unittests)
