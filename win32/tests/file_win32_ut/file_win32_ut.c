// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

#include "windows.h"
#include "macro_utils/macro_utils.h"

#include "real_gballoc_ll.h"
static void* real_malloc(size_t size)
{
    return real_gballoc_ll_malloc(size);
}

static void real_free(void* ptr)
{
    real_gballoc_ll_free(ptr);
}

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_windows.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umock_c_negative_tests.h"


#define ENABLE_MOCKS

#include "c_pal/execution_engine.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/execution_engine_win32.h"
#include "mock_file.h"

MOCKABLE_FUNCTION(, void, mock_user_callback, void*, user_context, bool, is_successful);

#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"

#include "c_pal/file.h"

static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

TEST_DEFINE_ENUM_TYPE(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_RESULT)
IMPLEMENT_UMOCK_C_ENUM_TYPE(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_VALUES)

TEST_DEFINE_ENUM_TYPE(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_RESULT)
IMPLEMENT_UMOCK_C_ENUM_TYPE(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_VALUES)

#define FILE_IO_ASYNC_VALUES \
    FILE_WRITE_ASYNC, \
    FILE_READ_ASYNC
MU_DEFINE_ENUM(FILE_IO_ASYNC_TYPE, FILE_IO_ASYNC_VALUES);

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static HANDLE fake_handle = (HANDLE)42;
static PTP_POOL fake_ptp_pool = (PTP_POOL)43;
static PTP_CLEANUP_GROUP fake_ptp_cleanup_group = (PTP_CLEANUP_GROUP)44;
static PTP_IO fake_ptp_io = (PTP_IO)45;
static HANDLE fake_h_event = (HANDLE)46;
static EXECUTION_ENGINE_HANDLE fake_execution_engine = (EXECUTION_ENGINE_HANDLE)47;

static void setup_file_create_expectations(const char* filename, PTP_CALLBACK_ENVIRON* captured_ptpcbe,  PTP_WIN32_IO_CALLBACK* captured_callback)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(execution_engine_inc_ref(fake_execution_engine));
    STRICT_EXPECTED_CALL(mock_CreateFileA(filename, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_FLAG_OVERLAPPED|FILE_FLAG_WRITE_THROUGH, NULL));
    STRICT_EXPECTED_CALL(mock_SetFileCompletionNotificationModes(fake_handle, FILE_SKIP_COMPLETION_PORT_ON_SUCCESS));
    STRICT_EXPECTED_CALL(mock_InitializeThreadpoolEnvironment(IGNORED_ARG))
        .CaptureArgumentValue_pcbe(captured_ptpcbe);
    STRICT_EXPECTED_CALL(execution_engine_win32_get_threadpool(fake_execution_engine))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(mock_SetThreadpoolCallbackPool(IGNORED_ARG, fake_ptp_pool))
        .ValidateArgumentValue_pcbe(captured_ptpcbe);
    STRICT_EXPECTED_CALL(mock_CreateThreadpoolCleanupGroup());
    STRICT_EXPECTED_CALL(mock_SetThreadpoolCallbackCleanupGroup(IGNORED_ARG, fake_ptp_cleanup_group, IGNORED_ARG))
        .ValidateArgumentValue_pcbe(captured_ptpcbe);
    STRICT_EXPECTED_CALL(mock_CreateThreadpoolIo(fake_handle, IGNORED_ARG, NULL, IGNORED_ARG))
        .ValidateArgumentValue_pcbe(captured_ptpcbe)
        .CaptureArgumentValue_pfnio(captured_callback);
}

static FILE_HANDLE get_file_handle_and_callback(const char* filename, PTP_WIN32_IO_CALLBACK* captured_callback)
{
    PTP_CALLBACK_ENVIRON captured_ptpcbe;
    setup_file_create_expectations(filename, &captured_ptpcbe, captured_callback);

    FILE_HANDLE file_handle = file_create(fake_execution_engine, filename, NULL, NULL);

    ASSERT_IS_NOT_NULL(file_handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    umock_c_reset_all_calls();
    return file_handle;
}
static FILE_HANDLE get_file_handle(const char* filename)
{
    PTP_WIN32_IO_CALLBACK captured_callback = NULL;
    return get_file_handle_and_callback(filename, &captured_callback);
}



static FILE_HANDLE start_file_io_async(FILE_IO_ASYNC_TYPE type, unsigned char* buffer, uint32_t size, uint64_t position, FILE_CB user_callback, void* user_context, PTP_WIN32_IO_CALLBACK* captured_callback, LPOVERLAPPED* captured_ov)
{
    FILE_HANDLE file_handle = get_file_handle_and_callback("test_file.txt", captured_callback);

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mock_CreateEvent(IGNORED_ARG, FALSE, FALSE, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mock_StartThreadpoolIo(fake_ptp_io));
    if(type == FILE_WRITE_ASYNC)
    {
        STRICT_EXPECTED_CALL(mock_WriteFile(fake_handle, buffer, size, IGNORED_ARG, IGNORED_ARG))
            .CaptureArgumentValue_lpOverlapped(captured_ov)
            .SetReturn(FALSE);
    }
    else if (type == FILE_READ_ASYNC)
    {
        STRICT_EXPECTED_CALL(mock_ReadFile(fake_handle, buffer, size, IGNORED_ARG, IGNORED_ARG))
            .CaptureArgumentValue_lpOverlapped(captured_ov)
            .SetReturn(FALSE);
    }
    STRICT_EXPECTED_CALL(mock_GetLastError())
        .SetReturn(ERROR_IO_PENDING);

    if(type == FILE_WRITE_ASYNC)
    {
        FILE_WRITE_ASYNC_RESULT result = file_write_async(file_handle, buffer, size, position, user_callback, user_context);
        ASSERT_ARE_EQUAL(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_OK, result);
    }
    else if(type == FILE_READ_ASYNC)
    {
        FILE_READ_ASYNC_RESULT result = file_read_async(file_handle, buffer, size, position, user_callback, user_context);
        ASSERT_ARE_EQUAL(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_OK, result);
    }

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    
    umock_c_reset_all_calls();
    return file_handle;

}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));
    ASSERT_ARE_EQUAL(int, 0, umocktypes_windows_register_types());
    ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types());

    REGISTER_GLOBAL_MOCK_HOOK(malloc, real_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(free, real_free);

    REGISTER_UMOCK_ALIAS_TYPE(PTP_IO, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PTP_CALLBACK_ENVIRON, void*);
    REGISTER_UMOCK_ALIAS_TYPE(EXECUTION_ENGINE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PTP_POOL, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PTP_CLEANUP_GROUP, void*);

    REGISTER_TYPE(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_RESULT);
    REGISTER_TYPE(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_RESULT);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_GLOBAL_MOCK_RETURNS(mock_CreateFileA, fake_handle, INVALID_HANDLE_VALUE);
    REGISTER_GLOBAL_MOCK_RETURNS(mock_SetFileCompletionNotificationModes, TRUE, FALSE);
    REGISTER_GLOBAL_MOCK_RETURNS(execution_engine_win32_get_threadpool, fake_ptp_pool, NULL);
    REGISTER_GLOBAL_MOCK_RETURNS(mock_CreateThreadpoolCleanupGroup, fake_ptp_cleanup_group, NULL);
    REGISTER_GLOBAL_MOCK_RETURNS(mock_CreateThreadpoolIo, fake_ptp_io, NULL);
    REGISTER_GLOBAL_MOCK_RETURNS(mock_CreateEvent, fake_h_event, NULL);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(f)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }

    umock_c_reset_all_calls();
    umock_c_negative_tests_init();
}

