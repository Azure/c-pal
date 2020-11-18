// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#else
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "azure_macro_utils/macro_utils.h"

void* real_malloc(size_t size)
{
    return malloc(size);
}

void real_free(void* ptr)
{
    free(ptr);
}

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"


#define ENABLE_MOCKS

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_pal/sync.h"
#include "mock_file.h"

MOCKABLE_FUNCTION(, void, mock_user_callback, void*, user_context, bool, is_successful);

#undef ENABLE_MOCKS

#include "c_pal/file.h"

static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

TEST_DEFINE_ENUM_TYPE(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_RESULT)
IMPLEMENT_UMOCK_C_ENUM_TYPE(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_VALUES)

TEST_DEFINE_ENUM_TYPE(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_RESULT)
IMPLEMENT_UMOCK_C_ENUM_TYPE(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_VALUES)

#define FILE_IO_ASYNC_VALUES \
    \
    FILE_READ_ASYNC
MU_DEFINE_ENUM(FILE_IO_ASYNC_TYPE, FILE_IO_ASYNC_VALUES);

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static int fake_fildes = 42;
static EXECUTION_ENGINE_HANDLE fake_execution_engine = (EXECUTION_ENGINE_HANDLE)43;

static void setup_file_create_expectations(const char* filename)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mock_open(filename, O_CREAT | O_RDWR | __O_DIRECT | __O_LARGEFILE, 0700));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 0))
        .CallCannotFail();
}

static FILE_HANDLE get_file_handle(const char* filename)
{
    setup_file_create_expectations(filename);

    FILE_HANDLE file_handle = file_create(fake_execution_engine, filename, NULL, NULL);
    ASSERT_IS_NOT_NULL(file_handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    umock_c_reset_all_calls();
    return file_handle;
}

typedef void (*AIO_CALLBACK_FUNC)(__sigval_t sv);

static FILE_HANDLE start_file_write_async(const unsigned char* buffer, uint32_t size, uint64_t position, FILE_CB user_callback, void* user_context, void** captured_aiocbp, AIO_CALLBACK_FUNC* captured_callback, __sigval_t* captured_write_context)
{
    FILE_HANDLE file_handle = get_file_handle("test_file.txt");

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(sizeof(struct aiocb)))
        .CaptureReturn(captured_aiocbp);
    STRICT_EXPECTED_CALL(mock_aio_write(IGNORED_ARG))
        .ValidateArgumentValue_aiocbp(captured_aiocbp);
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));

    FILE_WRITE_ASYNC_RESULT result = file_write_async(file_handle, buffer, size, position, user_callback, user_context);

    *captured_callback = ((struct aiocb*)(*captured_aiocbp))->aio_sigevent.sigev_notify_function;
    *captured_write_context = ((struct aiocb*)(*captured_aiocbp))->aio_sigevent.sigev_value;

    ASSERT_ARE_EQUAL(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();
    return file_handle;
}

static FILE_HANDLE start_file_read_async(unsigned char* buffer, uint32_t size, uint64_t position, FILE_CB user_callback, void* user_context, void** captured_aiocbp, AIO_CALLBACK_FUNC* captured_callback, __sigval_t* captured_read_context)
{
    FILE_HANDLE file_handle = get_file_handle("test_file.txt");

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(sizeof(struct aiocb)))
        .CaptureReturn(captured_aiocbp);
    STRICT_EXPECTED_CALL(mock_aio_read(IGNORED_ARG))
        .ValidateArgumentValue_aiocbp(captured_aiocbp);
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));

    FILE_READ_ASYNC_RESULT result = file_read_async(file_handle, buffer, size, position, user_callback, user_context);

    *captured_callback = ((struct aiocb*)(*captured_aiocbp))->aio_sigevent.sigev_notify_function;
    *captured_read_context = ((struct aiocb*)(*captured_aiocbp))->aio_sigevent.sigev_value;

    ASSERT_ARE_EQUAL(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();
    return file_handle;
}

static void call_callback(struct aiocb* aiocbp)
{
    aiocbp->aio_sigevent.sigev_notify_function(aiocbp->aio_sigevent.sigev_value);
}

