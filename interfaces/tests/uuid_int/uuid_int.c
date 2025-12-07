// Copyright (c) Microsoft. All rights reserved.

#include <stdlib.h>
#include <string.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"
#include "c_logging/logger.h"

#include "c_pal/uuid.h"

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
}

TEST_FUNCTION_INITIALIZE(method_init)
{
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
}

/*Tests_SRS_UUID_02_001: [ If destination is NULL then uuid_produce shall fail and return a non-zero value. ]*/
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
    result1 = uuid_produce(&one);
    result2 = uuid_produce(&two);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result1);
    ASSERT_ARE_EQUAL(int, 0, result2);
    ASSERT_IS_FALSE(UUID_T_IS_EQUAL(one, two));
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
    result = uuid_from_GUID(&destination, &g);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(uint8_t, (g.Data1 >> 24) & 0xFF, destination.bytes[0]);
    ASSERT_ARE_EQUAL(uint8_t, (g.Data1 >> 16) & 0xFF, destination.bytes[1]);
    ASSERT_ARE_EQUAL(uint8_t, (g.Data1 >>  8) & 0xFF, destination.bytes[2]);
    ASSERT_ARE_EQUAL(uint8_t, (g.Data1      ) & 0xFF, destination.bytes[3]);

    ASSERT_ARE_EQUAL(uint8_t, (g.Data2>> 8) & 0xFF, destination.bytes[4]);
    ASSERT_ARE_EQUAL(uint8_t, (g.Data2    ) & 0xFF, destination.bytes[5]);

    ASSERT_ARE_EQUAL(uint8_t, (g.Data3 >> 8) & 0xFF, destination.bytes[6]);
    ASSERT_ARE_EQUAL(uint8_t, (g.Data3     ) & 0xFF, destination.bytes[7]);

    ASSERT_ARE_EQUAL(uint8_t, g.Data4[0], destination.bytes[8]);
    ASSERT_ARE_EQUAL(uint8_t, g.Data4[1], destination.bytes[9]);
    ASSERT_ARE_EQUAL(uint8_t, g.Data4[2], destination.bytes[10]);
    ASSERT_ARE_EQUAL(uint8_t, g.Data4[3], destination.bytes[11]);
    ASSERT_ARE_EQUAL(uint8_t, g.Data4[4], destination.bytes[12]);
    ASSERT_ARE_EQUAL(uint8_t, g.Data4[5], destination.bytes[13]);
    ASSERT_ARE_EQUAL(uint8_t, g.Data4[6], destination.bytes[14]);
    ASSERT_ARE_EQUAL(uint8_t, g.Data4[7], destination.bytes[15]);
}

TEST_FUNCTION(GUID_from_uuid_succeeds)
{
    ///arrange
    UUID_T u;
    (void)uuid_produce(&u);

    GUID destination = { 0 };
    int result;

    ///act
    result = GUID_from_uuid(&destination, u);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(uint8_t, u.bytes[0], (destination.Data1>>24) & 0xFF);
    ASSERT_ARE_EQUAL(uint8_t, u.bytes[1], (destination.Data1 >> 16) & 0xFF);
    ASSERT_ARE_EQUAL(uint8_t, u.bytes[2], (destination.Data1 >> 8) & 0xFF);
    ASSERT_ARE_EQUAL(uint8_t, u.bytes[3], (destination.Data1     ) & 0xFF);
    ASSERT_ARE_EQUAL(uint8_t, u.bytes[4], (destination.Data2 >> 8) & 0xFF);
    ASSERT_ARE_EQUAL(uint8_t, u.bytes[5], (destination.Data2     ) & 0xFF);
    ASSERT_ARE_EQUAL(uint8_t, u.bytes[6], (destination.Data3 >> 8) & 0xFF);
    ASSERT_ARE_EQUAL(uint8_t, u.bytes[7], (destination.Data3     ) & 0xFF);
    ASSERT_ARE_EQUAL(uint8_t, u.bytes[8],  destination.Data4[0]);
    ASSERT_ARE_EQUAL(uint8_t, u.bytes[9],  destination.Data4[1]);
    ASSERT_ARE_EQUAL(uint8_t, u.bytes[10], destination.Data4[2]);
    ASSERT_ARE_EQUAL(uint8_t, u.bytes[11], destination.Data4[3]);
    ASSERT_ARE_EQUAL(uint8_t, u.bytes[12], destination.Data4[4]);
    ASSERT_ARE_EQUAL(uint8_t, u.bytes[13], destination.Data4[5]);
    ASSERT_ARE_EQUAL(uint8_t, u.bytes[14], destination.Data4[6]);
    ASSERT_ARE_EQUAL(uint8_t, u.bytes[15], destination.Data4[7]);
}

