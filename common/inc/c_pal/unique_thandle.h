// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef UNIQUE_THANDLE_H
#define UNIQUE_THANDLE_H

#include "macro_utils/macro_utils.h"

#include "c_pal/unique_thandle_ll.h"

/*macro to be used in headers*/
/*introduces UNIQUE_THANDLE(T) for a type T and declares the functions that move and dispose it*/
#define UNIQUE_THANDLE_TYPE_DECLARE(T)                                                                                  \
    UNIQUE_THANDLE_MACRO(T)                                                                                             \
    UNIQUE_THANDLE_LL_TYPE_DECLARE(T, T)

/*macro to be used in a .c file*/
/*defines the functions declared by UNIQUE_THANDLE_TYPE_DECLARE and the static functions for the .c file that defines T*/
#define UNIQUE_THANDLE_TYPE_DEFINE(T)                                                                                                       \
    /*Codes_SRS_UNIQUE_THANDLE_43_001: [ UNIQUE_THANDLE_TYPE_DEFINE shall use malloc, malloc_flex and free to allocate and free memory. ]*/ \
    UNIQUE_THANDLE_LL_TYPE_DEFINE(T, T)

/*macro to be used in a .c file*/
/*same as UNIQUE_THANDLE_TYPE_DEFINE, with memory functions specific to T*/
#define UNIQUE_THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(T, malloc_function, malloc_flex_function, free_function)                                                                            \
    /*Codes_SRS_UNIQUE_THANDLE_43_002: [ UNIQUE_THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS shall use malloc_function, malloc_flex_function and free_function to allocate and free memory. ]*/ \
    UNIQUE_THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(T, T, malloc_function, malloc_flex_function, free_function)

#endif /*UNIQUE_THANDLE_H*/
