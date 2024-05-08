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

#include "thandle_ptr_user.h"

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

/*QWQWQW*/
#if 1
THANDLE_PTR_DECLARE(A_S_PTR);
/*WEWEWE*/
THANDLE_PTR_DEFINE(A_S_PTR);
/*ERERER*/
#else
#endif

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



TEST_FUNCTION(thandle_int_works)
{
    ///arrange
    A_S_PTR a_s = malloc(sizeof(A_S));
    ASSERT_IS_NOT_NULL(a_s);
    a_s->a = 42;
    a_s->s = sprintf_char("3333333333333333333333_here_some_string_3333333333333333333333");
    ASSERT_IS_NOT_NULL(a_s->s);


    THANDLE_PTR_FREE_FUNC_TYPE_NAME(A_S_PTR) z = NULL;
    *(volatile int*)&(int) { 1 } ? (void)0 : z(a_s);

    ///act
    THANDLE(PTR_STRUCT_TYPE_NAME(A_S_PTR)) one = create(a_s, dispose);

    ///assert
    ASSERT_IS_NOT_NULL(one);
    ASSERT_ARE_EQUAL(void_ptr, a_s, one->pointer);

    ///cleanup
    THANDLE_ASSIGN(PTR_STRUCT_TYPE_NAME(A_S_PTR))(&one, NULL);

}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

