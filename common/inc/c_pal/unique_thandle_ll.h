// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef UNIQUE_THANDLE_LL_H
#define UNIQUE_THANDLE_LL_H

#include "macro_utils/macro_utils.h"

#include "c_pal/thandle_ll.h"

#include "umock_c/umock_c_prod.h"

/*a handle to a T that has a single owner. It is implemented as a THANDLE(T) whose reference count is always 1*/
#define UNIQUE_THANDLE(T) THANDLE(T)

/*given a previous type T, these are the names of the memory functions that UNIQUE_THANDLE_LL_TYPE_DEFINE defines for T. They are the THANDLE memory functions*/
#define UNIQUE_THANDLE_MALLOC(C) THANDLE_MALLOC(C)
#define UNIQUE_THANDLE_MALLOC_FLEX(C) THANDLE_MALLOC_FLEX(C)
#define UNIQUE_THANDLE_FREE(C) THANDLE_FREE(C)
#define UNIQUE_THANDLE_GET_T(C) THANDLE_GET_T(C)

/*given a previous type T, UNIQUE_THANDLE_MOVE introduces a new name for a function that moves a handle into an initialized handle and disposes the handle it held*/
#define UNIQUE_THANDLE_MOVE(C) MU_C2(C, _UNIQUE_MOVE)

/*given a previous type T, UNIQUE_THANDLE_INITIALIZE_MOVE introduces a new name for a function that moves a handle into an uninitialized handle*/
#define UNIQUE_THANDLE_INITIALIZE_MOVE(C) MU_C2(C, _UNIQUE_INITIALIZE_MOVE)

/*given a previous type T, UNIQUE_THANDLE_DISPOSE introduces a new name for a function that disposes a handle and sets it to NULL*/
#define UNIQUE_THANDLE_DISPOSE(C) MU_C2(C, _UNIQUE_DISPOSE)

/*macro to be used in headers*/
/*introduces UNIQUE_THANDLE(T) and declares the functions that move and dispose it, named after C. No function that copies the handle is declared*/
#define UNIQUE_THANDLE_LL_TYPE_DECLARE(C, T)                                                                          \
    THANDLE_MACRO(T);                                                                                                 \
    MOCKABLE_FUNCTION(, void, UNIQUE_THANDLE_MOVE(C), UNIQUE_THANDLE(T)*, t1, UNIQUE_THANDLE(T)*, t2);                \
    MOCKABLE_FUNCTION(, void, UNIQUE_THANDLE_INITIALIZE_MOVE(C), UNIQUE_THANDLE(T)*, t1, UNIQUE_THANDLE(T)*, t2);     \
    MOCKABLE_FUNCTION(, void, UNIQUE_THANDLE_DISPOSE(C), UNIQUE_THANDLE(T)*, t);                                      \

#endif /*UNIQUE_THANDLE_LL_H*/
