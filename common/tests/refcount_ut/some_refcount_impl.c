// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>

#include "macro_utils/macro_utils.h"

#include "c_pal/refcount.h"
#include "c_pal/gballoc_hl.h" // IWYU pragma: keep
#include "c_pal/gballoc_hl_redirect.h" // IWYU pragma: keep

#include "some_refcount_impl.h"

typedef struct pos_TAG
{
    int x;
    int flexible_array[];
} pos;

/* Codes_SRS_REFCOUNT_01_001: [ DEFINE_REFCOUNT_TYPE shall define the Create/Create_With_Extra_size/Create_Flex/Destroy functions for the type type. ]*/
DEFINE_REFCOUNT_TYPE(pos);

POS_HANDLE Pos_Create(int x)
{
    pos* result = REFCOUNT_TYPE_CREATE(pos);
    if (result != NULL)
    {
        result->x = x;
    }
    return result;
}

POS_HANDLE Pos_Create_With_Extra_Size(int x, size_t extra_size)
{
    pos* result = REFCOUNT_TYPE_CREATE_WITH_EXTRA_SIZE(pos, extra_size);
    if (result != NULL)
    {
        result->x = x;
    }
    return result;
}

POS_HANDLE Pos_Create_Flex(size_t nmemb)
{
    pos* result = REFCOUNT_TYPE_CREATE_FLEX(pos, nmemb, sizeof(int));
    if (result != NULL)
    {
        result->x = (int)nmemb;
    }
    return result;
}

POS_HANDLE Pos_Clone(POS_HANDLE posHandle)
{

    if (posHandle != NULL)
    {
        pos* p = posHandle;
        INC_REF(pos, p);
    }
    return posHandle;
}

void Pos_Destroy(POS_HANDLE posHandle)
{
    if (posHandle != NULL)
    {
        pos* p = posHandle;
        if (DEC_REF(pos, p) == 0)
        {
            REFCOUNT_TYPE_DESTROY(pos, p);
        }
    }
}
