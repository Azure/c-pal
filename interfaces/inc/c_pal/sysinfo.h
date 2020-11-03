// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef SYSINFO_H
#define SYSINFO_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C"
{
#endif

MOCKABLE_FUNCTION(, uint32_t, sysinfo_get_processor_count);

#ifdef __cplusplus
}
#endif

#endif /* SYSINFO_H */
