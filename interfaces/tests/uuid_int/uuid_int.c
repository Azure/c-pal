// Copyright (c) Microsoft. All rights reserved.


#include <stdlib.h>
#include <string.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"
#include "c_logging/xlogging.h"

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

#ifdef WIN32
TEST_FUNCTION(uuid_from_GUID_succeeds)
{
    ///arrange
    GUID g = { 0 };
    (void)UuidCreate(&g);

    UUID_T destination;
    int result;

    ///act
    result = uuid_from_GUID(destination, &g);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(uint8_t, (g.Data1 >> 24) & 0xFF, destination[0]);
    ASSERT_ARE_EQUAL(uint8_t, (g.Data1 >> 16) & 0xFF, destination[1]);
    ASSERT_ARE_EQUAL(uint8_t, (g.Data1 >> 8) & 0xFF, destination[2]);
    ASSERT_ARE_EQUAL(uint8_t, (g.Data1) & 0xFF, destination[3]);

    ASSERT_ARE_EQUAL(uint8_t, (g.Data2>> 8) & 0xFF, destination[4]);
    ASSERT_ARE_EQUAL(uint8_t, (g.Data2) & 0xFF, destination[5]);

    ASSERT_ARE_EQUAL(uint8_t, (g.Data3 >> 8) & 0xFF, destination[6]);
    ASSERT_ARE_EQUAL(uint8_t, (g.Data3 ) & 0xFF, destination[7]);

    ASSERT_ARE_EQUAL(uint8_t, g.Data4[0], destination[8]);
    ASSERT_ARE_EQUAL(uint8_t, g.Data4[1], destination[9]);
    ASSERT_ARE_EQUAL(uint8_t, g.Data4[2], destination[10]);
    ASSERT_ARE_EQUAL(uint8_t, g.Data4[3], destination[11]);
    ASSERT_ARE_EQUAL(uint8_t, g.Data4[4], destination[12]);
    ASSERT_ARE_EQUAL(uint8_t, g.Data4[5], destination[13]);
    ASSERT_ARE_EQUAL(uint8_t, g.Data4[6], destination[14]);
    ASSERT_ARE_EQUAL(uint8_t, g.Data4[7], destination[15]);

}
#endif

TEST_FUNCTION(PRI_UUID_compiles)
{
    ///arrange
    UUID_T u;
    ASSERT_ARE_EQUAL(int, 0, uuid_produce(u));

    ///act (well - it compiles)
    LogInfo("u=%" PRI_UUID "", UUID_VALUES(u));

    ///assert - none
}

static void PRI_UUID_with_NULL_helper(UUID_T u)
{
    LogInfo("u=%" PRI_UUID "", UUID_VALUES_OR_NULL(u));
}

TEST_FUNCTION(PRI_UUID_with_NULL_compiles)
{
    ///arrange

    ///act 1 (well - it compiles, doesn't crash at runtime)
    PRI_UUID_with_NULL_helper(NULL); /*needs a helper to decay array into pointer*/

    ///act 2 (well - it compiles, doesn't crash at runtime)
    LogInfo("some UUID_T=%" PRI_UUID "", UUID_VALUES_OR_NULL(*(UUID_T*)NULL));

    ///assert - none
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
