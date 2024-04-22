// Copyright (c) Microsoft. All rights reserved.

#ifndef TEST_MODULE_H
#define TEST_MODULE_H

#include <stdint.h>

#include "macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"

#include "../../../interfaces/inc/c_pal/malloc_multi_flex.h"

typedef struct INNER_STURCT_TAG
{
    uint32_t inner_int_1;
    uint64_t inner_int_2;
}INNER_STRUCT;

typedef struct INVALID_STRUCT_TAG
{
    uint64_t int_1;
    uint32_t* array_1;
    uint64_t* array_2;
}INVALID_STRUCT;

DECLARE_MALLOC_MULTI_FLEX_STRUCT(PARENT_STRUCT,
    FIELDS(uint64_t, int_1, uint32_t, int_2, uint32_t, int_3),
    ARRAY_FIELDS(uint32_t, array_1, uint64_t, array_2, INNER_STRUCT, array_3))

MOCKABLE_FUNCTION(, PARENT_STRUCT*, create_parent_struct, uint64_t, array_1_size, uint64_t, array_2_size, uint64_t, array_3_size);

#endif // TEST_MODULE_H