// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef EXAMPLE_H
#define EXAMPLE_H

#include "c_pal/thandle_ptr.h"

/*this is a complete type*/
typedef struct EXAMPLE_COMPLETE_TAG
{
    int example_complete;
}EXAMPLE_COMPLETE;

typedef EXAMPLE_COMPLETE* EXAMPLE_COMPLETE_PTR;

THANDLE_PTR_DECLARE(EXAMPLE_COMPLETE_PTR);

void dispose_example_complete(EXAMPLE_COMPLETE_PTR example_complete);

#endif /*EXAMPLE_H*/
