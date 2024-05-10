// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include "macro_utils/macro_utils.h"

#include "c_pal/gballoc_hl.h"               // IWYU pragma: keep
#include "c_pal/gballoc_hl_redirect.h"      // IWYU pragma: keep

#include "c_pal/thandle_ptr.h"

#include "example_incomplete_type.h"

struct EXAMPLE_INCOMPLETE
{
    int example_incomplete;
};

THANDLE_PTR_DEFINE(EXAMPLE_INCOMPLETE_PTR);

EXAMPLE_INCOMPLETE_PTR create_example_incomplete(void)
{
    return malloc(sizeof(struct EXAMPLE_INCOMPLETE));
}

void dispose_example_incomplete(EXAMPLE_INCOMPLETE_PTR example_incomplete)
{
    free(example_incomplete); /*nothing else*/
}