BEGIN_TEST_SUITE(file_linux_unittests)

TEST_SUITE_INITIALIZE(suite_init)
{
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));
    ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types());
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types());

    REGISTER_GLOBAL_MOCK_HOOK(malloc, real_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(free, real_free);

    REGISTER_TYPE(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_RESULT);
    REGISTER_TYPE(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_RESULT);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_GLOBAL_MOCK_RETURNS(mock_open, fake_fildes, -1);
    REGISTER_GLOBAL_MOCK_RETURNS(mock_close, 0, -1);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mock_aio_write, -1);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mock_aio_read, -1);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mock_aio_return, -1);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);
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

/*Tests_SRS_FILE_43_002: [ If full_file_name is NULL then file_create shall fail and return NULL. ]*/
/*Tests_SRS_FILE_LINUX_43_038: [ If full_file_name is NULL then file_create shall fail and return NULL. ]*/
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
/*Tests_SRS_FILE_LINUX_43_050: [ If full_file_name is an empty string, file_create shall fail and return NULL. ]*/
TEST_FUNCTION(file_create_fails_on_empty_full_file_name)
{
    ///arrange
    ///act
    FILE_HANDLE return_val = file_create(fake_execution_engine, "", NULL, NULL);
    ///assert
    ASSERT_IS_NULL(return_val);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_FILE_LINUX_43_029: [ file_create shall allocate a FILE_HANDLE. ]*/
/*Tests_SRS_FILE_LINUX_43_055: [ file_create shall initialize a counter for pending asynchronous operations to 0. ]*/
/*Tests_SRS_FILE_LINUX_43_001: [ file_create shall call open with full_file_name as pathname and flags O_CREAT, O_RDWR, O_DIRECT and O_LARGEFILE. ]*/
/*Tests_SRS_FILE_LINUX_43_002: [ file_create shall succeed and return a non-NULL value.]*/
TEST_FUNCTION(file_create_succeeds)
{
    ///arrange
    char* filename = "file_create_succeeds.txt";
    setup_file_create_expectations(filename);

    ///act
    FILE_HANDLE file_handle = file_create(fake_execution_engine, filename, NULL, NULL);

    ///assert
    ASSERT_IS_NOT_NULL(file_handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_43_034: [ If there are any failures, file_create shall fail and return NULL. ]*/
/*Tests_SRS_FILE_LINUX_43_053: [ If there are any other failures, file_create shall fail and return NULL. ]*/
TEST_FUNCTION(file_create_fails)
{
    ///arrange
    const char filename[] = "file_create_succeeds.txt";

    setup_file_create_expectations(filename);

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
/*Tests_SRS_FILE_LINUX_43_036: [ If handle is NULL, file_destroy shall return. ]*/
TEST_FUNCTION(file_destroy_called_with_null_handle)
{
    ///arrange

    ///act
    file_destroy(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_FILE_43_006: [ file_destroy shall wait for all pending I/O operations to complete. ]*/
/*Tests_SRS_FILE_43_007: [ file_destroy shall close the file handle handle. ]*/
/*Tests_SRS_FILE_LINUX_43_054: [ file_destroy shall wait for the pending asynchronous operations counter to become equal to zero. ]*/
/*Tests_SRS_FILE_LINUX_43_003: [ file_destroy shall call close.]*/
/*Tests_SRS_FILE_LINUX_43_030: [ file_destroy shall free the FILE_HANDLE. ]*/
TEST_FUNCTION(file_destroy_succeeds)
{

    ///arrange
    const char* filename = "file_destroy_succeeds.txt";
    setup_file_create_expectations(filename);

    FILE_HANDLE file_handle = file_create(fake_execution_engine, filename, NULL, NULL);

    ASSERT_IS_NOT_NULL(file_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(wait_on_address(IGNORED_ARG, IGNORED_ARG, UINT32_MAX));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(mock_close(fake_fildes));
    STRICT_EXPECTED_CALL(free(file_handle));

    ///act
    file_destroy(file_handle);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_FILE_43_009: [ If handle is NULL then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
/*Tests_SRS_FILE_LINUX_43_031: [ If handle is NULL then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
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
/*Tests_SRS_FILE_LINUX_43_032: [ If source is NULL then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
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
/*Tests_SRS_FILE_LINUX_43_049: [ If user_callback is NULL then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
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
/*Tests_SRS_FILE_LINUX_43_051: [ If position + size is greater than INT64_MAX, then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
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
/*Tests_SRS_FILE_LINUX_43_048: [ If size is 0 then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
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

/*Tests_SRS_FILE_LINUX_43_019: [ file_write_async shall allocate a struct to hold handle, source, size, user_callback and user_context. ]*/
/*Tests_SRS_FILE_LINUX_43_004: [ file_write_async shall initialize the sigevent struct on the allocated aiocb struct with SIGEV_THREAD as sigev_notify, the allocated struct as sigev_value, on_file_write_complete_linux as sigev_notify_function, NULL as sigev_notify_attributes. ]*/
/*Tests_SRS_FILE_LINUX_43_005: [ file_write_async shall allocate an aiocb struct with the file descriptor from file_handle as aio_fildes, position as aio_offset, source as aio_buf, size as aio_nbytes. ]*/
/*Tests_SRS_FILE_LINUX_43_006: [ file_write_async shall call aio_write with the allocated aiocb struct as aiocbp.]*/
/*Tests_SRS_FILE_LINUX_43_056: [ If aio_write succeeds, file_write_async shall increment the pending asynchronous operations counter on file_handle. ]*/
/*Tests_SRS_FILE_LINUX_43_007: [ If aio_write succeeds, file_write_async shall return FILE_WRITE_ASYNC_OK. ]*/
TEST_FUNCTION(file_write_async_succeeds_asynchronously)
{
    ///arrange
    FILE_HANDLE file_handle = get_file_handle("file_write_async_succeeds_asynchronously.txt");
    unsigned char source[10];
    uint32_t size = 10;
    uint64_t position = 5;

    void* captured_aiocbp;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(sizeof(struct aiocb)))
        .CaptureReturn(&captured_aiocbp);
    STRICT_EXPECTED_CALL(mock_aio_write(IGNORED_ARG))
        .ValidateArgumentValue_aiocbp(&captured_aiocbp);
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));

    ///act
    FILE_WRITE_ASYNC_RESULT result =  file_write_async(file_handle, source, size, position, mock_user_callback, NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_OK, result);
    ASSERT_ARE_EQUAL(int, SIGEV_THREAD, ((struct aiocb*)captured_aiocbp)->aio_sigevent.sigev_notify);
    ASSERT_ARE_EQUAL(uint64_t, position, ((struct aiocb*)captured_aiocbp)->aio_offset);
    ASSERT_IS_NULL(((struct aiocb*)captured_aiocbp)->aio_sigevent.sigev_notify_attributes);

    ///cleanup
    call_callback(captured_aiocbp);
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_43_035: [ If the call to write the file fails, file_write_async shall fail and return FILE_WRITE_ASYNC_WRITE_ERROR. ]*/
/*Tests_SRS_FILE_LINUX_43_019: [ file_write_async shall allocate a struct to hold handle, source, size, user_callback and user_context. ]*/
/*Tests_SRS_FILE_LINUX_43_004: [ file_write_async shall initialize the sigevent struct on the allocated aiocb struct with SIGEV_THREAD as sigev_notify, the allocated struct as sigev_value, on_file_write_complete_linux as sigev_notify_function, NULL as sigev_notify_attributes. ]*/
/*Tests_SRS_FILE_LINUX_43_006: [ file_write_async shall call aio_write with the allocated aiocb struct as aiocbp.]*/
/*Tests_SRS_FILE_LINUX_43_012: [ If aio_write fails, file_write_async shall return FILE_WRITE_ASYNC_WRITE_ERROR. ]*/
TEST_FUNCTION(file_write_async_fails_synchronously)
{
    ///arrange
    FILE_HANDLE file_handle = get_file_handle("file_write_async_fails_synchronously.txt");
    unsigned char source[10];
    uint32_t size = 10;
    uint64_t position = 5;

    void* io_context;
    void* captured_aiocbp;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .CaptureReturn(&io_context);
    STRICT_EXPECTED_CALL(malloc(sizeof(struct aiocb)))
        .CaptureReturn(&captured_aiocbp);
    STRICT_EXPECTED_CALL(mock_aio_write(IGNORED_ARG))
        .SetReturn(-1);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&captured_aiocbp);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&io_context);
    ///act
    FILE_WRITE_ASYNC_RESULT result = file_write_async(file_handle, source, size, position, mock_user_callback, NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_WRITE_ERROR, result);

    ///cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_43_015: [ If there are any failures, file_write_async shall fail and return FILE_WRITE_ASYNC_ERROR. ]*/
/*Tests_SRS_FILE_LINUX_43_013: [ If there are any other failures, file_write_async shall return FILE_WRITE_ASYNC_ERROR. ]*/
TEST_FUNCTION(file_write_async_fails)
{
    ///arrange
    FILE_HANDLE file_handle = get_file_handle("file_write_async_fails.txt");
    unsigned char source[10];
    uint32_t size = 10;
    uint64_t position = 5;
    void* io;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(sizeof(struct aiocb)));
    STRICT_EXPECTED_CALL(mock_aio_write(IGNORED_ARG))
        .CallCannotFail();

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
/*Tests_SRS_FILE_LINUX_43_034: [ If handle is NULL then file_read_async shall fail and return FILE_READ_ASYNC_INVALID_ARGS. ]*/
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
/*Tests_SRS_FILE_LINUX_43_043: [ IfdestinationisNULLthenfile_read_asyncshall fail and returnFILE_READ_ASYNC_INVALID_ARGS`. ]*/
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
/*Tests_SRS_FILE_LINUX_43_035: [ If user_callback is NULL then file_read_async shall fail and return FILE_READ_ASYNC_INVALID_ARGS. ]*/
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
/*Tests_SRS_FILE_LINUX_43_052: [ If size is 0 then file_read_async shall fail and return FILE_READ_ASYNC_INVALID_ARGS. ]*/
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

/*Tests_SRS_FILE_LINUX_43_045: [ file_read_async shall allocate a struct to hold handle, destination, user_callback and user_context. ]*/
/*Tests_SRS_FILE_LINUX_43_009: [ file_read_async shall allocate an aiocb struct with the file descriptor from file_handle as aio_fildes, position as aio_offset, the allocated buffer as aio_buf, size as aio_nbytes. ]*/
/*Tests_SRS_FILE_LINUX_43_008: [ file_read_async shall initialize the sigevent struct on the allocated aiocb struct with SIGEV_THREAD as sigev_notify, user_context as sigev_value, on_file_read_complete_linux as sigev_notify_function, NULL as sigev_notify_attributes. ]*/
/*Tests_SRS_FILE_LINUX_43_010: [ file_read_async shall call aio_read with the allocated aiocb struct as aiocbp. ]*/
/*Tests_SRS_FILE_LINUX_43_057: [ If aio_read succeeds, file_read_async shall increment the pending asynchronous operations counter on file_handle. ]*/
/*Tests_SRS_FILE_LINUX_43_014: [ If aio_read succeeds, file_read_async shall return FILE_READ_ASYNC_OK. ]*/
TEST_FUNCTION(file_read_async_succeeds_asynchronously)
{
    ///arrange
    FILE_HANDLE file_handle = get_file_handle("file_read_async_succeeds_asynchronously.txt");
    unsigned char destination[10];
    uint32_t size = 10;
    uint64_t position = 5;

    void* captured_aiocbp;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(sizeof(struct aiocb)))
        .CaptureReturn(&captured_aiocbp);
    STRICT_EXPECTED_CALL(mock_aio_read(IGNORED_ARG))
        .ValidateArgumentValue_aiocbp(&captured_aiocbp);
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));

    ///act
    FILE_READ_ASYNC_RESULT result =  file_read_async(file_handle, destination, size, position, mock_user_callback, NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_OK, result);
    ASSERT_ARE_EQUAL(int, SIGEV_THREAD, ((struct aiocb*)captured_aiocbp)->aio_sigevent.sigev_notify);
    ASSERT_ARE_EQUAL(uint64_t, position, ((struct aiocb*)captured_aiocbp)->aio_offset);
    ASSERT_IS_NULL(((struct aiocb*)captured_aiocbp)->aio_sigevent.sigev_notify_attributes);

    ///cleanup
    call_callback(captured_aiocbp);
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_43_036: [ If the call to read the file fails, file_read_async shall fail and return FILE_READ_ASYNC_READ_ERROR. ]*/
/*Tests_SRS_FILE_LINUX_43_045: [ file_read_async shall allocate a struct to hold handle, destination, user_callback and user_context. ]*/
/*Tests_SRS_FILE_LINUX_43_009: [ file_read_async shall allocate an aiocb struct with the file descriptor from file_handle as aio_fildes, position as aio_offset, the allocated buffer as aio_buf, size as aio_nbytes. ]*/
/*Tests_SRS_FILE_LINUX_43_008: [ file_read_async shall initialize the sigevent struct on the allocated aiocb struct with SIGEV_THREAD as sigev_notify, user_context as sigev_value, on_file_read_complete_linux as sigev_notify_function, NULL as sigev_notify_attributes. ]*/
/*Tests_SRS_FILE_LINUX_43_010: [ file_read_async shall call aio_read with the allocated aiocb struct as aiocbp. ]*/
/*Tests_SRS_FILE_LINUX_43_011: [ If aio_read fails, file_read_async shall return FILE_READ_ASYNC_READ_ERROR. ]*/
TEST_FUNCTION(file_read_async_fails_synchronously)
{
    ///arrange
    FILE_HANDLE file_handle = get_file_handle("file_read_async_fails_synchronously.txt");
    unsigned char source[10];
    uint32_t size = 10;
    uint64_t position = 5;

    void* io_context;
    void* captured_aiocbp;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .CaptureReturn(&io_context);
    STRICT_EXPECTED_CALL(malloc(sizeof(struct aiocb)))
        .CaptureReturn(&captured_aiocbp);
    STRICT_EXPECTED_CALL(mock_aio_read(IGNORED_ARG))
        .SetReturn(-1);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&captured_aiocbp);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&io_context);
    ///act
    FILE_READ_ASYNC_RESULT result = file_read_async(file_handle, source, size, position, mock_user_callback, NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_READ_ERROR, result);

    ///cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_43_022: [ If there are any failures then file_read_async shall fail and return FILE_READ_ASYNC_ERROR. ]*/
/*Tests_SRS_FILE_LINUX_43_015: [ If there are any other failures, file_read_async shall return FILE_READ_ASYNC_ERROR. ]*/
TEST_FUNCTION(file_read_async_fails)
{
    ///arrange
    FILE_HANDLE file_handle = get_file_handle("file_read_async_fails.txt");
    unsigned char source[10];
    uint32_t size = 10;
    uint64_t position = 5;
    void* io;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(sizeof(struct aiocb)));
    STRICT_EXPECTED_CALL(mock_aio_read(IGNORED_ARG))
        .CallCannotFail();

    umock_c_negative_tests_snapshot();
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); ++i)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            ///act
            FILE_READ_ASYNC_RESULT result = file_read_async(file_handle, source, size, position, mock_user_callback, NULL);

            ///assert
            ASSERT_ARE_EQUAL(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_ERROR, result);
        }
    }
    ///cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_LINUX_43_018: [ file_extend shall return 0. ]*/
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

