// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include "c_pal/gballoc_hl.h"           // IWYU pragma: keep
#include "c_pal/gballoc_hl_redirect.h"  // IWYU pragma: keep

#include "c_pal/thandle_ptr.h"

#include "example.h"

THANDLE_PTR_DEFINE(EXAMPLE_COMPLETE_PTR);

void dispose_example_complete(EXAMPLE_COMPLETE_PTR example_complete)
{
    free(example_complete); /*nothing else*/
}