TEST_FUNCTION(PRI_GUID_succeeds)
{
    GUID g = { 0x10213243, 0x5465, 0x7687, {0x98, 0xA9, 0xBA, 0xCB, 0xDE, 0xED, 0xFE, 0x0F } };
    char temp[1000];  /*a vast array greatly bigger than the stringification of GUID*/
    int r = snprintf(temp, sizeof(temp), "%" PRI_GUID "", GUID_VALUES(g));
    ASSERT_IS_TRUE((r >= 0) && (r < sizeof(temp)));

    ASSERT_ARE_EQUAL(char_ptr, "10213243-5465-7687-98a9-bacbdeedfe0f", temp);
}

TEST_FUNCTION(PRI_GUID_with_NULL_succeeds)
{
    GUID* g = NULL;
    char temp[1000];  /*a vast array greatly bigger than the stringification of GUID*/
    int r = snprintf(temp, sizeof(temp), "%" PRI_GUID "", GUID_VALUES_OR_NULL(g));
    ASSERT_IS_TRUE((r >= 0) && (r < sizeof(temp)));

    ASSERT_ARE_EQUAL(char_ptr, "00000000-0000-0000-0000-000000000000", temp);
}

TEST_FUNCTION(PRI_GUID_and_friends)
{
    ///arrange
    struct GUID_AND_EXPECTED_STRINGS
    {
        GUID guid;
        const char* expectedGuidAsString;
    } guidAndExpectedStrings[] =
    {
        {/*[0]*/
            {
                0,                                          /*unsigned long  Data1;     */
                0,                                          /*unsigned short Data2;     */
                0,                                          /*unsigned short Data3;     */
                {0,0,0,0,0,0,0,0}                           /*unsigned char  Data4[8];  */
            },
            "00000000-0000-0000-0000-000000000000"
        },
        {/*[1]*/
            {
                0xFFFFFFFF,                                 /*unsigned long  Data1;     */
                0xFFFF,                                     /*unsigned short Data2;     */
                0xFFFF,                                     /*unsigned short Data3;     */
                {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}   /*unsigned char  Data4[8];  */
            },
            "ffffffff-ffff-ffff-ffff-ffffffffffff"
        },
        {/*[2]*/
            /*a most famous bug that needed testing - it was producing 1f018f1a-1b1f-40ad-b78d-d577f2b27821
            instead of the expected                                    1f018f1a-1b1f-40ad-b78d-d578f2b27821 and we had great fun with it!*/
            {
                0x1f018f1a,                                 /*unsigned long  Data1;     */
                0x1b1f,                                     /*unsigned short Data2;     */
                0x40ad,                                     /*unsigned short Data3;     */
                {0xb7,0x8d,0xd5,0x78,0xf2,0xb2,0x78,0x21}   /*unsigned char  Data4[8];  */
            },
            "1f018f1a-1b1f-40ad-b78d-d578f2b27821"
        }
    };

    ///act
    for (size_t i = 0; i < sizeof(guidAndExpectedStrings) / sizeof(guidAndExpectedStrings[0]); i++)
    {
        char temp[1000];
        int r = snprintf(temp, sizeof(temp), "%" PRI_GUID "", GUID_VALUES(guidAndExpectedStrings[i].guid));
        ASSERT_IS_TRUE((r >= 0) && (r < sizeof(temp)));

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, guidAndExpectedStrings[i].expectedGuidAsString, temp);

        ///clean
    }
}

#endif

TEST_FUNCTION(PRI_UUID_T_compiles)
{
    ///arrange
    UUID_T u;
    ASSERT_ARE_EQUAL(int, 0, uuid_produce(&u));

    ///act (well - it compiles)
    LogInfo("u=%" PRI_UUID_T "", UUID_T_VALUES(u));

    ///assert - none
}

static void UUID_T_VALUES_OR_NULL_helper(UUID_T u)
{
    LogInfo("u=%" PRI_UUID_T "", UUID_T_VALUES(u));
}

TEST_FUNCTION(UUID_T_VALUES_OR_NULL_compiles)
{
    ///arrange
    UUID_T u;
    ASSERT_ARE_EQUAL(int, 0, uuid_produce(&u));

    ///act 1 (well - it compiles, doesn't crash at runtime)
    LogInfo("NULL UUID_T*=%" PRI_UUID_T "", PUUID_T_VALUES_OR_NULL(NULL));

    ///act 2 (well - it compiles, doesn't crash at runtime)
    LogInfo("u=%" PRI_UUID_T "", UUID_T_VALUES(u));

    ///assert - none
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