/*Tests_SRS_FILE_LINUX_43_058: [ on_file_write_complete_linux shall decrement the pending asynchronous operations counter on the FILE_HANDLE contained in write_info. ]*/
/*Tests_SRS_FILE_LINUX_43_066: [ on_file_write_complete_linux shall wake all threads waiting on the address of the pending asynchronous operations counter. ]*/
/*Tests_SRS_FILE_LINUX_43_022: [ on_file_write_complete_linux shall call aio_return to determine if the asynchronous write operation succeeded. ]*/
/*Tests_SRS_FILE_LINUX_43_062: [ on_file_write_complete_linux shall free the aiocb struct associated with the current asynchronous write operation. ]*/
/*Tests_SRS_FILE_LINUX_43_063: [ on_file_write_complete_linux shall free write_info. ]*/
/*Tests_SRS_FILE_LINUX_43_027: [ If the asynchronous write operation succeeded, on_file_write_complete_linux shall call user_callback with user_context and true as is_successful. ]*/
/*Tests_SRS_FILE_LINUX_43_027: [ If the asynchronous write operation succeeded, on_file_write_complete_linux shall call user_callback with user_context and true as is_successful. ]*/
TEST_FUNCTION(on_file_write_complete_linux_calls_callback_successfully)
{
    ///arrange
    unsigned char source[10];
    uint64_t position = 11;
    void* user_context = (void*)20;

    void* captured_aiocbp;
    AIO_CALLBACK_FUNC captured_callback;
    __sigval_t captured_write_context;

    FILE_HANDLE file_handle = start_file_write_async(source, sizeof(source), position, mock_user_callback, user_context, &captured_aiocbp, &captured_callback, &captured_write_context);


    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(wake_by_address_all(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mock_aio_return(IGNORED_ARG))
        .ValidateArgumentValue_aiocbp(&captured_aiocbp)
        .SetReturn(sizeof(source));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&captured_aiocbp);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&captured_write_context.sival_ptr);
    STRICT_EXPECTED_CALL(mock_user_callback(user_context, true));
    ASSERT_ARE_EQUAL(uint64_t, position, ((struct aiocb*)captured_aiocbp)->aio_offset);

    ///act
    captured_callback(captured_write_context);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    file_destroy(file_handle);
}


