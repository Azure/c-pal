// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef INTERLOCKED_MACROS_H
#define INTERLOCKED_MACROS_H

#ifdef __cplusplus
#include <cstdint>

#else
#include <stdint.h>
#endif

#include "macro_utils/macro_utils.h"

/*Codes_SRS_INTERLOCKED_MACROS_42_001: [ INTERLOCKED_DEFINE_VOLATILE_STATE_ENUM shall generate a union with two fields: a volatile_atomic int32_t and a variable of the type enum_type. ]*/
#define INTERLOCKED_DEFINE_VOLATILE_STATE_ENUM(enum_type, variable_name) \
    union \
    { \
        volatile_atomic int32_t variable_name; \
        enum_type MU_C2(variable_name, _enum); \
        char MU_C3(assert_that_, enum_type, _is_int32_t_size)[(sizeof(int32_t)==sizeof(enum_type)) ? 1 : -1];\
    }

#endif // INTERLOCKED_MACROS_H
