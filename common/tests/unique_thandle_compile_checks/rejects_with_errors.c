// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/*code that the compiler rejects with an error. Each case starts with a #line directive that names it, so that unique_thandle_compile_checks.cmake checks that each case has its own error*/

#include <stddef.h>

#include "macro_utils/macro_utils.h"

#include "c_pal/unique_thandle.h"

#include "unique_thandle_compile_checks.h"

#line 1000 "copy_initialization_from_a_parameter"
void copy_initialization_from_a_parameter(UNIQUE_THANDLE(ITEM) item)
{
    UNIQUE_THANDLE(ITEM) copy = item;
    (void)copy;
}

#line 1000 "copy_initialization_from_a_variable"
void copy_initialization_from_a_variable(void)
{
    UNIQUE_THANDLE(ITEM) item = { { NULL } };
    UNIQUE_THANDLE(ITEM) copy = item;
    (void)copy;
}

#line 1000 "assignment_of_a_handle_in_an_array"
void assignment_of_a_handle_in_an_array(LIST* list)
{
    list->items[0] = list->items[1];
}

#line 1000 "assignment_through_a_handle_parameter"
void assignment_through_a_handle_parameter(UNIQUE_THANDLE(ITEM) destination, UNIQUE_THANDLE(ITEM) source)
{
    *destination = *source;
}

#line 1000 "assignment_of_the_element_of_a_handle"
void assignment_of_the_element_of_a_handle(UNIQUE_THANDLE(ITEM) destination, UNIQUE_THANDLE(ITEM) source)
{
    destination[0] = source[0];
}

#line 1000 "assignment_of_the_object_pointer_of_a_handle"
void assignment_of_the_object_pointer_of_a_handle(UNIQUE_THANDLE(ITEM) destination, UNIQUE_THANDLE(ITEM) source)
{
    destination[0].object = source[0].object;
}

#line 1000 "function_returning_a_handle"
UNIQUE_THANDLE(ITEM) function_returning_a_handle(void);