/*Tests_SRS_FILE_LINUX_43_058: [ on_file_write_complete_linux shall decrement the pending asynchronous operations counter on the FILE_HANDLE contained in write_info. ]*/
/*Tests_SRS_FILE_LINUX_43_066: [ on_file_write_complete_linux shall wake all threads waiting on the address of the pending asynchronous operations counter. ]*/
/*Tests_SRS_FILE_LINUX_43_022: [ on_file_write_complete_linux shall call aio_return to determine if the asynchronous write operation succeeded. ]*/
/*Tests_SRS_FILE_LINUX_43_062: [ on_file_write_complete_linux shall free the aiocb struct associated with the current asynchronous write operation. ]*/
/*Tests_SRS_FILE_LINUX_43_063: [ on_file_write_complete_linux shall free write_info. ]*/
/*Tests_SRS_FILE_LINUX_43_023: [ If the asynchronous write operation did not succeed, on_file_write_complete_linux shall call user_callback with user_context and false as is_successful. ]*/
TEST_FUNCTION(on_file_write_complete_linux_calls_callback_unsuccessfully_because_io_failed)
{
    ///arrange
    unsigned char source[10];
    uint64_t position = 11;
    void* user_context = (void*)20;

    void* captured_aiocbp;
    AIO_CALLBACK_FUNC captured_callback;
    __sigval_t captured_write_context;

    FILE_HANDLE file_handle = start_file_write_async(source, sizeof(source), position, mock_user_callback, user_context, &captured_aiocbp, &captured_callback, &captured_write_context);

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(wake_by_address_all(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mock_aio_return(IGNORED_ARG))
        .ValidateArgumentValue_aiocbp(&captured_aiocbp)
        .SetReturn(-1);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&captured_aiocbp);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&captured_write_context.sival_ptr);
    STRICT_EXPECTED_CALL(mock_user_callback(user_context, false));
    ASSERT_ARE_EQUAL(uint64_t, position, ((struct aiocb*)captured_aiocbp)->aio_offset);

    ///act
    captured_callback(captured_write_context);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_LINUX_43_058: [ on_file_write_complete_linux shall decrement the pending asynchronous operations counter on the FILE_HANDLE contained in write_info. ]*/
