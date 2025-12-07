// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>
#include <stdbool.h>

#include <uuid/uuid.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/uuid.h"

MU_STATIC_ASSERT(sizeof(UUID_T) == sizeof(uuid_t)); /*just a sanity check*/
MU_STATIC_ASSERT(alignof(UUID_T) >= alignof(uuid_t)); /*just a sanity check*/

int uuid_produce(UUID_T* destination)
{
    int result;
    /*Codes_SRS_UUID_02_001: [ If destination is NULL then uuid_produce shall fail and return a non-zero value. ]*/
    /*Codes_SRS_UUID_LINUX_02_001: [ If destination is NULL then uuid_produce shall fail and return a non-zero value. ]*/
    if (destination == NULL)
    {
        LogError("invalid argument UUID_T destination=%p", destination);
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_UUID_02_002: [ uuid_produce shall generate in destination the representation of a UUID (as per RFC 4122). ]*/
        /*Codes_SRS_UUID_LINUX_02_002: [ uuid_produce shall call uuid_generate to generate a UUID. ]*/
        uuid_generate(&destination);

        /*Codes_SRS_UUID_02_004: [ uuid_produce shall succeed and return 0. ]*/
        /*Codes_SRS_UUID_LINUX_02_004: [ uuid_produce shall succeed and return 0. ]*/
        result = 0;
    }
    return result;
}

bool is_uuid_nil(const UUID_T uuid_value)
{
    bool result;

    // Codes_SRS_UUID_LINUX_11_002: [ If all the values of is_uuid_nil are 0 then is_uuid_nil shall return true. ]
    // Codes_SRS_UUID_LINUX_11_003: [ If any the values of is_uuid_nil are not 0 then is_uuid_nil shall return false. ]
    result = UUID_T_IS_NIL(uuid_value);

    return result;
}