TEST_FUNCTION_CLEANUP(cleans)
{
    umock_c_negative_tests_deinit();
    TEST_MUTEX_RELEASE(g_testByTest);
}

/*Tests_SRS_FILE_43_033: [ If execution_engine is NULL, file_create shall fail and return NULL. ]*/
/*Tests_SRS_FILE_WIN32_43_040: [ If execution_engine is NULL, file_create shall fail and return NULL. ]*/
TEST_FUNCTION(file_create_fails_on_null_execution_engine)
{
    ///arrange

    ///act
    FILE_HANDLE return_val = file_create(NULL, "test.txt", NULL, NULL);

    ///assert
    ASSERT_IS_NULL(return_val);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

}

/*Tests_SRS_FILE_43_002: [ If full_file_name is NULL then file_create shall fail and return NULL. ]*/
/*Tests_SRS_FILE_WIN32_43_048: [ If full_file_name is NULL then file_create shall fail and return NULL. ]*/
TEST_FUNCTION(file_create_fails_on_null_full_file_name)
{
    ///arrange
    ///act
    FILE_HANDLE return_val = file_create(fake_execution_engine, NULL, NULL, NULL);
    ///assert
    ASSERT_IS_NULL(return_val);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_FILE_43_037: [ If full_file_name is an empty string, file_create shall fail and return NULL. ]*/
/*Tests_SRS_FILE_WIN32_43_059: [ If full_file_name is an empty string, file_create shall fail and return NULL. ]*/
TEST_FUNCTION(file_create_fails_on_empty_full_file_name)
{
    ///arrange
    ///act
    FILE_HANDLE return_val = file_create(fake_execution_engine, "", NULL, NULL);
    ///assert
    ASSERT_IS_NULL(return_val);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_FILE_WIN32_43_041: [ file_create shall allocate a FILE_HANDLE. ]*/
/*Tests_SRS_FILE_WIN32_01_001: [ file_create shall increment the reference count of execution_engine in order to hold on to it. ]*/
/*Tests_SRS_FILE_WIN32_43_001: [ file_create shall call CreateFileA with full_file_name as lpFileName, GENERIC_READ|GENERIC_WRITE as dwDesiredAccess, FILE_SHARED_READ as dwShareMode, NULL as lpSecurityAttributes, OPEN_ALWAYS as dwCreationDisposition, FILE_FLAG_OVERLAPPED|FILE_FLAG_WRITE_THROUGH as dwFlagsAndAttributes and NULL as hTemplateFile. ]*/
/*Tests_SRS_FILE_WIN32_43_002: [ file_create shall call SetFileCompletionNotificationModes to disable calling the completion port when an async operations finishes synchrounously. ]*/
/*Tests_SRS_FILE_WIN32_43_003: [ file_create shall initialize a threadpool environment by calling InitializeThreadpolEnvironment. ]*/
/*Tests_SRS_FILE_WIN32_43_004: [ file_create shall obtain a PTP_POOL struct by calling execution_engine_win32_get_threadpool on execution_engine. ]*/
/*Tests_SRS_FILE_WIN32_43_005: [ file_create shall register the threadpool environment by calling SetThreadpoolCallbackPool on the initialized threadpool environment and the obtained ptp_pool ]*/
/*Tests_SRS_FILE_WIN32_43_006: [ file_create shall create a cleanup group by calling CreateThreadpoolCleanupGroup. ]*/
/*Tests_SRS_FILE_WIN32_43_007: [ file_create shall register the cleanup group with the threadpool environment by calling SetThreadpoolCallbackCleanupGroup. ]*/
/*Tests_SRS_FILE_WIN32_43_033: [ file_create shall create a threadpool io with the allocated FILE_HANDLE and on_file_io_complete_win32 as a callback by calling CreateThreadpoolIo ]*/
/*Tests_SRS_FILE_WIN32_43_009: [ file_create shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(file_create_succeeds)
{
    ///arrange
    PTP_CALLBACK_ENVIRON captured_ptpcbe;
    PTP_WIN32_IO_CALLBACK captured_callback = NULL;
    char* filename = "file_create_succeeds.txt";
    setup_file_create_expectations(filename, &captured_ptpcbe, &captured_callback);

    ///act
    FILE_HANDLE file_handle = file_create(fake_execution_engine, filename, NULL, NULL);

    ///assert
    ASSERT_IS_NOT_NULL(file_handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    
    ///cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_43_034: [ If there are any failures, file_create shall fail and return NULL. ]*/
/*Tests_SRS_FILE_WIN32_43_008: [ If there are any failures, file_create shall return NULL. ]*/
TEST_FUNCTION(file_create_fails)
{
    ///arrange
    const char filename[] = "file_create_succeeds.txt";
    PTP_CALLBACK_ENVIRON captured_ptpcbe;
    PTP_WIN32_IO_CALLBACK captured_ptp_callback;

    setup_file_create_expectations(filename, &captured_ptpcbe, &captured_ptp_callback);

    umock_c_negative_tests_snapshot();
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); ++i)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            ///act
            FILE_HANDLE file_handle = file_create(fake_execution_engine, filename, NULL, NULL);

            ///assert
            ASSERT_IS_NULL(file_handle);
        }
    }
}