/*Tests_SRS_FILE_LINUX_43_066: [ on_file_write_complete_linux shall wake all threads waiting on the address of the pending asynchronous operations counter. ]*/
/*Tests_SRS_FILE_LINUX_43_022: [ on_file_write_complete_linux shall call aio_return to determine if the asynchronous write operation succeeded. ]*/
/*Tests_SRS_FILE_LINUX_43_062: [ on_file_write_complete_linux shall free the aiocb struct associated with the current asynchronous write operation. ]*/
/*Tests_SRS_FILE_LINUX_43_063: [ on_file_write_complete_linux shall free write_info. ]*/
/*Tests_SRS_FILE_LINUX_43_064: [ If the number of bytes written are less than the bytes requested by the user, on_file_write_complete_linux shall call user_callback with user_context and false as is_successful. ]*/
TEST_FUNCTION(on_file_write_complete_linux_calls_callback_unsuccessfully_because_num_bytes_is_less)
{
    ///arrange
    unsigned char source[10];
    uint64_t position = 11;
    void* user_context = (void*)20;

    void* captured_aiocbp;
    AIO_CALLBACK_FUNC captured_callback;
    __sigval_t captured_write_context;

    FILE_HANDLE file_handle = start_file_write_async(source, sizeof(source), position, mock_user_callback, user_context, &captured_aiocbp, &captured_callback, &captured_write_context);

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(wake_by_address_all(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mock_aio_return(IGNORED_ARG))
        .ValidateArgumentValue_aiocbp(&captured_aiocbp)
        .SetReturn(sizeof(source)-1);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&captured_aiocbp);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&captured_write_context.sival_ptr);
    STRICT_EXPECTED_CALL(mock_user_callback(user_context, false));
    ASSERT_ARE_EQUAL(uint64_t, position, ((struct aiocb*)captured_aiocbp)->aio_offset);

    ///act
    captured_callback(captured_write_context);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_LINUX_43_059: [ on_file_read_complete_linux shall decrement the pending asynchronous operations counter on the FILE_HANDLE contained in read_info. ]*/
