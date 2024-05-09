// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <string.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/thandle.h"
#include "c_pal/string_utils.h"

#include "c_pal/thandle_ptr.h"

typedef struct A_S_TAG
{
    int a;
    char* s;
}A_S;

typedef A_S* A_S_PTR;

static void dispose(A_S_PTR a_s)
{
    free(a_s->s);
    free(a_s);
}

THANDLE_PTR_DECLARE(A_S_PTR);
THANDLE_PTR_DEFINE(A_S_PTR);

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(it_does_something)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(f)
{
}

TEST_FUNCTION_CLEANUP(cleans)
{
}

/*the test will
1. allocate on the heap some structure that needs a special "destroy" function
2. move the pointer to the structure into a THANDLE(PTR())
3. verify that the pointer in the THANDLE is the same as the original pointer
4. THANDLE_ASSIGN(PTR, NULL) so that memory is freed (the THANDLE memory and the original "destroy"
*/
TEST_FUNCTION(thandle_int_works)
{
    ///arrange
    LogInfo("1. allocate on the heap some structure that needs a special \"destroy\" function.");
    A_S_PTR a_s = malloc(sizeof(A_S));
    ASSERT_IS_NOT_NULL(a_s);
    a_s->a = 42;
    a_s->s = sprintf_char("3333333333333333333333_here_some_string_3333333333333333333333");
    ASSERT_IS_NOT_NULL(a_s->s);

    ///act
    LogInfo("2. move the pointer to the structure into a THANDLE(PTR())");
    THANDLE(PTR(A_S_PTR)) one = THANDLE_PTR_CREATE_WITH_MOVE(A_S_PTR)(a_s, dispose);

    ///assert
    LogInfo("3. verify that the pointer in the THANDLE is the same as the original pointer");
    ASSERT_IS_NOT_NULL(one);
    ASSERT_ARE_EQUAL(void_ptr, a_s, one->pointer);

    ///cleanup
    LogInfo("4. THANDLE_ASSIGN(PTR, NULL) so that memory is freed (the THANDLE memory and the original \"destroy\"");
    THANDLE_ASSIGN(PTR(A_S_PTR))(&one, NULL);

}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

