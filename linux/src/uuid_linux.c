// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>
#include <stdbool.h>

#include <uuid/uuid.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/uuid.h"

static int MU_UNUSED_VAR is_UUID_T_and_UUID_same_size[sizeof(UUID_T) == sizeof(uuid_t)]; /*just a sanity check*/

int uuid_produce(UUID_T destination)
{
    int result;
    /*Codes_SRS_UUID_02_001: [ If destination is NULL then uuid_produce shall fail and return a non-NULL value. ]*/
    /*Codes_SRS_UUID_LINUX_02_001: [ If destination is NULL then uuid_produce shall fail and return a non-NULL value. ]*/
    if (destination == NULL)
    {
        LogError("invalid argument UUID_T destination=%p", destination);
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_UUID_02_002: [ uuid_produce shall generate in destination the representation of a UUID (as per RFC 4122). ]*/
        /*Codes_SRS_UUID_LINUX_02_002: [ uuid_produce shall call uuid_generate to generate a UUID. ]*/
        uuid_generate(destination);

        /*Codes_SRS_UUID_02_004: [ uuid_produce shall succeed and return 0. ]*/
        /*Codes_SRS_UUID_LINUX_02_004: [ uuid_produce shall succeed and return 0. ]*/
        result = 0;
    }
    return result;
}

bool is_uuid_nil(const UUID_T uuid_value)
{
    bool result;
    // Codes_SRS_UUID_LINUX_11_001: [ if uuid_value is NULL then is_uuid_nil shall fail and return false. ]
    if (uuid_value == NULL)
    {
        LogError("invalid argument UUID_T uuid_value=%p", uuid_value);
        result = false;
    }
    else
    {
        if (
            uuid_value[0] == 0 &&
            uuid_value[1] == 0 &&
            uuid_value[2] == 0 &&
            uuid_value[3] == 0 &&
            uuid_value[4] == 0 &&
            uuid_value[5] == 0 &&
            uuid_value[6] == 0 &&
            uuid_value[7] == 0 &&
            uuid_value[8] == 0 &&
            uuid_value[9] == 0 &&
            uuid_value[10] == 0 &&
            uuid_value[11] == 0 &&
            uuid_value[12] == 0 &&
            uuid_value[13] == 0 &&
            uuid_value[14] == 0 &&
            uuid_value[15] == 0
            )
        {
            // Codes_SRS_UUID_LINUX_11_002: [ If all the values of is_uuid_nil are 0 then is_uuid_nil shall return true. ]
            result = true;
        }
        else
        {
            // Codes_SRS_UUID_LINUX_11_003: [ If any the values of is_uuid_nil are not 0 then is_uuid_nil shall return false. ]
            result = false;
        }
    }
    return result;
}
