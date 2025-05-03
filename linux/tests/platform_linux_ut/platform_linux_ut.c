// Copyright(C) Microsoft Corporation.All rights reserved.

#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"

#define ENABLE_MOCKS

#include "c_pal/completion_port_linux.h"
#include "platform_mocked.h"

#undef ENABLE_MOCKS

#include "c_pal/platform.h"
#include "c_pal/platform_linux.h"

static COMPLETION_PORT_HANDLE test_completion_port = (COMPLETION_PORT_HANDLE)0x4245;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static struct addrinfo g_addrInfo = { 0 };

static int my_getaddrinfo(const char* pNodeName, const char* pServiceName, const struct addrinfo* pHints, struct addrinfo** ppResult)
{
    (void)pNodeName;
    (void)pServiceName;
    (void)pHints;
    *ppResult = &g_addrInfo;
    return 0;
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_charptr_register_types());

    REGISTER_GLOBAL_MOCK_RETURNS(completion_port_create, test_completion_port, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(mocked_getaddrinfo, my_getaddrinfo);

    REGISTER_UMOCK_ALIAS_TYPE(COMPLETION_PORT_HANDLE, void*);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
}

TEST_FUNCTION_INITIALIZE(function_init)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(function_cleanup)
{
}

// platform_init

/* Tests_SRS_PLATFORM_LINUX_01_001: [ Otherwise, platform_init shall call getaddrinfo for localhost and port 4242. ]*/
// Tests_SRS_PLATFORM_LINUX_11_001: [ platform_init shall call completion_port_create. ]
// Tests_SRS_PLATFORM_LINUX_11_002: [ platform_init shall succeed and return zero. ]
TEST_FUNCTION(platform_init_succeeds)
{
    //arrange
    STRICT_EXPECTED_CALL(mocked_getaddrinfo("localhost", "4242", IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_freeaddrinfo(IGNORED_ARG));
    STRICT_EXPECTED_CALL(completion_port_create());

    //act
    int result = platform_init();

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    //cleanup
    platform_deinit();
}

// Tests_SRS_PLATFORM_LINUX_11_007: [ If the completion port object is non-NULL, platform_init shall return zero. ]
TEST_FUNCTION(platform_init_2nd_call_succeeds)
{
    //arrange
    STRICT_EXPECTED_CALL(mocked_getaddrinfo("localhost", "4242", IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_freeaddrinfo(IGNORED_ARG));
    STRICT_EXPECTED_CALL(completion_port_create());

    ASSERT_ARE_EQUAL(int, 0, platform_init());
    umock_c_reset_all_calls();

    //act
    int result = platform_init();

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    //cleanup
    platform_deinit();
}

// Tests_SRS_PLATFORM_LINUX_01_002: [ If any error occurs, platform_init shall return a non-zero value. ]
TEST_FUNCTION(when_getaddrinfo_fails_platform_init_fails)
{
    //arrange
    STRICT_EXPECTED_CALL(mocked_getaddrinfo("localhost", "4242", IGNORED_ARG, IGNORED_ARG))
        .SetReturn(-1);

    //act
    int result = platform_init();

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    //cleanup
    platform_deinit();
}

// Tests_SRS_PLATFORM_LINUX_01_002: [ If any error occurs, platform_init shall return a non-zero value. ]
TEST_FUNCTION(when_completion_port_create_fails_platform_init_fails)
{
    //arrange
    STRICT_EXPECTED_CALL(mocked_getaddrinfo("localhost", "4242", IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_freeaddrinfo(IGNORED_ARG));
    STRICT_EXPECTED_CALL(completion_port_create())
        .SetReturn(NULL);

    //act
    int result = platform_init();

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    //cleanup
    platform_deinit();
}

// platform_deinit

// Tests_SRS_PLATFORM_LINUX_11_004: [ If the completion port object is non-NULL, platform_deinit shall decrement whose reference by calling completion_port_dec_ref. ]
// Tests_SRS_PLATFORM_LINUX_11_005: [ If the completion object is not NULL, platform_get_completion_port shall increment the reference count of the COMPLETION_PORT_HANDLE object by calling completion_port_inc_ref. ]
TEST_FUNCTION(platform_deinit_succeeds)
{
    //arrange
    ASSERT_ARE_EQUAL(int, 0, platform_init());
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(completion_port_dec_ref(test_completion_port));

    //act
    platform_deinit();

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

// Tests_SRS_PLATFORM_LINUX_11_008: [ If the completion port object is non-NULL, platform_deinit shall do nothing. ]
TEST_FUNCTION(platform_deinit_without_a_platform_init_succeeds)
{
    //arrange

    //act
    platform_deinit();

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

// platform_get_completion_port

// Tests_SRS_PLATFORM_LINUX_11_006: [ platform_get_completion_port shall return the completion object. ]
TEST_FUNCTION(platform_get_completion_port_succeeds)
{
    //arrange
    ASSERT_ARE_EQUAL(int, 0, platform_init());
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(completion_port_inc_ref(test_completion_port));

    //act
    COMPLETION_PORT_HANDLE completion_port = platform_get_completion_port();

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(completion_port);
    ASSERT_ARE_EQUAL(void_ptr, test_completion_port, completion_port);

    //cleanup
    platform_deinit();
}

// Tests_SRS_PLATFORM_LINUX_11_006: [ platform_get_completion_port shall return the completion object. ]
TEST_FUNCTION(platform_get_completion_port_not_initialized_succeeds)
{
    //arrange

    //act
    COMPLETION_PORT_HANDLE completion_port = platform_get_completion_port();

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(completion_port);

    //cleanup
    platform_deinit();
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
