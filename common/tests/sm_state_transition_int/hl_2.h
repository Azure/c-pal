// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef HL_H
#define HL_H

#ifdef __cplusplus
#else
#include <stdbool.h>
#endif

#include "interface.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct HL_IO_CONFIG_TAG
{
    const INTERFACE_DESCRIPTION* underlying_interface;
    void* config_param;
} HL_IO_CONFIG;

MOCKABLE_FUNCTION(, const INTERFACE_DESCRIPTION*, hl_2_get_interface_description);

#ifdef __cplusplus
}
#endif

#endif // HL_H
