// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "rpc.h"

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/uuid.h"

static int is_UUID_T_and_UUID_same_size[sizeof(UUID_T) == sizeof(UUID)]; /*just a sanity check*/

/*these are interface requirements*/
/*Codes_SRS_UUID_02_001: [ If destination is NULL then uuid_produce shall fail and return a non-NULL value. ]*/
/*Codes_SRS_UUID_02_002: [ uuid_produce shall generate in destination the representation of a UUID (as per RFC 4122). ]*/
/*Codes_SRS_UUID_02_004: [ uuid_produce shall succeed and return 0. ]*/

static void GUID_to_UUID_T(const GUID* guid, UUID_T uuid)
{
    uuid[0] = (guid->Data1 >> 24) & 0xFF;
    uuid[1] = (guid->Data1 >> 16) & 0xFF;
    uuid[2] = (guid->Data1 >> 8) & 0xFF;
    uuid[3] = (guid->Data1) & 0xFF;
    uuid[4] = (guid->Data2 >> 8) & 0xFF;
    uuid[5] = (guid->Data2) & 0xFF;
    uuid[6] = (guid->Data3 >> 8) & 0xFF;
    uuid[7] = (guid->Data3) & 0xFF;
    uuid[8] = guid->Data4[0];
    uuid[9] = guid->Data4[1];
    uuid[10] = guid->Data4[2];
    uuid[11] = guid->Data4[3];
    uuid[12] = guid->Data4[4];
    uuid[13] = guid->Data4[5];
    uuid[14] = guid->Data4[6];
    uuid[15] = guid->Data4[7];
}

int uuid_produce(UUID_T destination)
{
    int result;
    /*Codes_SRS_UUID_02_001: [ If destination is NULL then uuid_produce shall fail and return a non-NULL value. ]*/
    /*Codes_SRS_UUID_WIN32_02_001: [ If destination is NULL then uuid_produce shall fail and return a non-NULL value. ]*/
    if (destination == NULL)
    {
        LogError("invalid argument UUID_T destination=%p", destination);
        result = MU_FAILURE;
    }
    else
    {
        UUID u;
        /*Codes_SRS_UUID_02_002: [ uuid_produce shall generate in destination the representation of a UUID (as per RFC 4122). ]*/
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
            GUID_to_UUID_T(&u, destination);

            /*Codes_SRS_UUID_02_004: [ uuid_produce shall succeed and return 0. ]*/
            /*Codes_SRS_UUID_WIN32_02_004: [ uuid_produce shall succeed and return 0. ]*/
            result = 0;
        }
    }
    return result;
}

int uuid_from_GUID(UUID_T destination, const GUID* source)
{
    int result;
    if (
        /*Codes_SRS_UUID_WIN32_02_006: [ If destination is NULL then uuid_from_GUID shall fail and return a non-zero value. ]*/
        (destination == NULL) ||
        /*Codes_SRS_UUID_WIN32_02_007: [ If source is NULL then uuid_from_GUID shall fail and return a non-zero value. ]*/
        (source == NULL)
        )
    {
        LogError("invalid arguments UUID_T destination=%p, const GUID* source=%p", destination, source);
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_UUID_WIN32_02_008: [ uuid_from_GUID shall convert GUID to UUID_T, succeed and return 0. ]*/
        GUID_to_UUID_T(source, destination);
        result = 0;
    }
    return result;
}

int GUID_from_uuid(GUID* destination, const UUID_T source)
{
    int result;
    if (
        /*Codes_SRS_UUID_WIN32_02_009: [ If destination is NULL then GUID_from_uuid shall fail and return a non-zero value. ]*/
        (destination == NULL) ||
        /*Codes_SRS_UUID_WIN32_02_010: [ If source is NULL then GUID_from_uuid shall fail and return a non-zero value. ]*/
        (source == NULL)
        )
    {
        LogError("invalid argument GUID* destination=%" PRI_GUID ", const UUID_T source=%" PRI_UUID_T "", GUID_VALUES_OR_NULL(destination), UUID_T_VALUES_OR_NULL(source));
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_UUID_WIN32_02_011: [ GUID_from_uuid shall convert UUID_T to GUID, succeed and return 0. ]*/
        destination->Data1 =
            (source[0] << 24) +
            (source[1] << 16) +
            (source[2] << 8) +
            (source[3]);
        destination->Data2 =
            (source[4] << 8) +
            (source[5]);
        destination->Data3 =
            (source[6] << 8) +
            (source[7]);
        (void)memcpy(destination->Data4, source + 8, 8);
        result = 0;
    }
    return result;
}
