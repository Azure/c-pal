// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>

#ifdef HAVE_UUID_UUID_H
#include <uuid/uuid.h>
#elif defined(HAVE_UUID_H)
#include <uuid.h>
#endif

#include "macro_utils/macro_utils.h"

#include "c_logging/xlogging.h"

#include "c_pal/uuid.h"


#ifdef HAVE_UUID_UUID_H

static int is_UUID_T_and_UUID_same_size[sizeof(UUID_T) == sizeof(uuid_t)]; /*just a sanity check*/

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

#else

static int is_UUID_T_and_UUID_same_size[sizeof(UUID_T) == UUID_LEN_BIN]; /*just a sanity check*/

int uuid_produce(UUID_T destination)
{
    int result;
    /*Codes_SRS_UUID_02_001: [ If destination is NULL then uuid_produce shall fail and return a non-NULL value. ]*/
    /*Codes_SRS_UUID_LINUX_45_001: [ If destination is NULL then uuid_produce shall fail and return a non-NULL value. ]*/
    if (destination == NULL)
    {
        LogError("invalid argument UUID_T destination=%p", destination);
        result = MU_FAILURE;
    }
    else
    {
        uuid_t * uuid;
        /*Codes_SRS_UUID_LINUX_45_003: [ uuid_produce shall call uuid_create to allocate a uuid. ] */
        uuid_rc_t uuid_result = uuid_create(&uuid);
        if (uuid_result != UUID_RC_OK)
        {
            LogError("uuid_create failed result=%i", uuid_result);
            result = MU_FAILURE;
        }
        else
        {
            /*Codes_SRS_UUID_LINUX_45_004: [ uuid_produce shall call uuid_make with mode = UUID_MAKE_V4 to generate the UUID value. ] */
            uuid_result = uuid_make(uuid, UUID_MAKE_V4);
            if (uuid_result != UUID_RC_OK)
            {
                LogError("uuid_make failed result=%i", uuid_result);
                result = MU_FAILURE;
            }
            else
            {
                /*Codes_SRS_UUID_02_002: [ uuid_produce shall generate in destination the representation of a UUID (as per RFC 4122). ]*/
                /*Codes_SRS_UUID_LINUX_45_005: [ uuid_produce shall call uuid_export with fmt = UUID_FMT_BIN to load the value into the destination. ] */
                size_t destination_size = sizeof(UUID_T);
                uuid_result = uuid_export(uuid, UUID_FMT_BIN, &destination, &destination_size);
                if (uuid_result != UUID_RC_OK || destination_size != sizeof(UUID_T))
                {
                    /*Codes_SRS_UUID_LINUX_45_007: [ If any uuid call returns a value other than UUID_RC_OK, the call shall fail and return a non-zero value. ]*/
                    /*Codes_SRS_UUID_LINUX_45_008: [ If uuid_export does not write size_of(UUID_T) bytes into the destination, the call shall fail and return a non-zero value. ]*/
                    LogError("uuid_export failed result=%i, wrote %" PRIu64 " bytes", uuid_result, destination_size);
                    result = MU_FAILURE;
                }
                else
                {
                    /*Codes_SRS_UUID_02_004: [ uuid_produce shall succeed and return 0. ]*/
                    /*Codes_SRS_UUID_LINUX_45_002: [ If all uuid calls return success, uuid_produce shall succeed and return 0. ] */
                    result = 0;
                }
            }
            /*Codes_SRS_UUID_LINUX_45_006: [ uuid_produce shall call uuid_destroy to deallocate the uuid. ] */
            (void)uuid_destroy(uuid);
        }
    }
    return result;
}

#endif // HAVE_UUID_UUID_H