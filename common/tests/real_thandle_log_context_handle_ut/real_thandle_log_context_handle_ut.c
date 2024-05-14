// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "testrunnerswitcher.h"

#include "c_pal/interlocked.h" // IWYU pragma: keep

#include "c_logging/log_context.h"

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h" // IWYU pragma: keep
#include "c_pal/gballoc_hl_redirect.h" // IWYU pragma: keep
#include "c_pal/thandle_ll.h"
#include "c_pal/thandle_ptr.h"
#include "c_pal/thandle_log_context_handle.h"
#undef ENABLE_MOCKS


#include "umock_c/umock_c_prod.h"
#include "real_gballoc_hl.h"
#include "real_thandle_log_context_handle.h"

#include "module.h"

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_charptr_register_types");

    REGISTER_UMOCK_ALIAS_TYPE(THANDLE_PTR_FREE_FUNC_TYPE_NAME(LOG_CONTEXT_HANDLE), void*);
    REGISTER_UMOCK_ALIAS_TYPE(LOG_CONTEXT_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(PTR(LOG_CONTEXT_HANDLE)), void*);

    REGISTER_UMOCK_ALIAS_TYPE(THANDLE_PTR_FREE_FUNC_TYPE_NAME(real_LOG_CONTEXT_HANDLE), void*);
    REGISTER_UMOCK_ALIAS_TYPE(real_LOG_CONTEXT_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(PTR(real_LOG_CONTEXT_HANDLE)), void*);

    REGISTER_THANDLE_LOG_CONTEXT_HANDLE_GLOBAL_MOCK_HOOK();

}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();
    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{

}

TEST_FUNCTION(thandle_ptr_log_context_handle_with_mocks)
{
    ///arrange
    STRICT_EXPECTED_CALL(THANDLE_PTR_CREATE_WITH_MOVE(LOG_CONTEXT_HANDLE)((LOG_CONTEXT_HANDLE)0x42, NULL));
    STRICT_EXPECTED_CALL(THANDLE_ASSIGN(PTR(LOG_CONTEXT_HANDLE))(IGNORED_ARG, NULL));

    ///act
    function_under_test();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean;
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
