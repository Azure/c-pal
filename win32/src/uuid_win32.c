// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "rpc.h"

#include "macro_utils/macro_utils.h"

#include "c_logging/xlogging.h"

#include "c_pal/uuid.h"

static int is_UUID_T_and_UUID_same_size[sizeof(UUID_T) == sizeof(UUID)]; /*just a sanity check*/

int uuid_produce(UUID_T destination)
{
    int result;
    if (destination == NULL)
    {
        LogError("invalid argument UUID_T destination=%p", destination);
        result = MU_FAILURE;
    }
    else
    {
        UUID u;
        RPC_STATUS rpc_status = UuidCreate(&u);
        if (!(
            (rpc_status == RPC_S_OK) ||
            (rpc_status == RPC_S_UUID_LOCAL_ONLY)
            ))
        {
            LogError("failure in UuidCreate(&u), rpc_status=%ld", rpc_status);
            result = MU_FAILURE;
        }
        else
        {
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
            result = 0;
        }
    }
    return result;
}
