// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
 #include <unistd.h>
#include <fcntl.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umock_c_negative_tests.h"

#include "real_gballoc_ll.h"

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "umock_c/umock_c_prod.h"
#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"

#include "c_pal/windows_defines.h"

#include "c_pal/file_util.h"

static const char* G_FILE_NAME = "test_file";

MOCK_FUNCTION_WITH_CODE(, int, mocked_close, int, s)
MOCK_FUNCTION_END(0)
MOCK_FUNCTION_WITH_CODE(, int, mocked_open, const char*, pathname, int, s)
MOCK_FUNCTION_END(1)

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)
static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types());
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc_2, NULL);

    REGISTER_UMOCK_ALIAS_TYPE(HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LPSECURITY_ATTRIBUTES, void*);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
    umock_c_negative_tests_deinit();
    real_gballoc_ll_deinit();
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    umock_c_reset_all_calls();
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init(), "umock_c_negative_tests_init failed");
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    umock_c_negative_tests_deinit();
}

// Tests_SRS_FILE_UTIL_LINUX_09_001: [ If the full_file_name input is either empty or NULL, file_util_create_file shall return an INVALID_HANDLE_VALUE. ]
TEST_FUNCTION(file_util_create_file_full_file_name_NULL_Fail)
{
    ///arrange
    HANDLE result;

    ///act
    uint32_t desired_access = GENERIC_READ;
    uint32_t share_mode = FILE_SHARE_READ;
    uint32_t creation_disposition = 1;
    uint32_t flags_and_attributes = 128;
    result = file_util_create_file(NULL, desired_access, share_mode, NULL,creation_disposition, flags_and_attributes ,NULL);

    ///assert
    ASSERT_ARE_EQUAL(uint64_t, result, INVALID_HANDLE_VALUE);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_FILE_UTIL_LINUX_09_003: [ If memory allocation for result fails, file_util_create_file shall return an INVALID_HANDLE_VALUE. 
TEST_FUNCTION(file_util_create_file_malloc_Fail)
{
    HANDLE result;
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .SetReturn(INVALID_HANDLE_VALUE);

    uint32_t desired_access = GENERIC_READ;
    uint32_t share_mode = FILE_SHARE_READ;
    uint32_t creation_disposition = 1;
    uint32_t flags_and_attributes = 128;
    result = file_util_create_file(G_FILE_NAME, desired_access, share_mode, NULL,creation_disposition, flags_and_attributes ,NULL);

    ///assert
    ASSERT_ARE_EQUAL(uint64_t, result, INVALID_HANDLE_VALUE);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//Tests_SRS_FILE_UTIL_LINUX_09_008: [ If there are any failures, file_util_create_file shall fail and return INVALID_HANDLE_VALUE. ]
TEST_FUNCTION(file_util_create_file_open_Fail)
{
    ///arrange
    HANDLE result;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_open(G_FILE_NAME, O_RDONLY|O_CREAT)) 
        .SetReturn(-1);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    ///act
    uint32_t desired_access = GENERIC_READ;
    uint32_t share_mode = FILE_SHARE_READ;
    uint32_t creation_disposition = CREATE_ALWAYS;
    uint32_t flags_and_attributes = 128;
    result = file_util_create_file(G_FILE_NAME, desired_access, share_mode, NULL,creation_disposition, flags_and_attributes ,NULL);


    ///assert
    ASSERT_ARE_EQUAL(uint64_t, result, INVALID_HANDLE_VALUE);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

static const struct CREATION_DISPOSITION_INPUTS_TAG
{
    int creation_disposition;
    int flags;

} TEST_VEC_CREATION_DISPOSITION_INPUTS[] = 
    {
        {CREATE_ALWAYS, O_CREAT},
        {CREATE_NEW, O_CREAT|O_EXCL},
        {TRUNCATE_EXISTING, O_TRUNC},
        {OPEN_ALWAYS, O_CREAT},
    };

//Tests_SRS_FILE_UTIL_LINUX_09_014: [ If creation_disposition is CREATE_ALWAYS or OPEN_ALWAYS, file_util_create_file will call open with O_CREAT and shall either create a new file handle if the specificied pathname exists and return it or return an existing file handle. ]
//Tests_SRS_FILE_UTIL_LINUX_09_015: [ If creation_disposition is CREATE_NEW, file_util_create_file will call open with O_CREAT|O_EXCL and shall return a new file handle if the file doesn't already exist. ]
//Tests_SRS_FILE_UTIL_LINUX_09_017: [ If creation_disposition is TRUNCATE_EXISTING, file_util_create_file will call open with O_TRUNC and shall return a file handle who's size has been truncated to zero bytes. ]
TEST_FUNCTION(file_util_create_file_creation_disposition_Succeeds)
{
    size_t i;
    for(i = 0; i < sizeof(TEST_VEC_CREATION_DISPOSITION_INPUTS) / sizeof(TEST_VEC_CREATION_DISPOSITION_INPUTS[0]); i++)
    {
        ///arrange
        HANDLE result;

        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
        STRICT_EXPECTED_CALL(mocked_open(G_FILE_NAME, O_RDONLY|TEST_VEC_CREATION_DISPOSITION_INPUTS[i].flags));

        ///act
        uint32_t desired_access = GENERIC_READ;
        uint32_t share_mode = FILE_SHARE_READ;
        uint32_t creation_disposition = TEST_VEC_CREATION_DISPOSITION_INPUTS[i].creation_disposition;
        uint32_t flags_and_attributes = 128;
        result = file_util_create_file(G_FILE_NAME, desired_access, share_mode, NULL,creation_disposition, flags_and_attributes ,NULL);

        ///assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        (void)file_util_close_handle(result);
        umock_c_reset_all_calls();
    }
}

TEST_FUNCTION(file_util_create_file_desired_access_ALL_creation_disposition_Succeeds)
{
    size_t i;
    for(i = 0; i < sizeof(TEST_VEC_CREATION_DISPOSITION_INPUTS) / sizeof(TEST_VEC_CREATION_DISPOSITION_INPUTS[0]); i++)
    {
        ///arrange
        HANDLE result;

        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
        STRICT_EXPECTED_CALL(mocked_open(G_FILE_NAME, O_RDWR|TEST_VEC_CREATION_DISPOSITION_INPUTS[i].flags));

        ///act
        uint32_t desired_access = GENERIC_ALL;
        uint32_t share_mode = FILE_SHARE_READ;
        uint32_t creation_disposition = TEST_VEC_CREATION_DISPOSITION_INPUTS[i].creation_disposition;
        uint32_t flags_and_attributes = 128;
        result = file_util_create_file(G_FILE_NAME, desired_access, share_mode, NULL,creation_disposition, flags_and_attributes ,NULL);

        ///assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        (void)file_util_close_handle(result);
        umock_c_reset_all_calls();
    }
}

static const struct DESIRED_ACCESS_INPUTS
{
    int in_desired_access;
    int user_access;
} DESIRED_ACCESS_INPUTS[] = 
    {
        {GENERIC_READ, O_RDONLY},
        {GENERIC_WRITE, O_WRONLY},
        {GENERIC_ALL, O_RDWR},
        {GENERIC_READ&GENERIC_WRITE, O_RDWR},
    };

//Tests_SRS_FILE_UTIL_LINUX_09_004: [ If desired_access is GENERIC_READ, file_util_create_file will call open with O_RDONLY and shall return a file handle for read only. ]
//Tests_SRS_FILE_UTIL_LINUX_09_005: [ If desired_access is GENERIC_WRITE, file_util_create_file will call open with O_WRONLY and shall return a file handle for write only. ]
//Tests_SRS_FILE_UTIL_LINUX_09_006: [ If desired_access is GENERIC_ALL or GENERIC_READ&GENERIC_WRITE, file_util_create_file will call open with O_RDWR and shall return a file handle for read and write. ]
TEST_FUNCTION(file_util_create_file_desired_access_Succeeds)
{
    size_t i;
    for(i = 0; i < sizeof(DESIRED_ACCESS_INPUTS) / sizeof(DESIRED_ACCESS_INPUTS[0]); i++)
    {
        ///arrange
        HANDLE result;

        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
        STRICT_EXPECTED_CALL(mocked_open(G_FILE_NAME, DESIRED_ACCESS_INPUTS[i].user_access|O_CREAT));

        ///act
        uint32_t desired_access = DESIRED_ACCESS_INPUTS[i].in_desired_access;
        uint32_t share_mode = FILE_SHARE_READ;
        uint32_t creation_disposition = OPEN_ALWAYS;
        uint32_t flags_and_attributes = 128;
        result = file_util_create_file(G_FILE_NAME, desired_access, share_mode, NULL,creation_disposition, flags_and_attributes ,NULL);

        ///assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        (void)file_util_close_handle(result);
        umock_c_reset_all_calls();
    }
}

TEST_FUNCTION(file_util_create_file_create_disposition_NEW_desired_access_Succeeds)
{
    size_t i;
    for(i = 0; i < sizeof(DESIRED_ACCESS_INPUTS) / sizeof(DESIRED_ACCESS_INPUTS[0]); i++)
    {
        ///arrange
        HANDLE result;

        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
        STRICT_EXPECTED_CALL(mocked_open(G_FILE_NAME, DESIRED_ACCESS_INPUTS[i].user_access|O_CREAT|O_EXCL));

        ///act
        uint32_t desired_access = DESIRED_ACCESS_INPUTS[i].in_desired_access;
        uint32_t share_mode = FILE_SHARE_READ;
        uint32_t creation_disposition = CREATE_NEW;
        uint32_t flags_and_attributes = 128;
        result = file_util_create_file(G_FILE_NAME, desired_access, share_mode, NULL,creation_disposition, flags_and_attributes ,NULL);

        ///assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        (void)file_util_close_handle(result);
        umock_c_reset_all_calls();
    }
}

//Tests_SRS_FILE_UTIL_LINUX_09_011: [ file_util_close_handle closes a file handle in linux and returns true. ]
TEST_FUNCTION(file_util_close_handle_Succeeds)
{
    HANDLE handle_input;
    uint32_t desired_access = GENERIC_READ;
    uint32_t share_mode = FILE_SHARE_READ;
    uint32_t creation_disposition = CREATE_ALWAYS;
    uint32_t flags_and_attributes = 128;
    handle_input = file_util_create_file(G_FILE_NAME, desired_access, share_mode, NULL,creation_disposition, flags_and_attributes ,NULL);
    umock_c_reset_all_calls();

    bool result;

    STRICT_EXPECTED_CALL(mocked_close(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    result = file_util_close_handle(handle_input);
    
    //assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//Tests_SRS_FILE_UTIL_LINUX_09_013: [ If close returns a zero, then file_util_close_handle shall return true. ]
TEST_FUNCTION(file_util_close_handle_FAILS)
{
    HANDLE handle_input;
    uint32_t desired_access = GENERIC_READ;
    uint32_t share_mode = FILE_SHARE_READ;
    uint32_t creation_disposition = CREATE_ALWAYS;
    uint32_t flags_and_attributes = 128;
    handle_input = file_util_create_file(G_FILE_NAME, desired_access, share_mode, NULL,creation_disposition, flags_and_attributes ,NULL);
    umock_c_reset_all_calls();

    bool result;

    STRICT_EXPECTED_CALL(mocked_close(IGNORED_ARG))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    result = file_util_close_handle(handle_input);
    
    //assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//Tests_SRS_FILE_UTIL_LINUX_09_009: [ If handle_input is NULL, file_util_close_handle returns false. ]
TEST_FUNCTION(file_util_close_handle_input_NULL)
{
    ///arrange
    bool result;
    HANDLE handle_input = INVALID_HANDLE_VALUE;

    ///act
    result = file_util_close_handle(handle_input);

    ///assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
} 

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
