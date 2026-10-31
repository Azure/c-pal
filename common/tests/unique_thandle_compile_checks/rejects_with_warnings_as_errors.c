// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/*code that the compiler rejects when warnings are treated as errors, as in c-pal builds. Each case starts with a #line directive that names it, so that unique_thandle_compile_checks.cmake checks that each case has its own diagnostic*/

#include <stddef.h>

#include "macro_utils/macro_utils.h"

#include "c_pal/unique_thandle.h"

#include "unique_thandle_compile_checks.h"

#line 1000 "move_from_a_const_handle"
void move_from_a_const_handle(UNIQUE_THANDLE(ITEM) destination, const UNIQUE_THANDLE(ITEM) item)
{
    UNIQUE_THANDLE_INITIALIZE_MOVE(ITEM)(destination, item);
}

#line 1000 "dispose_of_a_const_handle"
void dispose_of_a_const_handle(const UNIQUE_THANDLE(ITEM) item)
{
    UNIQUE_THANDLE_DISPOSE(ITEM)(item);
}

#line 1000 "move_from_a_handle_in_a_const_struct"
void move_from_a_handle_in_a_const_struct(UNIQUE_THANDLE(ITEM) destination, const LIST* list)
{
    UNIQUE_THANDLE_INITIALIZE_MOVE(ITEM)(destination, list->items[0]);
}

#line 1000 "copy_of_the_object_pointer_to_a_pointer_to_t"
const ITEM* copy_of_the_object_pointer_to_a_pointer_to_t(const UNIQUE_THANDLE(ITEM) item)
{
    const ITEM* copy = item[0].object;
    return copy;
}

#line 1000 "move_from_a_pointer_to_t"
void move_from_a_pointer_to_t(UNIQUE_THANDLE(ITEM) destination, const ITEM* object)
{
    UNIQUE_THANDLE_INITIALIZE_MOVE(ITEM)(destination, &object);
}

#line 1000 "move_from_a_handle_of_another_type"
void move_from_a_handle_of_another_type(UNIQUE_THANDLE(ITEM) destination, UNIQUE_THANDLE(OTHER) other)
{
    UNIQUE_THANDLE_INITIALIZE_MOVE(ITEM)(destination, other);
}
