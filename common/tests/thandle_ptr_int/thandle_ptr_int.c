// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "c_logging/logger.h"

#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"               // IWYU pragma: keep
#include "c_pal/gballoc_hl_redirect.h"      // IWYU pragma: keep
#include "c_pal/thandle.h"
#include "c_pal/string_utils.h"

#include "c_pal/thandle_ll.h"
#include "c_pal/thandle.h"                  // IWYU pragma: keep
#include "c_pal/thandle_ptr.h"

#include "example.h"
#include "example_incomplete_type.h"

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


typedef struct A_S_CONST_TAG
{
    const int a;
    const char* s;
}A_S_CONST;

typedef A_S_CONST* A_S_CONST_PTR;

static void dispose_const(A_S_CONST_PTR a_s)
{
    free((void*)a_s->s);
    free(a_s);
}

THANDLE_PTR_DECLARE(A_S_CONST_PTR);
THANDLE_PTR_DEFINE(A_S_CONST_PTR);

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
3.1 verify that the pointer has the same qualifiers as the original pointer (in this case fields are not const)
4. THANDLE_ASSIGN(PTR, NULL) so that memory is freed (the THANDLE memory and the original "destroy"
*/
TEST_FUNCTION(thandle_int_works_with_both_declare_and_define_in_this_file)
{
    ///arrange
    LogInfo("1. allocate on the heap some structure that needs a special \"destroy\" function.");
    A_S_PTR a_s = malloc(sizeof(A_S));
    ASSERT_IS_NOT_NULL(a_s);
    a_s->a = 42;
    a_s->s = sprintf_char("%s", "3333333333333333333333_here_some_string_3333333333333333333333");
    ASSERT_IS_NOT_NULL(a_s->s);
    A_S_PTR original_a_s = a_s; /*needed to verification later*/

    ///act
    LogInfo("2. move the pointer to the structure into a THANDLE(PTR())");
    THANDLE(PTR(A_S_PTR)) one = THANDLE_PTR_CREATE_WITH_MOVE(A_S_PTR)(&a_s, dispose);

    ///assert
    LogInfo("3. verify that the pointer in the THANDLE is the same as the original pointer");
    ASSERT_IS_NOT_NULL(one);
    ASSERT_IS_NULL(a_s);
    ASSERT_ARE_EQUAL(void_ptr, original_a_s, one->pointer);

    ///assert
    LogInfo("3.1 verify that the pointer has the same qualifiers as the original pointer (in this case fields are not const)");
    one->pointer->a = 3;
    one->pointer->s[0] = 'a';

    ///cleanup
    LogInfo("4. THANDLE_ASSIGN(PTR, NULL) so that memory is freed (the THANDLE memory and the original \"destroy\"");
    THANDLE_ASSIGN(PTR(A_S_PTR))(&one, NULL);
}

/*the test will
1. allocate on the heap some structure that needs a special "destroy" function
2. move the pointer to the structure into a THANDLE(PTR())
3. verify that the pointer in the THANDLE is the same as the original pointer
3.1 verify that the pointer has the same qualifiers as the original pointer (in this case fields are const)
4. THANDLE_ASSIGN(PTR, NULL) so that memory is freed (the THANDLE memory and the original "destroy"
*/
TEST_FUNCTION(thandle_int_works_with_both_declare_and_define_in_this_file_for_const)
{
    ///arrange
    LogInfo("1. allocate on the heap some structure that needs a special \"destroy\" function.");
    A_S_CONST_PTR a_s = malloc(sizeof(A_S_CONST));
    ASSERT_IS_NOT_NULL(a_s);
    /*cast the const away... just for a tiny momnet*/
    *(int*)& a_s->a = 42;
    *(char**)&a_s->s = sprintf_char("%s", "3333333333333333333333_here_some_string_3333333333333333333333");
    ASSERT_IS_NOT_NULL(a_s->s);
    A_S_CONST_PTR original_a_s = a_s; /*needed to verification later*/

    ///act
    LogInfo("2. move the pointer to the structure into a THANDLE(PTR())");
    THANDLE(PTR(A_S_CONST_PTR)) one = THANDLE_PTR_CREATE_WITH_MOVE(A_S_CONST_PTR)(&a_s, dispose_const);

    ///assert
    LogInfo("3. verify that the pointer in the THANDLE is the same as the original pointer");
    ASSERT_IS_NOT_NULL(one);
    ASSERT_IS_NULL(a_s);
    ASSERT_ARE_EQUAL(void_ptr, original_a_s, one->pointer);

    ///assert
    LogInfo("3.1 verify that the pointer has the same qualifiers as the original pointer (in this case fields are const)");
    /*one->pointer->a = 3;*/ /*error C2166: l-value specifies const object*/
    /*one->pointer->s[0] = 'a';*/ /*error C2166: l-value specifies const object*/

    /*ENTER C GENERICS - with a shout out to "compatible types..." Note: it seems that "const int" and "int" are "compatible", however, int* and const int* are not.*/
    ASSERT_ARE_EQUAL(int, 1, _Generic(&one->pointer->a, const int *: 1, int*: 2, default : 0));
    ASSERT_ARE_EQUAL(int, 1, _Generic(&(one->pointer->s), const char** : 1, default: 0));

    ///cleanup
    LogInfo("4. THANDLE_ASSIGN(PTR, NULL) so that memory is freed (the THANDLE memory and the original \"destroy\"");
    THANDLE_ASSIGN(PTR(A_S_CONST_PTR))(&one, NULL);
}

TEST_FUNCTION(thandle_int_works_with_both_declare_and_define_in_different_files)
{
    ///arrange
    EXAMPLE_COMPLETE_PTR p = malloc(sizeof(EXAMPLE_COMPLETE));
    ASSERT_IS_NOT_NULL(p);
    p->example_complete = 2;
    EXAMPLE_COMPLETE_PTR original_p = p;

    ///act
    THANDLE(PTR(EXAMPLE_COMPLETE_PTR)) example_complete = THANDLE_PTR_CREATE_WITH_MOVE(EXAMPLE_COMPLETE_PTR)(&p, dispose_example_complete);

    ///assert
    ASSERT_IS_NOT_NULL(example_complete);
    ASSERT_IS_NULL(p);
    ASSERT_ARE_EQUAL(void_ptr, original_p, example_complete->pointer);
    ASSERT_ARE_EQUAL(int, 2, example_complete->pointer->example_complete);

    ///cleanup
    THANDLE_ASSIGN(PTR(EXAMPLE_COMPLETE_PTR))(&example_complete, NULL);

}

TEST_FUNCTION(thandle_int_works_with_incomplete_types)
{
    ///arrange
    EXAMPLE_INCOMPLETE_PTR p = create_example_incomplete();
    ASSERT_IS_NOT_NULL(p);
    EXAMPLE_INCOMPLETE_PTR original_p = p;

    ///act
    THANDLE(PTR(EXAMPLE_INCOMPLETE_PTR)) example_incomplete = THANDLE_PTR_CREATE_WITH_MOVE(EXAMPLE_INCOMPLETE_PTR)(&p, dispose_example_incomplete);

    ///assert
    ASSERT_IS_NOT_NULL(example_incomplete);
    ASSERT_IS_NULL(p);
    ASSERT_ARE_EQUAL(void_ptr, original_p, example_incomplete->pointer);

    ///cleanup
    THANDLE_ASSIGN(PTR(EXAMPLE_INCOMPLETE_PTR))(&example_incomplete, NULL);

}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

