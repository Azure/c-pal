// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef T_OFF_H
#define T_OFF_H

/*
T_OFF will use THANDLE_LL_TYPE_DEFINE (will not use THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS) - so "type off"
*/

#include <stdint.h>

#include "c_pal/thandle.h"
#include "c_pal/thandle_ll.h"

#include "umock_c/umock_c_prod.h"

    typedef struct T_OFF_TAG
    {
        int x;      /*some pretend coordinate*/
        uint32_t n; /*the number of characters in s[]. Might be 0, in which case s should not be accessed*/
        char s[]    /*has n characters*/;
    }T_OFF_DUMMY;

    THANDLE_TYPE_DECLARE(T_OFF_DUMMY);

    MOCKABLE_FUNCTION(, THANDLE(T_OFF_DUMMY), T_OFF_create, int, x);
    MOCKABLE_FUNCTION(, THANDLE(T_OFF_DUMMY), T_OFF_create_with_malloc_functions, int, x);
    MOCKABLE_FUNCTION(, THANDLE(T_OFF_DUMMY), T_OFF_create_flex, int, x, const char*, s);
    MOCKABLE_FUNCTION(, THANDLE(T_OFF_DUMMY), T_OFF_create_flex_with_malloc_functions, int, x, const char*, s);
    MOCKABLE_FUNCTION(, THANDLE(T_OFF_DUMMY), T_OFF_create_from_content_flex, const T_OFF_DUMMY*, origin);
    MOCKABLE_FUNCTION(, THANDLE(T_OFF_DUMMY), T_OFF_create_from_content_flex_with_malloc_functions, const T_OFF_DUMMY*, origin);

    MOCKABLE_FUNCTION(, THANDLE(T_OFF_DUMMY), T_OFF_create_from_content_flex_with_getsizeof_NULL, const T_OFF_DUMMY*, origin);




#endif /*T_OFF_H*/
