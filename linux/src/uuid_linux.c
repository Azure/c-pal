// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>

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
