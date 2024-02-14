//Copyright(c) Microsoft.All rights reserved.
//Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>
#include <stdint.h>

#include "testrunnerswitcher.h"

#include "macro_utils/macro_utils.h"

#include "c_pal/interlocked.h"
#include "c_pal/interlocked_macros.h"

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(a)
{
}

TEST_SUITE_CLEANUP(b)
{
}

TEST_FUNCTION_INITIALIZE(c)
{
}

TEST_FUNCTION_CLEANUP(d)
{
}

#define MY_ENUM_VALUES \
    MY_ENUM_VALUE_1, \
    MY_ENUM_VALUE_2, \
    MY_ENUM_VALUE_3
MU_DEFINE_ENUM(MY_ENUM, MY_ENUM_VALUES);

TEST_DEFINE_ENUM_TYPE(MY_ENUM, MY_ENUM_VALUES);

typedef struct TEST_STRUCT_TAG
{
    INTERLOCKED_DEFINE_VOLATILE_STATE_ENUM(MY_ENUM, state);
} TEST_STRUCT;

// Mostly a case of "if it compiles..."
/*Tests_SRS_INTERLOCKED_MACROS_42_001: [ INTERLOCKED_DEFINE_VOLATILE_STATE_ENUM shall generate a union with two fields: a volatile_atomic int32_t and a variable of the type enum_type. ]*/
TEST_FUNCTION(INTERLOCKED_DEFINE_VOLATILE_STATE_ENUM_works_with_some_enum)
{
    ///arrange
    TEST_STRUCT test_struct;

    ///act
    test_struct.state_enum = MY_ENUM_VALUE_1;
    (void)interlocked_exchange(&test_struct.state, MY_ENUM_VALUE_2);

    ///assert
    size_t size_of_int32_t = sizeof(int32_t);
    size_t size_of_enum = sizeof(MY_ENUM);
    size_t expected_size = (size_of_int32_t > size_of_enum ? size_of_int32_t : size_of_enum);
    ASSERT_ARE_EQUAL(size_t, expected_size, sizeof(test_struct));
    ASSERT_ARE_EQUAL(uint32_t, MY_ENUM_VALUE_2, interlocked_add(&test_struct.state, 0));
    ASSERT_ARE_EQUAL(MY_ENUM, MY_ENUM_VALUE_2, test_struct.state_enum);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