/*Tests_SRS_FILE_LINUX_43_067: [ on_file_read_complete_linux shall wake all threads waiting on the address of the pending asynchronous operations counter. ]*/
/*Tests_SRS_FILE_LINUX_43_040: [ on_file_read_complete_linux shall call aio_return to determine if the asynchronous read operation succeeded. ]*/
/*Tests_SRS_FILE_LINUX_43_060: [ on_file_read_complete_linux shall free the aiocb struct associated with the current asynchronous read operation. ]*/
/*Tests_SRS_FILE_LINUX_43_061: [ on_file_read_complete_linux shall free read_info. ]*/
/*Tests_SRS_FILE_LINUX_43_042: [ If the asynchronous read operation succeeded, on_file_read_complete_linux shall call user_callback with user_context and false as is_successful. ]*/
TEST_FUNCTION(on_file_read_complete_linux_calls_callback_successfully)
{
    ///arrange
    unsigned char source[10];
    uint64_t position = 11;
    void* user_context = (void*)20;

    void* captured_aiocbp;
    AIO_CALLBACK_FUNC captured_callback;
    __sigval_t captured_write_context;

    FILE_HANDLE file_handle = start_file_read_async(source, sizeof(source), position, mock_user_callback, user_context, &captured_aiocbp, &captured_callback, &captured_write_context);

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(wake_by_address_all(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mock_aio_return(IGNORED_ARG))
        .ValidateArgumentValue_aiocbp(&captured_aiocbp)
        .SetReturn(sizeof(source));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&captured_aiocbp);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&captured_write_context.sival_ptr);
    STRICT_EXPECTED_CALL(mock_user_callback(user_context, true));
    ASSERT_ARE_EQUAL(uint64_t, position, ((struct aiocb*)captured_aiocbp)->aio_offset);

    ///act
    captured_callback(captured_write_context);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_LINUX_43_059: [ on_file_read_complete_linux shall decrement the pending asynchronous operations counter on the FILE_HANDLE contained in read_info. ]*/
