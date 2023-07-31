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
#include "c_pal/thandle.h"
#include "c_pal/threadpool.h"

#ifdef WIN32
#include "c_pal/execution_engine_win32.h"
#endif

#include "c_pal/execution_engine.h"

/* static const char* G_FILE_NAME = "test_file"; */


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

/*Tests_SRS_FILE_UTIL_LINUX_09_001: [ If the full_file_name input is either empty or NULL, file_util_open_file shall return an INVALID_HANDLE_VALUE. ]*/
TEST_FUNCTION(create_5K_file)
{
    ///arrange
    HANDLE result;

    ///act
    const char* full_file_name = "C:\\devsecond\\sampletext.txt";
    uint32_t desired_access = GENERIC_WRITE;
    uint32_t share_mode = FILE_SHARE_READ;
    uint32_t creation_disposition = 1;
    uint32_t flags_and_attributes = 128;
    result = file_util_open_file(full_file_name, desired_access, share_mode, NULL,creation_disposition, flags_and_attributes ,NULL);
    ASSERT_ARE_NOT_EQUAL(uint64_t, result, INVALID_HANDLE_VALUE);

#ifdef WIN32
    EXECUTION_ENGINE_PARAMETERS_WIN32 execution_engine_parameters = { 4, 0 };
#else
    //EXECUTION_ENGINE_PARAMETERS_LINUX execution_engine_parameters = { 4, 0 };
#endif

    //EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    //ASSERT_IS_NOT_NULL(execution_engine);

   // THANDLE(THREADPOOL) file_threadpool = threadpool_create(execution_engine);
    //ASSERT_IS_NOT_NULL(file_threadpool);

    char* str_input = "hello world";
    OVERLAPPED overlapped = { 0 };

    bool success_open = file_util_write_file(result, str_input, (uint32_t)strlen(str_input), &overlapped , NULL);
    ASSERT_IS_TRUE(success_open);

   /* bool success_close = file_util_close_file(result); */
    //ASSERT_IS_TRUE(success_close);

    //threadpool_close(file_threadpool);

    /* bool success_delete = file_util_delete_file(full_file_name); */
    //ASSERT_IS_TRUE(success_delete);

   // THANDLE_ASSIGN(THREADPOOL)(&file_threadpool, NULL);
   // execution_engine_dec_ref(execution_engine);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
