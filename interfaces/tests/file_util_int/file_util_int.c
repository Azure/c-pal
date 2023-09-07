// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <inttypes.h>
#include <string.h> // IWYU pragma: keep
#include <fcntl.h>
#include <stdbool.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "umock_c/umock_c_prod.h"

#include "c_pal/windows_defines.h"

#include "c_pal/file_util.h"
#include "c_pal/timer.h"
#include "c_pal/thandle.h"
#include "c_pal/threadpool.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/error_handling_linux.h"

#ifdef WIN32
#include "c_pal/execution_engine_win32.h"
#endif

#include "c_pal/execution_engine.h"

TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{

}

TEST_SUITE_CLEANUP(suite_cleanup)
{

}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{

}

static void callback_file_write(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PVOID Overlapped, ULONG IoResult, ULONG_PTR NumberOfBytesTransferred, PTP_IO Io)
{
    volatile_atomic int32_t* write_wait = Context;
    (void)Instance;
    (void)Overlapped;
    (void)IoResult;
    (void)NumberOfBytesTransferred;
    (void)Io;

    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_SetAndWake(write_wait, 1));
}

/*Tests_SRS_FILE_UTIL_LINUX_09_001: [ If the full_file_name input is either empty or NULL, file_util_open_file shall return an INVALID_HANDLE_VALUE. ]*/
TEST_FUNCTION(create_and_write_file)
{
    ///arrange
    HANDLE file_handle;

#ifdef WIN32
    const char* full_file_name = "C:\\devsecond\\sampletext.txt";

#else
    const char* full_file_name = "/mnt/c/devsecond/sampletext2.txt";

#endif
    uint32_t desired_access = GENERIC_ALL;
    uint32_t share_mode = FILE_SHARE_READ;
    uint32_t creation_disposition = CREATE_ALWAYS;
    uint32_t flags_and_attributes = FILE_FLAG_OVERLAPPED;
    file_handle = file_util_open_file(full_file_name, desired_access, share_mode, NULL, creation_disposition, flags_and_attributes, NULL);
    ASSERT_ARE_NOT_EQUAL(uint64_t, file_handle, INVALID_HANDLE_VALUE);

    volatile_atomic int32_t write_wait;
    interlocked_exchange(&write_wait, 0);
    char* str_input = "hello world";
    OVERLAPPED overlapped = { 0 };

    file_util_create_threadpool_io(file_handle, callback_file_write, (void*)&write_wait);

    file_util_write_file(file_handle, str_input, (uint32_t)strlen(str_input), &overlapped);

    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&write_wait, 1, UINT32_MAX));

    bool success_close = file_util_close_file(file_handle);
    ASSERT_IS_TRUE(success_close);

    bool success_delete = file_util_delete_file(full_file_name);
    ASSERT_IS_TRUE(success_delete);

}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