/*Tests_SRS_FILE_LINUX_43_067: [ on_file_read_complete_linux shall wake all threads waiting on the address of the pending asynchronous operations counter. ]*/
/*Tests_SRS_FILE_LINUX_43_040: [ on_file_read_complete_linux shall call aio_return to determine if the asynchronous read operation succeeded. ]*/
/*Tests_SRS_FILE_LINUX_43_060: [ on_file_read_complete_linux shall free the aiocb struct associated with the current asynchronous read operation. ]*/
/*Tests_SRS_FILE_LINUX_43_061: [ on_file_read_complete_linux shall free read_info. ]*/
/*Tests_SRS_FILE_LINUX_43_041: [ If the asynchronous read operation did not succeed, on_file_read_complete_linux shall call user_callback with user_context and false as is_successful. ]*/
TEST_FUNCTION(on_file_read_complete_linux_calls_callback_unsuccessfully_because_io_failed)
{
    ///arrange
    unsigned char source[10];
    uint64_t position = 11;
    void* user_context = (void*)20;

    void* captured_aiocbp;
    AIO_CALLBACK_FUNC captured_callback;
    __sigval_t captured_write_context;

    FILE_HANDLE file_handle = start_file_read_async(source, sizeof(source), position, mock_user_callback, user_context, &captured_aiocbp, &captured_callback, &captured_write_context);

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(wake_by_address_all(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mock_aio_return(IGNORED_ARG))
        .ValidateArgumentValue_aiocbp(&captured_aiocbp)
        .SetReturn(-1);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&captured_aiocbp);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&captured_write_context.sival_ptr);
    STRICT_EXPECTED_CALL(mock_user_callback(user_context, false));
    ASSERT_ARE_EQUAL(uint64_t, position, ((struct aiocb*)captured_aiocbp)->aio_offset);

    ///act
    captured_callback(captured_write_context);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_LINUX_43_059: [ on_file_read_complete_linux shall decrement the pending asynchronous operations counter on the FILE_HANDLE contained in read_info. ]*/