/*Tests_SRS_FILE_43_005: [ If handle is NULL, file_destroy shall return. ]*/
/*Tests_SRS_FILE_WIN32_43_049: [ If handle is NULL, file_destroy shall return. ]*/
TEST_FUNCTION(file_destroy_called_with_null_handle)
{
    ///arrange

    ///act
    file_destroy(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_FILE_WIN32_43_011: [ file_destroy shall wait for all I / O to complete by calling WaitForThreadpoolIoCallbacks.]*/
/*Tests_SRS_FILE_WIN32_43_012: [ file_destroy shall close the cleanup group by calling CloseThreadpoolCleanupGroup.]*/
/*Tests_SRS_FILE_WIN32_43_013: [ file_destroy shall destroy the environment by calling DestroyThreadpoolEnvironment.]*/
/*Tests_SRS_FILE_WIN32_43_016: [ file_destroy shall call CloseHandle on the handle returned by CreateFileA. ]*/
/*Tests_SRS_FILE_WIN32_43_015: [ file_destroy shall close the threadpool IO by calling CloseThreadPoolIo.]*/
/*Tests_SRS_FILE_WIN32_01_002: [ file_destroy shall decrement the reference count for the execution engine. ]*/
/*Tests_SRS_FILE_WIN32_43_042: [ file_destroy shall free the handle.]*/
TEST_FUNCTION(file_destroy_succeeds)
{

    ///arrange
    PTP_CALLBACK_ENVIRON captured_ptpcbe;
    PTP_WIN32_IO_CALLBACK captured_callback = NULL;
    const char* filename = "file_destroy_succeeds.txt";
    setup_file_create_expectations(filename, &captured_ptpcbe, &captured_callback);

    FILE_HANDLE file_handle = file_create(fake_execution_engine, filename, NULL, NULL);

    ASSERT_IS_NOT_NULL(file_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mock_WaitForThreadpoolIoCallbacks(fake_ptp_io, FALSE));
    STRICT_EXPECTED_CALL(mock_CloseThreadpoolCleanupGroup(fake_ptp_cleanup_group));
    STRICT_EXPECTED_CALL(mock_DestroyThreadpoolEnvironment(captured_ptpcbe));
    STRICT_EXPECTED_CALL(mock_CloseHandle(fake_handle));
    STRICT_EXPECTED_CALL(mock_CloseThreadpoolIo(fake_ptp_io));
    STRICT_EXPECTED_CALL(execution_engine_dec_ref(fake_execution_engine));
    STRICT_EXPECTED_CALL(free(file_handle));

    ///act
    file_destroy(file_handle);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_FILE_43_009: [ If handle is NULL then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
/*Tests_SRS_FILE_WIN32_43_043: [ If handle is NULL then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
TEST_FUNCTION(file_write_async_fails_with_null_handle)
{
    ///arrange
    unsigned char source[10];

    ///act
    FILE_WRITE_ASYNC_RESULT result = file_write_async(NULL, source, 1, 1, mock_user_callback, NULL);

    ///assert
    ASSERT_ARE_EQUAL(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_FILE_43_010: [ If source is NULL then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
/*Tests_SRS_FILE_WIN32_43_044: [ If source is NULL then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
TEST_FUNCTION(file_write_async_fails_with_null_source)
{
    ///arrange
    FILE_HANDLE file_handle = get_file_handle("file_write_async_fails_with_null_source.txt");

    ///act
    FILE_WRITE_ASYNC_RESULT result = file_write_async(file_handle, NULL, 1, 1, mock_user_callback, NULL);

    ///assert
    ASSERT_ARE_EQUAL(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_43_012: [ If user_callback is NULL then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
/*Tests_SRS_FILE_WIN32_43_045: [ If user_callback is NULL then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
TEST_FUNCTION(file_write_async_fails_with_null_user_callback)
{
    ///arrange
    FILE_HANDLE file_handle = get_file_handle("file_write_async_fails_with_null_user_callback.txt");
    unsigned char source[10];

    ///act
    FILE_WRITE_ASYNC_RESULT result = file_write_async(file_handle, source, 1, 1, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_43_040: [ If position + size is greater than INT64_MAX, then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
/*Tests_SRS_FILE_WIN32_43_060: [ If position + size is greater than INT64_MAX, then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
TEST_FUNCTION(file_write_async_fails_if_write_overflows_max_size)
{
    ///arrange
    FILE_HANDLE file_handle = get_file_handle("file_write_async_fails_if_write_overflows_max_size.txt");
    unsigned char source[10];

    ///act
    FILE_WRITE_ASYNC_RESULT result = file_write_async(file_handle, source, 1, INT64_MAX, mock_user_callback, NULL);

    ///assert
    ASSERT_ARE_EQUAL(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_43_042: [ If size is 0 then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
/*Tests_SRS_FILE_WIN32_43_061: [ If size is 0 then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
TEST_FUNCTION(file_write_async_fails_if_size_is_zero)
{
    ///arrange
    FILE_HANDLE file_handle = get_file_handle("file_write_async_fails_if_size_is_zero.txt");
    unsigned char source[10];

    ///act
    FILE_WRITE_ASYNC_RESULT result = file_write_async(file_handle, source, 0, INT64_MAX, mock_user_callback, NULL);

    ///assert
    ASSERT_ARE_EQUAL(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_WIN32_43_017: [ file_write_async shall call StartThreadpoolIo.]*/
/*Tests_SRS_FILE_WIN32_43_054: [ file_write_async shall create an event by calling CreateEvent.]*/
/*Tests_SRS_FILE_WIN32_43_020: [ file_write_async shall allocate an OVERLAPPED struct and populate it with the created event and position.]*/
/*Tests_SRS_FILE_WIN32_43_018: [ file_write_async shall allocate a context to store the allocated OVERLAPPED struct, handle, size, user_callback and user_context. ]*/
/*Tests_SRS_FILE_WIN32_43_021: [ file_write_async shall call WriteFile with handle, source, size and the allocated OVERLAPPED struct.]*/
/*Tests_SRS_FILE_WIN32_43_022: [ If WriteFile fails synchronously and GetLastError indicates ERROR_IO_PENDING then file_write_async shall succeed and return FILE_WRITE_ASYNC_OK.]*/
TEST_FUNCTION(file_write_async_succeeds_asynchronously)
{
    ///arrange
    PTP_WIN32_IO_CALLBACK captured_callback;
    LPOVERLAPPED captured_ov;
    FILE_HANDLE file_handle = get_file_handle_and_callback("file_write_async_succeeds_asynchronously.txt", &captured_callback);
    unsigned char source[10];
    uint32_t size = 10;
    uint64_t position = 5;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mock_CreateEvent(IGNORED_ARG, FALSE, FALSE, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mock_StartThreadpoolIo(fake_ptp_io));
    STRICT_EXPECTED_CALL(mock_WriteFile(IGNORED_ARG, source, size, NULL, IGNORED_ARG))
        .CaptureArgumentValue_lpOverlapped(&captured_ov)
        .SetReturn(FALSE);
    STRICT_EXPECTED_CALL(mock_GetLastError())
        .SetReturn(ERROR_IO_PENDING);

    ///act
    FILE_WRITE_ASYNC_RESULT result = file_write_async(file_handle, source, size, position, mock_user_callback, NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_OK, result);
    
    ///cleanup
    captured_callback(NULL, NULL, captured_ov, NO_ERROR, sizeof(source), NULL);
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_43_035: [ If the call to write the file fails, file_write_async shall fail and return FILE_WRITE_ASYNC_WRITE_ERROR. ]*/
/*Tests_SRS_FILE_WIN32_43_017: [ file_write_async shall call StartThreadpoolIo.]*/
/*Tests_SRS_FILE_WIN32_43_054: [ file_write_async shall create an event by calling CreateEvent.]*/
/*Tests_SRS_FILE_WIN32_43_020: [ file_write_async shall allocate an OVERLAPPED struct and populate it with the created event and position.]*/
/*Tests_SRS_FILE_WIN32_43_018: [ file_write_async shall allocate a context to store the allocated OVERLAPPED struct, handle, size, user_callback and user_context. ]*/
/*Tests_SRS_FILE_WIN32_43_021: [ file_write_async shall call WriteFile with handle, source, size and the allocated OVERLAPPED struct.]*/
/*Tests_SRS_FILE_WIN32_43_023: [ If WriteFile fails synchronously and GetLastError does not indicate ERROR_IO_PENDING then file_write_async shall fail, call CancelThreadpoolIo and return FILE_WRITE_ASYNC_WRITE_ERROR. ]*/
TEST_FUNCTION(file_write_async_fails_synchronously)
{
    ///arrange
    FILE_HANDLE file_handle = get_file_handle("file_write_async_fails_synchronously.txt");
    unsigned char source[10];
    uint32_t size = 10;
    uint64_t position = 5;
    void* io;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .CaptureReturn(&io);
    STRICT_EXPECTED_CALL(mock_CreateEvent(IGNORED_ARG, FALSE, FALSE, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mock_StartThreadpoolIo(fake_ptp_io));
    STRICT_EXPECTED_CALL(mock_WriteFile(fake_handle, source, size, NULL, IGNORED_ARG))
        .SetReturn(FALSE);
    STRICT_EXPECTED_CALL(mock_GetLastError())
        .SetReturn(ERROR_IO_INCOMPLETE);
    STRICT_EXPECTED_CALL(mock_CancelThreadpoolIo(fake_ptp_io));
    STRICT_EXPECTED_CALL(mock_CloseHandle(fake_h_event));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&io);
    ///act
    FILE_WRITE_ASYNC_RESULT result = file_write_async(file_handle, source, size, position, mock_user_callback, NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_WRITE_ERROR, result);
    
    ///cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_WIN32_43_017: [ file_write_async shall call StartThreadpoolIo.]*/
/*Tests_SRS_FILE_WIN32_43_054: [ file_write_async shall create an event by calling CreateEvent.]*/
/*Tests_SRS_FILE_WIN32_43_020: [ file_write_async shall allocate an OVERLAPPED struct and populate it with the created event and position.]*/
/*Tests_SRS_FILE_WIN32_43_018: [ file_write_async shall allocate a context to store the allocated OVERLAPPED struct, handle, size, user_callback and user_context. ]*/
/*Tests_SRS_FILE_WIN32_43_021: [ file_write_async shall call WriteFile with handle, source, size and the allocated OVERLAPPED struct.]*/
/*Tests_SRS_FILE_WIN32_43_024: [ If WriteFile succeeds synchronously then file_write_async shall succeed, call CancelThreadpoolIo, call user_callback and return FILE_WRITE_ASYNC_OK. ]*/
TEST_FUNCTION(file_write_async_succeeds_synchronously)
{
    ///arrange
    FILE_HANDLE file_handle = get_file_handle("file_write_async_succeeds_synchronously.txt");
    unsigned char source[10];
    uint32_t size = 10;
    uint64_t position = 5;
    void* user_context = (void*)45;
    void* io;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .CaptureReturn(&io);
    STRICT_EXPECTED_CALL(mock_CreateEvent(IGNORED_ARG, FALSE, FALSE, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mock_StartThreadpoolIo(fake_ptp_io));
    STRICT_EXPECTED_CALL(mock_WriteFile(fake_handle, source, size, NULL, IGNORED_ARG))
        .SetReturn(TRUE);
    STRICT_EXPECTED_CALL(mock_CancelThreadpoolIo(fake_ptp_io));
    STRICT_EXPECTED_CALL(mock_user_callback(user_context, true));
    STRICT_EXPECTED_CALL(mock_CloseHandle(fake_h_event));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&io);
    
    ///act
    FILE_WRITE_ASYNC_RESULT result = file_write_async(file_handle, source, size, position, mock_user_callback, user_context);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_OK, result);
    
    ///cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_43_015: [ If there are any failures, file_write_async shall fail and return FILE_WRITE_ASYNC_ERROR. ]*/
/*Tests_SRS_FILE_WIN32_43_057: [ If there are any other failures, file_write_async shall fail and return FILE_WRITE_ASYNC_ERROR. ]*/
TEST_FUNCTION(file_write_async_fails)
{
    ///arrange
    FILE_HANDLE file_handle = get_file_handle("file_write_async_fails.txt");
    unsigned char source[10];
    uint32_t size = 10;
    uint64_t position = 5;
    void* io;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .CaptureReturn(&io);
    STRICT_EXPECTED_CALL(mock_CreateEvent(IGNORED_ARG, FALSE, FALSE, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mock_StartThreadpoolIo(fake_ptp_io));
    STRICT_EXPECTED_CALL(mock_WriteFile(IGNORED_ARG, source, size, NULL, IGNORED_ARG))
        .SetReturn(FALSE)
        .CallCannotFail();
    STRICT_EXPECTED_CALL(mock_GetLastError())
        .SetReturn(ERROR_IO_PENDING)
        .CallCannotFail();
    STRICT_EXPECTED_CALL(mock_CancelThreadpoolIo(fake_ptp_io));
    STRICT_EXPECTED_CALL(mock_CloseHandle(fake_h_event))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&io);

    umock_c_negative_tests_snapshot();
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); ++i)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            ///act
            FILE_WRITE_ASYNC_RESULT result = file_write_async(file_handle, source, size, position, mock_user_callback, NULL);

            ///assert
            ASSERT_ARE_EQUAL(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_ERROR, result);
        }
    }

    ///cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_43_017: [ If handle is NULL then file_read_async shall fail and return FILE_READ_ASYNC_INVALID_ARGS. ]*/
/*Tests_SRS_FILE_WIN32_43_046: [ If handle is NULL then file_read_async shall fail and return FILE_READ_ASYNC_INVALID_ARGS. ]*/
TEST_FUNCTION(file_read_async_fails_with_null_handle)
{
    ///arrange
    unsigned char destination[10];

    ///act
    FILE_READ_ASYNC_RESULT result = file_read_async(NULL, destination, 1, 1, mock_user_callback, NULL);

    ///assert
    ASSERT_ARE_EQUAL(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_INVALID_ARGS, result);
}

/*Tests_SRS_FILE_43_032: [ If destination is NULL then file_read_async shall fail and return FILE_READ_ASYNC_INVALID_ARGS. ]*/
/*Tests_SRS_FILE_WIN32_43_051: [ If destination is NULL then file_read_async shall fail and return FILE_READ_ASYNC_INVALID_ARGS. ]*/
TEST_FUNCTION(file_read_async_fails_with_null_destination)
{
    ///arrange
    FILE_HANDLE file_handle = get_file_handle("file_read_async_fails_with_null_destination.txt");

    ///act
    FILE_READ_ASYNC_RESULT result = file_read_async(file_handle, NULL, 1, 1, mock_user_callback, NULL);

    ///assert
    ASSERT_ARE_EQUAL(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    
    ///cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_43_020: [ If user_callback is NULL then file_read_async shall fail and return FILE_READ_ASYNC_INVALID_ARGS. ]*/
/*Tests_SRS_FILE_WIN32_43_047: [ If user_callback is NULL then file_read_async shall fail and return FILE_READ_ASYNC_INVALID_ARGS. ]*/
TEST_FUNCTION(file_read_async_fails_with_null_user_callback)
{
    ///arrange
    FILE_HANDLE file_handle = get_file_handle("file_read_async_fails_with_null_user_callback.txt");
    unsigned char destination[10];

    ///act
    FILE_READ_ASYNC_RESULT result = file_read_async(file_handle, destination, 1, 1, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    
    ///cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_43_043: [ If size is 0 then file_read_async shall fail and return FILE_READ_ASYNC_INVALID_ARGS. ]*/
/*Tests_SRS_FILE_WIN32_43_062: [ If size is 0 then file_read_async shall fail and return FILE_READ_ASYNC_INVALID_ARGS. ]*/
TEST_FUNCTION(file_read_async_fails_if_size_is_zero)
{
    ///arrange
    FILE_HANDLE file_handle = get_file_handle("file_read_async_fails_if_size_is_zero.txt");
    unsigned char source[10];

    ///act
    FILE_READ_ASYNC_RESULT result = file_read_async(file_handle, source, 0, INT64_MAX, mock_user_callback, NULL);

    ///assert
    ASSERT_ARE_EQUAL(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    
    ///cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_WIN32_43_025: [ file_read_async shall call StartThreadpoolIo.]*/
/*Tests_SRS_FILE_WIN32_43_055: [ file_read_async shall create an event by calling CreateEvent.]*/
/*Tests_SRS_FILE_WIN32_43_028: [ file_read_async shall allocate an OVERLAPPED struct and populate it with the created event and position.]*/
/*Tests_SRS_FILE_WIN32_43_026: [ file_read_async shall allocate a context to store the allocated OVERLAPPED struct, destination, handle, size, user_callback and user_context ]*/
/*Tests_SRS_FILE_WIN32_43_029: [ file_read_async shall call ReadFile with handle, destination, size and the allocated OVERLAPPED struct.]*/
/*Tests_SRS_FILE_WIN32_43_030: [ If ReadFile fails synchronously and GetLastError indicates ERROR_IO_PENDING then file_read_async shall succeed and return FILE_READ_ASYNC_OK.]*/
TEST_FUNCTION(file_read_async_succeeds_asynchronously)
{
    ///arrange
    unsigned char source[10];
    PTP_WIN32_IO_CALLBACK captured_callback;
    LPOVERLAPPED captured_ov;
    FILE_HANDLE file_handle = get_file_handle_and_callback("file_read_async_succeeds_asynchronously.txt", &captured_callback);
    unsigned char destination[10];
    uint64_t position = 5;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mock_CreateEvent(IGNORED_ARG, FALSE, FALSE, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mock_StartThreadpoolIo(fake_ptp_io));
    STRICT_EXPECTED_CALL(mock_ReadFile(fake_handle, destination, sizeof(destination), NULL, IGNORED_ARG))
        .CaptureArgumentValue_lpOverlapped(&captured_ov)
        .SetReturn(FALSE);
    STRICT_EXPECTED_CALL(mock_GetLastError())
        .SetReturn(ERROR_IO_PENDING);

    ///act
    FILE_READ_ASYNC_RESULT result = file_read_async(file_handle, destination, sizeof(destination), position, mock_user_callback, NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_OK, result);
    
    ///cleanup
    captured_callback(NULL, NULL, captured_ov, NO_ERROR, sizeof(source), NULL);
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_43_036: [ If the call to read the file fails, file_read_async shall fail and return FILE_READ_ASYNC_READ_ERROR. ]*/
/*Tests_SRS_FILE_WIN32_43_025: [ file_read_async shall call StartThreadpoolIo.]*/
/*Tests_SRS_FILE_WIN32_43_055: [ file_read_async shall create an event by calling CreateEvent.]*/
/*Tests_SRS_FILE_WIN32_43_028: [ file_read_async shall allocate an OVERLAPPED struct and populate it with the created event and position.]*/
/*Tests_SRS_FILE_WIN32_43_026: [ file_read_async shall allocate a context to store the allocated OVERLAPPED struct, destination, handle, size, user_callback and user_context ]*/
/*Tests_SRS_FILE_WIN32_43_029: [ file_read_async shall call ReadFile with handle, destination, size and the allocated OVERLAPPED struct.]*/
/*Tests_SRS_FILE_WIN32_43_031: [ If ReadFile fails synchronously and GetLastError does not indicate ERROR_IO_PENDING then file_read_async shall fail, call CancelThreadpoolIo and return FILE_READ_ASYNC_READ_ERROR. ]*/
TEST_FUNCTION(file_read_async_fails_synchronously)
{
    ///arrange
    FILE_HANDLE file_handle = get_file_handle("file_read_async_fails_synchronously.txt");
    unsigned char destination[10];
    uint64_t position = 5;
    void* io;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .CaptureReturn(&io);
    STRICT_EXPECTED_CALL(mock_CreateEvent(IGNORED_ARG, FALSE, FALSE, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mock_StartThreadpoolIo(fake_ptp_io));
    STRICT_EXPECTED_CALL(mock_ReadFile(IGNORED_ARG, destination, sizeof(destination), NULL, IGNORED_ARG))
        .SetReturn(FALSE);
    STRICT_EXPECTED_CALL(mock_GetLastError())
        .SetReturn(ERROR_IO_INCOMPLETE);
    STRICT_EXPECTED_CALL(mock_CancelThreadpoolIo(fake_ptp_io));
    STRICT_EXPECTED_CALL(mock_CloseHandle(fake_h_event));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&io);

    ///act
    FILE_READ_ASYNC_RESULT result = file_read_async(file_handle, destination, sizeof(destination), position, mock_user_callback, NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_READ_ERROR, result);
    
    ///cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_WIN32_43_025: [ file_read_async shall call StartThreadpoolIo.]*/
/*Tests_SRS_FILE_WIN32_43_055: [ file_read_async shall create an event by calling CreateEvent.]*/
/*Tests_SRS_FILE_WIN32_43_028: [ file_read_async shall allocate an OVERLAPPED struct and populate it with the created event and position.]*/
/*Tests_SRS_FILE_WIN32_43_026: [ file_read_async shall allocate a context to store the allocated OVERLAPPED struct, destination, handle, size, user_callback and user_context ]*/
/*Tests_SRS_FILE_WIN32_43_029: [ file_read_async shall call ReadFile with handle, destination, size and the allocated OVERLAPPED struct.]*/
/*Tests_SRS_FILE_WIN32_43_032: [ If ReadFile succeeds synchronously then file_read_async shall succeed, call CancelThreadpoolIo, call user_callback and return FILE_READ_ASYNC_OK. ]*/
TEST_FUNCTION(file_read_async_succeeds_synchronously)
{
    ///arrange
    FILE_HANDLE file_handle = get_file_handle("file_read_async_succeeds_synchronously.txt");
    unsigned char destination[10];
    uint64_t position = 5;
    void* user_context = (void*)45;
    void* io;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .CaptureReturn(&io);
    STRICT_EXPECTED_CALL(mock_CreateEvent(IGNORED_ARG, FALSE, FALSE, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mock_StartThreadpoolIo(fake_ptp_io));
    STRICT_EXPECTED_CALL(mock_ReadFile(fake_handle, destination, sizeof(destination), NULL, IGNORED_ARG))
        .SetReturn(TRUE);
    STRICT_EXPECTED_CALL(mock_CancelThreadpoolIo(fake_ptp_io));
    STRICT_EXPECTED_CALL(mock_user_callback(user_context, true));
    STRICT_EXPECTED_CALL(mock_CloseHandle(fake_h_event));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&io);

    ///act
    FILE_READ_ASYNC_RESULT result = file_read_async(file_handle, destination, sizeof(destination), position, mock_user_callback, user_context);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_OK, result);

    ///cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_43_022: [ If there are any failures then file_read_async shall fail and return FILE_READ_ASYNC_ERROR. ]*/
/*Tests_SRS_FILE_WIN32_43_058: [ If there are any other failures, file_read_async shall fail and return FILE_READ_ASYNC_ERROR. ]*/
TEST_FUNCTION(file_read_async_fails)
{
    ///arrange
    FILE_HANDLE file_handle = get_file_handle("file_read_async_fails.txt");
    unsigned char destination[10];
    uint64_t position = 5;
    void* io;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .CaptureReturn(&io);
    STRICT_EXPECTED_CALL(mock_CreateEvent(IGNORED_ARG, FALSE, FALSE, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mock_StartThreadpoolIo(fake_ptp_io));
    STRICT_EXPECTED_CALL(mock_ReadFile(IGNORED_ARG, destination, sizeof(destination), NULL, IGNORED_ARG))
        .SetReturn(FALSE)
        .CallCannotFail();
    STRICT_EXPECTED_CALL(mock_GetLastError())
        .SetReturn(ERROR_IO_PENDING)
        .CallCannotFail();
    STRICT_EXPECTED_CALL(mock_CancelThreadpoolIo(fake_ptp_io));
    STRICT_EXPECTED_CALL(mock_CloseHandle(fake_h_event))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&io);

    umock_c_negative_tests_snapshot();
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); ++i)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            ///act
            FILE_READ_ASYNC_RESULT result = file_read_async(file_handle, destination, sizeof(destination), position, mock_user_callback, NULL);

            ///assert
            ASSERT_ARE_EQUAL(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_ERROR, result);
        }
    }

    ///cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_WIN32_43_050: [ file_extend shall return 0. ]*/
TEST_FUNCTION(file_extend_returns_zero)
{
    ///arrange
    FILE_HANDLE file_handle = get_file_handle("file_extend_returns_zero.txt");
    ///act
    int return_val = file_extend(file_handle, 0);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, return_val);

    ///cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_WIN32_43_034: [ on_file_io_complete_win32 shall recover the file handle, the number of bytes requested by the user, user_callback and user_context from the context containing overlapped. ]*/
/*Tests_SRS_FILE_WIN32_43_066: [ on_file_io_complete_win32 shall call user_callback with is_successful as true if and only if io_result is equal to NO_ERROR and number_of_bytes_transferred is equal to the number of bytes requested by the user. ]*/
TEST_FUNCTION(on_file_io_complete_win32_calls_callback_successfully_for_write)
{
    ///arrange
    unsigned char source[10];
    uint64_t position = 11;
    void* user_context = (void*)20;

    PTP_WIN32_IO_CALLBACK captured_callback = NULL;
    LPOVERLAPPED captured_ov;

    FILE_HANDLE file_handle = start_file_io_async(FILE_WRITE_ASYNC, source, sizeof(source), position, mock_user_callback, user_context,  &captured_callback, &captured_ov);

    STRICT_EXPECTED_CALL(mock_CloseHandle(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mock_user_callback(user_context, true));
    ASSERT_ARE_EQUAL(uint64_t, position, captured_ov->Offset);

    ///act
    captured_callback(NULL, NULL, captured_ov, NO_ERROR, sizeof(source), NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_WIN32_43_034: [ on_file_io_complete_win32 shall recover the file handle, the number of bytes requested by the user, user_callback and user_context from the context containing overlapped. ]*/
/*Tests_SRS_FILE_WIN32_43_068: [ If either io_result is not equal to NO_ERROR or number_of_bytes_transferred is not equal to the bytes requested by the user, on_file_io_complete_win32 shall return false. ]*/
TEST_FUNCTION(on_file_io_complete_win32_calls_callback_unsuccessfully_for_write_because_io_failed)
{
    ///arrange
    unsigned char source[10];
    uint64_t position = 11;
    void* user_context = (void*)20;

    PTP_WIN32_IO_CALLBACK captured_callback = NULL;
    LPOVERLAPPED captured_ov;

    FILE_HANDLE file_handle = start_file_io_async(FILE_WRITE_ASYNC, source, sizeof(source), position, mock_user_callback, user_context,  &captured_callback, &captured_ov);

    STRICT_EXPECTED_CALL(mock_CloseHandle(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    STRICT_EXPECTED_CALL(mock_user_callback(user_context, false));
    ASSERT_ARE_EQUAL(uint64_t, position, captured_ov->Offset);

    ///act
    captured_callback(NULL, NULL, captured_ov, ERROR_IO_INCOMPLETE, sizeof(source), NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_WIN32_43_034: [on_file_io_complete_win32 shall recover the file handle, the number of bytes requested by the user, user_callback and user_context from the context containing overlapped.]*/
/*Tests_SRS_FILE_WIN32_43_068 : [If either GetOverlappedResult returns false or number_of_bytes_transferred is not equal to the bytes requested by the user, on_file_io_complete_win32 shall return false.]*/
TEST_FUNCTION(on_file_io_complete_win32_calls_callback_unsuccessfully_for_write_because_num_bytes_is_less)
{
    ///arrange
    unsigned char source[10];
    uint64_t position = 11;
    void* user_context = (void*)20;

    PTP_WIN32_IO_CALLBACK captured_callback = NULL;
    LPOVERLAPPED captured_ov;

    FILE_HANDLE file_handle = start_file_io_async(FILE_WRITE_ASYNC, source, sizeof(source), position, mock_user_callback, user_context,  &captured_callback, &captured_ov);

    STRICT_EXPECTED_CALL(mock_CloseHandle(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mock_user_callback(user_context, false));
    ASSERT_ARE_EQUAL(uint64_t, position, captured_ov->Offset);

    ///act
    captured_callback(NULL, NULL, captured_ov, NO_ERROR, sizeof(source)-1, NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_WIN32_43_034: [ on_file_io_complete_win32 shall recover the file handle, the number of bytes requested by the user, user_callback and user_context from the context containing overlapped. ]*/
/*Tests_SRS_FILE_WIN32_43_066: [ on_file_io_complete_win32 shall call user_callback with is_successful as true if and only if io_result is equal to NO_ERROR and number_of_bytes_transferred is equal to the number of bytes requested by the user. ]*/
TEST_FUNCTION(on_file_io_complete_win32_calls_callback_successfully_for_read)
{
    ///arrange
    unsigned char destination[10];
    uint64_t position = 11;
    void* user_context = (void*)20;

    PTP_WIN32_IO_CALLBACK captured_callback = NULL;
    LPOVERLAPPED captured_ov;

    FILE_HANDLE file_handle = start_file_io_async(FILE_READ_ASYNC, destination, sizeof(destination), position, mock_user_callback, user_context,  &captured_callback, &captured_ov);

    STRICT_EXPECTED_CALL(mock_CloseHandle(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mock_user_callback(user_context, true));
    ASSERT_ARE_EQUAL(uint64_t, position, captured_ov->Offset);

    ///act
    captured_callback(NULL, NULL, captured_ov, NO_ERROR, sizeof(destination), NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_WIN32_43_034: [ on_file_io_complete_win32 shall recover the file handle, the number of bytes requested by the user, user_callback and user_context from the context containing overlapped. ]*/
/*Tests_SRS_FILE_WIN32_43_068: [ If either io_result is not equal to NO_ERROR or number_of_bytes_transferred is not equal to the bytes requested by the user, on_file_io_complete_win32 shall return false. ]*/
TEST_FUNCTION(on_file_io_complete_win32_calls_callback_unsuccessfully_for_read_because_io_failed)
{
    ///arrange
    unsigned char destination[10];
    uint64_t position = 11;
    void* user_context = (void*)20;

    PTP_WIN32_IO_CALLBACK captured_callback = NULL;
    LPOVERLAPPED captured_ov;

    FILE_HANDLE file_handle = start_file_io_async(FILE_READ_ASYNC, destination, sizeof(destination), position, mock_user_callback, user_context,  &captured_callback, &captured_ov);

    STRICT_EXPECTED_CALL(mock_CloseHandle(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mock_user_callback(user_context, false));
    ASSERT_ARE_EQUAL(uint64_t, position, captured_ov->Offset);

    ///act
    captured_callback(NULL, NULL, captured_ov, ERROR_IO_INCOMPLETE, sizeof(destination), NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_WIN32_43_034: [on_file_io_complete_win32 shall recover the file handle, the number of bytes requested by the user, user_callback and user_context from the context containing overlapped.]*/
/*Tests_SRS_FILE_WIN32_43_068 : [If either GetOverlappedResult returns false or number_of_bytes_transferred is not equal to the bytes requested by the user, on_file_io_complete_win32 shall return false.]*/
TEST_FUNCTION(on_file_io_complete_win32_calls_callback_unsuccessfully_for_read_because_num_bytes_is_less)
{
    ///arrange
    unsigned char destination[10];
    uint64_t position = 11;
    void* user_context = (void*)20;

    PTP_WIN32_IO_CALLBACK captured_callback = NULL;
    LPOVERLAPPED captured_ov;

    FILE_HANDLE file_handle = start_file_io_async(FILE_READ_ASYNC, destination, sizeof(destination), position, mock_user_callback, user_context,  &captured_callback, &captured_ov);

    STRICT_EXPECTED_CALL(mock_CloseHandle(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mock_user_callback(user_context, false));
    ASSERT_ARE_EQUAL(uint64_t, position, captured_ov->Offset);

    ///act
    captured_callback(NULL, NULL, captured_ov, NO_ERROR, sizeof(destination) -1, NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    file_destroy(file_handle);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
