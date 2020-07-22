// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>

#include "azure_macro_utils/macro_utils.h"
#include "azure_c_logging/xlogging.h"

#include "azure_c_pal/gballoc_ll.h"

#include "azure_c_pal/gballoc_hl.h"

int gballoc_hl_init(void* gballoc_hl_init_params, void* gballoc_ll_init_params)
{
    int result;
    (void)gballoc_hl_init_params; /*are ignored, this is "passthrough*/

    /*Codes_SRS_GBALLOC_HL_PASSTHROUGH_02_001: [ gballoc_hl_init shall call gballoc_ll_init(gballoc_ll_init_params). ]*/

    if (gballoc_ll_init(gballoc_ll_init_params) != 0)
    {
        /*Codes_SRS_GBALLOC_HL_PASSTHROUGH_02_003: [ If there are any failures then gballoc_hl_init shall fail and return a non-zero value. ]*/
        LogError("failure in gballoc_ll_init(gballoc_ll_init_params=%p)", gballoc_ll_init_params);
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_GBALLOC_HL_PASSTHROUGH_02_002: [ gballoc_hl_init shall succeed and return 0. ]*/
        result = 0;
    }

    return result;
}

void gballoc_hl_deinit(void)
{
    /*no work for this layer, it is passthough*/
    /*Codes_SRS_GBALLOC_HL_PASSTHROUGH_02_004: [ gballoc_hl_deinit shall call gballoc_ll_deinit. ]*/
    gballoc_ll_deinit();
}

void* gballoc_hl_malloc(size_t size)
{
    /*Codes_SRS_GBALLOC_HL_PASSTHROUGH_02_005: [ gballoc_hl_malloc shall call gballoc_ll_malloc(size) and return what gballoc_ll_malloc returned. ]*/
    void* result = gballoc_ll_malloc(size);

    if (result == NULL)
    {
        LogError("failure in gballoc_ll_malloc(size=%zu)", size);
    }
    return result;
}

void gballoc_hl_free(void* ptr)
{
    /*Codes_SRS_GBALLOC_HL_PASSTHROUGH_02_006: [ gballoc_hl_free shall call gballoc_hl_free(ptr). ]*/
    gballoc_ll_free(ptr);
}


void* gballoc_hl_calloc(size_t nmemb, size_t size)
{
    /*Codes_SRS_GBALLOC_HL_PASSTHROUGH_02_007: [ gballoc_hl_calloc shall call gballoc_ll_calloc(nmemb, size) and return what gballoc_ll_calloc returned. ]*/
    void* result = gballoc_ll_calloc(nmemb, size);

    if (result == NULL)
    {
        LogError("failure in gballoc_ll_calloc(nmemb=%zu, size=%zu)", nmemb, size);
    }

    return result;
}


void* gballoc_hl_realloc(void* ptr, size_t size)
{
    /*Codes_SRS_GBALLOC_HL_PASSTHROUGH_02_008: [ gballoc_hl_realloc shall call gballoc_ll_realloc(ptr, size) and return what gballoc_ll_realloc returned. ]*/
    void* result = gballoc_ll_realloc(ptr, size);

    if (result == NULL)
    {
        LogError("Failure in gballoc_ll_realloc(ptr=%p, size=%zu)", ptr, size);
    }

    return result;
}
