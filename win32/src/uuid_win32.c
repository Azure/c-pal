// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "rpc.h"

#include "macro_utils/macro_utils.h"

#include "c_logging/xlogging.h"

#include "c_pal/uuid.h"

static int is_UUID_T_and_UUID_same_size[sizeof(UUID_T) == sizeof(UUID)]; /*just a sanity check*/

/*these are interface requirements*/
/*Codes_SRS_UUID_02_001: [ If destination is NULL then uuid_produce shall fail and return a non-NULL value. ]*/
/*Codes_SRS_UUID_02_002: [ uuid_produce shall generate in destination the representation of a UUID (as per RFC 4122). ]*/
/*Codes_SRS_UUID_02_004: [ uuid_produce shall succeed and return 0. ]*/

int uuid_produce(UUID_T destination)
{
    int result;
    /*Codes_SRS_UUID_WIN32_02_001: [ If destination is NULL then uuid_produce shall fail and return a non-NULL value. ]*/
    if (destination == NULL)
    {
        LogError("invalid argument UUID_T destination=%p", destination);
        result = MU_FAILURE;
    }
    else
    {
        UUID u;
        /*Codes_SRS_UUID_WIN32_02_002: [ uuid_produce shall call UuidCreate to generate a UUID. ]*/
        RPC_STATUS rpc_status = UuidCreate(&u);
        if (!(
            (rpc_status == RPC_S_OK) ||
            (rpc_status == RPC_S_UUID_LOCAL_ONLY)
            ))
        {
            /*Codes_SRS_UUID_WIN32_02_005: [ If there are any failures then uuid_produce shall fail and return a non-zero value. ]*/
            LogError("failure in UuidCreate(&u), rpc_status=%ld", rpc_status);
            result = MU_FAILURE;
        }
        else
        {
            /*Codes_SRS_UUID_WIN32_02_003: [ uuid_produce shall copy the generated UUID's bytes in destination. ]*/
            destination[ 0] = (u.Data1 >> 24) & 0xFF;
            destination[ 1] = (u.Data1 >> 16) & 0xFF;
            destination[ 2] = (u.Data1 >>  8) & 0xFF;
            destination[ 3] = (u.Data1      ) & 0xFF;
            destination[ 4] = (u.Data2 >>  8) & 0xFF;
            destination[ 5] = (u.Data2      ) & 0xFF;
            destination[ 6] = (u.Data3 >>  8) & 0xFF;
            destination[ 7] = (u.Data3      ) & 0xFF;
            destination[ 8] =  u.Data4[0];
            destination[ 9] =  u.Data4[1];
            destination[10] =  u.Data4[2];
            destination[11] =  u.Data4[3];
            destination[12] =  u.Data4[4];
            destination[13] =  u.Data4[5];
            destination[14] =  u.Data4[6];
            destination[15] =  u.Data4[7];

            /*Codes_SRS_UUID_WIN32_02_004: [ uuid_produce shall succeed and return 0. ]*/
            result = 0;
        }
    }
    return result;
}
