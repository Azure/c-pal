// Copyright (c) Microsoft. All rights reserved.


#include <stdlib.h>
#include <string.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"

#include "c_pal/uuid.h"


static TEST_MUTEX_HANDLE test_serialize_mutex;


BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    test_serialize_mutex = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(test_serialize_mutex);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    TEST_MUTEX_DESTROY(test_serialize_mutex);
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    if (TEST_MUTEX_ACQUIRE(test_serialize_mutex))
    {
        ASSERT_FAIL("Could not acquire test serialization mutex.");
    }
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    TEST_MUTEX_RELEASE(test_serialize_mutex);
}

/*Tests_SRS_UUID_02_001: [ If destination is NULL then uuid_produce shall fail and return a non-NULL value. ]*/
TEST_FUNCTION(uuid_produce_succeeds_with_destination_NULL)
{
    ///arrange
    int result;

    ///act
    result = uuid_produce(NULL);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}


/*Tests_SRS_UUID_02_002: [ uuid_produce shall generate in destination the representation of a UUID (as per RFC 4122). ]*/
/*Tests_SRS_UUID_02_004: [ uuid_produce shall succeed and return 0. ]*/
TEST_FUNCTION(uuid_produce_succeeds_and_generates_different_UUIDs)
{
    ///arrange
    UUID_T one;
    UUID_T two;
    int result1, result2;

    ///act
    result1 = uuid_produce(one);
    result2 = uuid_produce(two);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result1);
    ASSERT_ARE_EQUAL(int, 0, result2);
    ASSERT_IS_TRUE(memcmp(one, two, sizeof(UUID_T)) != 0);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
