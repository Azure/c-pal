// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef UUID_H
#define UUID_H

#include <inttypes.h>
#include <stdbool.h>

#ifdef WIN32 
#include "windows.h"
#endif

#define UUID_T_LENGTH       16
typedef unsigned char UUID_T[UUID_T_LENGTH]; /*introduces UUID_T as "array of 16 bytes"*/

#include "macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

    MOCKABLE_FUNCTION(, int, uuid_produce, UUID_T, destination);
    MOCKABLE_FUNCTION(, bool, is_uuid_nil, const UUID_T, uuid_value);

#ifdef WIN32 /*some functions, format specifiers only exists in Windows realm*/
    MOCKABLE_FUNCTION(, int, uuid_from_GUID, UUID_T, destination, const GUID*, source);
    MOCKABLE_FUNCTION(, int, GUID_from_uuid, GUID*, destination, const UUID_T, source);

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
    (uuid)[0], (uuid)[1], (uuid)[2], (uuid)[3], (uuid)[4], (uuid)[5], (uuid)[6], (uuid)[7], \
    (uuid)[8], (uuid)[9], (uuid)[10], (uuid)[11], (uuid)[12], (uuid)[13], (uuid)[14], (uuid)[15]

#define UUID_T_VALUES_OR_NULL(uuid) \
    ((uuid) == NULL) ? 0 : (uuid)[0], ((uuid) == NULL) ? 0 : (uuid)[1], ((uuid) == NULL) ? 0 : (uuid)[2], ((uuid) == NULL) ? 0 : (uuid)[3], \
    ((uuid) == NULL) ? 0 : (uuid)[4], ((uuid) == NULL) ? 0 : (uuid)[5], ((uuid) == NULL) ? 0 : (uuid)[6], ((uuid) == NULL) ? 0 : (uuid)[7], \
    ((uuid) == NULL) ? 0 : (uuid)[8], ((uuid) == NULL) ? 0 : (uuid)[9], ((uuid) == NULL) ? 0 : (uuid)[10], ((uuid) == NULL) ? 0 : (uuid)[11], \
    ((uuid) == NULL) ? 0 : (uuid)[12], ((uuid) == NULL) ? 0 : (uuid)[13], ((uuid) == NULL) ? 0 : (uuid)[14], ((uuid) == NULL) ? 0 : (uuid)[15] \

#ifdef __cplusplus
}
#endif

#endif /*UUID_H*/
