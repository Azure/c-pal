// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef UUID_H
#define UUID_H

typedef unsigned char UUID_T[16]; /*introduces UUID_T as "array of 16 bytes"*/

#include "macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

    MOCKABLE_FUNCTION(, int, uuid_produce, UUID_T, destination);

#ifdef __cplusplus
}
#endif

#endif /*UUID_H*/
