// Copyright (c) Microsoft. All rights reserved.

#include "test_module.h"

#include "../../../interfaces/inc/c_pal/malloc_multi_flex.h"

DEFINE_MALLOC_MULTI_FLEX_STRUCT(PARENT_STRUCT,
    FIELDS(uint64_t, int_1, uint32_t, int_2, uint32_t, int_3),
    ARRAY_FIELDS(uint32_t, array_1, uint64_t, array_2, INNER_STRUCT, array_3))

PARENT_STRUCT* create_parent_struct()
{
    uint32_t array1_size = 10;
    uint32_t array2_size = 20;
    uint32_t array3_size = 30;
    PARENT_STRUCT* parent_struct = MALLOC_MULTI_FLEX_STRUCT(PARENT_STRUCT)(sizeof(PARENT_STRUCT), array1_size, array2_size, array3_size);
    if (parent_struct != NULL)
    {
        parent_struct->int_1 = 3;
        parent_struct->int_2 = 6;
        parent_struct->int_3 = 9;

        for (uint32_t i = 0; i < array1_size; i++)
        {
            parent_struct->array_1[i] = i;
        }

        for (uint32_t i = 0; i < array2_size; i++)
        {
            parent_struct->array_2[i] = i + 100;
        }

        for (uint32_t i = 0; i < array3_size; i++)
        {
            parent_struct->array_3[i].inner_int_1 = i + 1000;
            parent_struct->array_3[i].inner_int_2 = i + 2000;
        }
    }
    return parent_struct;
}
