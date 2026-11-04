// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/*the misuses that unique_thandle_requirements.md shows and that the compiler rejects with an error, in the same order, then other forms of them.
Keep the cases in step with unique_thandle_requirements.md. Each case starts with a #line directive that names it, so that unique_thandle_compile_checks.cmake checks that each case has its own error*/

#include <stddef.h>
#include <stdint.h>

#include "macro_utils/macro_utils.h"

#include "c_pal/unique_thandle.h"

#include "unique_thandle_compile_checks.h"

/*item.c*/
#line 1000 "misuses"
void misuses(UNIQUE_THANDLE(ITEM) item)
{
    UNIQUE_THANDLE(ITEM) temp = UNIQUE_THANDLE_NULL;

#line 1000 "copies_a_handle"
    /*copies a handle: copy and item would both own the item*/
    UNIQUE_THANDLE(ITEM) copy = item;

#line 1000 "assigns_a_handle"
    /*assigns a handle: temp and item would both own the item*/
    temp = item;

#line 1000 "writes_the_pointer_in_a_handle"
    /*writes the pointer in a handle: temp and item would both own the item*/
    temp[0].object = item[0].object;

#line 2000 "misuses"
    (void)copy;
    (void)temp;
}

/*list.h*/
#line 1000 "returns_a_handle"
/*returns a handle: the caller and the list would both own the item*/
UNIQUE_THANDLE(ITEM) list_get(LIST_HANDLE list, uint32_t index);

/*other forms of the misuses above*/

#line 1000 "copies_a_handle_variable"
void copies_a_handle_variable(void)
{
    UNIQUE_THANDLE(ITEM) item = UNIQUE_THANDLE_NULL;
    UNIQUE_THANDLE(ITEM) copy = item;
    (void)copy;
}

#line 1000 "assigns_through_handle_parameters"
void assigns_through_handle_parameters(UNIQUE_THANDLE(ITEM) copy, UNIQUE_THANDLE(ITEM) item)
{
    *copy = *item;
}

#line 1000 "assigns_the_elements_of_handles"
void assigns_the_elements_of_handles(UNIQUE_THANDLE(ITEM) copy, UNIQUE_THANDLE(ITEM) item)
{
    copy[0] = item[0];
}
