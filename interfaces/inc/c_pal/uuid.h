// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef UUID_H
#define UUID_H

#ifdef __cplusplus
#include <cinttypes>
#include <cstdbool>
#include <cstdalign>
#else
#include <inttypes.h>
#include <stdbool.h>
#include <stdalign.h>
#endif

#ifdef WIN32
#include "windows.h"
#endif

#include "macro_utils/macro_utils.h"

typedef struct UUID_T_TAG
{
    alignas(8) uint8_t bytes[16];
} UUID_T;

/*cannot function without these basic asserts*/
MU_STATIC_ASSERT(sizeof(UUID_T) == 16);
MU_STATIC_ASSERT(alignof(UUID_T) == 8);

#define UUID_T_IS_EQUAL(a, b) \
    ((((const uint64_t*)&(a))[0] == ((const uint64_t*)&(b))[0]) && \
     (((const uint64_t*)&(a))[1] == ((const uint64_t*)&(b))[1]))

#define UUID_T_IS_NIL(a) \
    ((((const uint64_t*)&(a))[0] == 0) && \
     (((const uint64_t*)&(a))[1] == 0))

typedef const UUID_T const_UUID_T;

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

    MOCKABLE_FUNCTION_WITH_RETURNS(, int, uuid_produce, UUID_T*, destination)(0, MU_FAILURE);
    MOCKABLE_FUNCTION(, bool, is_uuid_nil, UUID_T, uuid_value);

#ifdef WIN32 /*some functions, format specifiers only exists in Windows realm*/
    MOCKABLE_FUNCTION(, int, uuid_from_GUID, UUID_T*, destination, const GUID*, source);
    MOCKABLE_FUNCTION(, int, GUID_from_uuid, GUID*, destination, UUID_T, source);

/*
below macros can be used with printf. example:
printf("PartitionId = %" GUID_FORMAT "\n", GUID_VALUES(fabricDeployedStatefulServiceReplicaQueryResultItem->PartitionId)); produces on the screen:
PartitionId=316132b8-96a0-4bc7-aecc-a16e7c5a6bf6
*/

#define PRI_GUID "8.8" PRIx32 "-%4.4" PRIx16 "-%4.4" PRIx16 "-%4.4" PRIx16 "-%12.12" PRIx64
#define GUID_VALUES(guid) \
    (guid).Data1, \
    (guid).Data2, \
    (guid).Data3, \
    ((guid).Data4[0]<<8) + (guid).Data4[1], \
    ((uint64_t)((guid).Data4[2])<<40) + ((uint64_t)((guid).Data4[3])<<32) + (((uint64_t)(guid).Data4[4])<<24) + ((guid).Data4[5]<<16) + ((guid).Data4[6]<<8) + ((guid).Data4[7])

#define GUID_VALUES_OR_NULL(pguid) \
    (pguid == NULL) ? 0 : (pguid)->Data1, \
    (pguid == NULL) ? 0 : (pguid)->Data2, \
    (pguid == NULL) ? 0 : (pguid)->Data3, \
    (pguid == NULL) ? 0 : ((pguid)->Data4[0] << 8) + (pguid)->Data4[1], \
    (pguid == NULL) ? 0 : ((uint64_t)((pguid)->Data4[2]) << 40) + ((uint64_t)((pguid)->Data4[3]) << 32) + (((uint64_t)(pguid)->Data4[4]) << 24) + ((pguid)->Data4[5] << 16) + ((pguid)->Data4[6] << 8) + ((pguid)->Data4[7])

/*for backward compatibility*/
#define GUID_FORMAT PRI_GUID
#endif

/* These 2 strings can be conveniently used directly in printf-like statements
  Notice that PRI_UUID has to be used like any other print format specifier, meaning it
  has to be preceded with % */

#define PRI_UUID_T        "02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x"

#define UUID_T_VALUES(uuid) \
    (uuid).bytes[0], (uuid).bytes[1], (uuid).bytes[2],  (uuid).bytes[3],  (uuid).bytes[4],  (uuid).bytes[5],  (uuid).bytes[6],  (uuid).bytes[7], \
    (uuid).bytes[8], (uuid).bytes[9], (uuid).bytes[10], (uuid).bytes[11], (uuid).bytes[12], (uuid).bytes[13], (uuid).bytes[14], (uuid).bytes[15]

#define PUUID_T_VALUES_OR_NULL(uuid) \
    ((uuid) == NULL) ? 0 : (uuid)->bytes[0],  ((uuid) == NULL) ? 0 : (uuid)->bytes[1],  ((uuid) == NULL) ? 0 : (uuid)->bytes[2],  ((uuid) == NULL) ? 0 : (uuid)->bytes[3], \
    ((uuid) == NULL) ? 0 : (uuid)->bytes[4],  ((uuid) == NULL) ? 0 : (uuid)->bytes[5],  ((uuid) == NULL) ? 0 : (uuid)->bytes[6],  ((uuid) == NULL) ? 0 : (uuid)->bytes[7], \
    ((uuid) == NULL) ? 0 : (uuid)->bytes[8],  ((uuid) == NULL) ? 0 : (uuid)->bytes[9],  ((uuid) == NULL) ? 0 : (uuid)->bytes[10], ((uuid) == NULL) ? 0 : (uuid)->bytes[11], \
    ((uuid) == NULL) ? 0 : (uuid)->bytes[12], ((uuid) == NULL) ? 0 : (uuid)->bytes[13], ((uuid) == NULL) ? 0 : (uuid)->bytes[14], ((uuid) == NULL) ? 0 : (uuid)->bytes[15] \

#ifdef __cplusplus
}
#endif

#endif /*UUID_H*/
