// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef EXAMPLE_INCOMPLETE_TYPE_H
#define EXAMPLE_INCOMPLETE_TYPE_H

#include "macro_utils/macro_utils.h"

#include "c_pal/thandle_ptr.h"

/*EXAMPLE_INCOMPLETE is an incomplete type (opaque)*/
typedef struct EXAMPLE_INCOMPLETE* EXAMPLE_INCOMPLETE_PTR;

THANDLE_PTR_DECLARE(EXAMPLE_INCOMPLETE_PTR);

EXAMPLE_INCOMPLETE_PTR create_example_incomplete(void);

void dispose_example_incomplete(EXAMPLE_INCOMPLETE_PTR example_incomplete);

#endif /*EXAMPLE_INCOMPLETE_TYPE_H*/