/*Tests_SRS_FILE_LINUX_43_067: [ on_file_read_complete_linux shall wake all threads waiting on the address of the pending asynchronous operations counter. ]*/
/*Tests_SRS_FILE_LINUX_43_040: [ on_file_read_complete_linux shall call aio_return to determine if the asynchronous read operation succeeded. ]*/
/*Tests_SRS_FILE_LINUX_43_060: [ on_file_read_complete_linux shall free the aiocb struct associated with the current asynchronous read operation. ]*/
/*Tests_SRS_FILE_LINUX_43_061: [ on_file_read_complete_linux shall free read_info. ]*/
/*Tests_SRS_FILE_LINUX_43_065: [ If the number of bytes read are less than the bytes requested by the user, on_file_read_complete_linux shall call user_callback with user_context and false as is_successful. ]*/
TEST_FUNCTION(on_file_read_complete_linux_calls_callback_unsuccessfully_because_num_bytes_is_less)
{
    ///arrange
    unsigned char source[10];
    uint64_t position = 11;
    void* user_context = (void*)20;

    void* captured_aiocbp;
    AIO_CALLBACK_FUNC captured_callback;
    __sigval_t captured_write_context;

    FILE_HANDLE file_handle = start_file_read_async(source, sizeof(source), position, mock_user_callback, user_context, &captured_aiocbp, &captured_callback, &captured_write_context);

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(wake_by_address_all(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mock_aio_return(IGNORED_ARG))
        .ValidateArgumentValue_aiocbp(&captured_aiocbp)
        .SetReturn(sizeof(source)-1);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&captured_aiocbp);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&captured_write_context.sival_ptr);
    STRICT_EXPECTED_CALL(mock_user_callback(user_context, false));
    ASSERT_ARE_EQUAL(uint64_t, position, ((struct aiocb*)captured_aiocbp)->aio_offset);

    ///act
    captured_callback(captured_write_context);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    file_destroy(file_handle);
}

END_TEST_SUITE(file_linux_unittests)
