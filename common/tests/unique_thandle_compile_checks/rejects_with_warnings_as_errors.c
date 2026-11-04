// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/*the misuses that unique_thandle_requirements.md shows and that the compiler rejects when warnings are treated as errors, as in c-pal builds, in the same order, then another form of them.
Keep the cases in step with unique_thandle_requirements.md. Each case starts with a #line directive that names it, so that unique_thandle_compile_checks.cmake checks that each case has its own diagnostic*/

#include <stddef.h>

#include "macro_utils/macro_utils.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/unique_thandle.h"

#include "unique_thandle_compile_checks.h"

/*item.c*/
struct ITEM_TAG
{
    int value;
};

UNIQUE_THANDLE_TYPE_DEFINE(ITEM);

#line 1000 "misuses"
void misuses(UNIQUE_THANDLE(ITEM) item, UNIQUE_THANDLE(OTHER) other)
{
    UNIQUE_THANDLE(ITEM) temp = UNIQUE_THANDLE_NULL;
    ITEM* pointer = UNIQUE_THANDLE_GET_T(ITEM)(item);

#line 1000 "makes_a_handle_from_a_pointer"
    /*warnings as errors: makes a handle from a pointer to an item, which a handle may already own*/
    UNIQUE_THANDLE_INITIALIZE_MOVE(ITEM)(temp, &pointer);

#line 1000 "moves_a_handle_to_another_type"
    /*warnings as errors: moves a handle to another type, OTHER, into a handle to an ITEM*/
    UNIQUE_THANDLE_INITIALIZE_MOVE(ITEM)(temp, other);
#line 2000 "misuses"
}

/*list.c*/
#line 1000 "node_get_value"
/*node_get_value reads a node*/
int node_get_value(const NODE* node)
{
    UNIQUE_THANDLE(ITEM) temp = UNIQUE_THANDLE_NULL;

#line 1000 "node_get_value_moves_the_item_into_temp"
    /*warnings as errors: moves the item, which the node owns, into temp*/
    UNIQUE_THANDLE_INITIALIZE_MOVE(ITEM)(temp, node->item);
#line 2000 "node_get_value"
    return 0;
}

#line 1000 "item_get_value"
/*item_get_value uses the item during the call*/
int item_get_value(const UNIQUE_THANDLE(ITEM) item)
{
    UNIQUE_THANDLE(ITEM) temp = UNIQUE_THANDLE_NULL;

#line 1000 "item_get_value_moves_the_item_into_temp"
    UNIQUE_THANDLE_INITIALIZE_MOVE(ITEM)(temp, item); /*warnings as errors: does not compile*/

#line 1000 "item_get_value_disposes_the_item"
    UNIQUE_THANDLE_DISPOSE(ITEM)(item); /*warnings as errors: does not compile*/

#line 2000 "item_get_value"
    ITEM* item_ptr = UNIQUE_THANDLE_GET_T(ITEM)(item);
    return item_ptr->value;
}

/*another form of the misuses above*/

#line 1000 "copies_the_pointer_in_a_handle_to_a_pointer_to_an_item"
const ITEM* copies_the_pointer_in_a_handle_to_a_pointer_to_an_item(const UNIQUE_THANDLE(ITEM) item)
{
    const ITEM* pointer = item[0].object;
    return pointer;
}
