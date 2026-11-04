// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef UNIQUE_THANDLE_LL_H
#define UNIQUE_THANDLE_LL_H

#include <stddef.h>

#include "macro_utils/macro_utils.h"

#include "c_pal/thandle_ll.h"

#include "umock_c/umock_c_prod.h"

/*UNIQUE_THANDLE(T) is a handle to a T that has a single owner.
A handle is an array of one slot, like jmp_buf. The compiler rejects UNIQUE_THANDLE(T) copy = handle;, assignments of handles and functions that return a handle.
A UNIQUE_THANDLE(T) parameter points to the caller's handle, and a const UNIQUE_THANDLE(T) parameter only reads it.
The slot holds a const pointer to a struct that is only declared. UNIQUE_THANDLE_ADOPT, UNIQUE_THANDLE_INITIALIZE_MOVE and UNIQUE_THANDLE_DISPOSE write the slot, and UNIQUE_THANDLE_GET_T converts its pointer to a T*.*/
#define UNIQUE_THANDLE(T) MU_C2(T, _UNIQUE_THANDLE)

/*initializes a UNIQUE_THANDLE(T) to NULL, as in UNIQUE_THANDLE(T) handle = UNIQUE_THANDLE_NULL;*/
#define UNIQUE_THANDLE_NULL { { NULL } }

/*given a previous type T, these are the struct tags of the slot of UNIQUE_THANDLE(T) and of the object the slot points to. The object struct tag is only declared*/
#define UNIQUE_THANDLE_SLOT_TAG(T) MU_C2(T, _UNIQUE_SLOT)
#define UNIQUE_THANDLE_OBJECT_TAG(T) MU_C2(T, _UNIQUE_OBJECT)

/*introduces UNIQUE_THANDLE(T), once per type T. The names of the types are built from T, so UNIQUE_THANDLE_LL_TYPE_DECLARE(C, T) for any C uses the same UNIQUE_THANDLE(T)*/
#define UNIQUE_THANDLE_MACRO(T)                                                                                         \
    typedef struct UNIQUE_THANDLE_SLOT_TAG(T)                                                                           \
    {                                                                                                                   \
        const struct UNIQUE_THANDLE_OBJECT_TAG(T)* const volatile object;                                               \
    } UNIQUE_THANDLE(T)[1];

/*given a previous type T, UNIQUE_THANDLE_INITIALIZE_MOVE introduces a new name for a function that moves a handle into a handle that is NULL or not initialized*/
#define UNIQUE_THANDLE_INITIALIZE_MOVE(C) MU_C2(C, _UNIQUE_INITIALIZE_MOVE)

/*given a previous type T, UNIQUE_THANDLE_DISPOSE introduces a new name for a function that sets a handle to NULL and disposes the object it held*/
#define UNIQUE_THANDLE_DISPOSE(C) MU_C2(C, _UNIQUE_DISPOSE)

/*given a previous type T, these are the names of the static functions that UNIQUE_THANDLE_LL_TYPE_DEFINE defines for the .c file that defines T*/
#define UNIQUE_THANDLE_MALLOC(C) THANDLE_MALLOC(C)
#define UNIQUE_THANDLE_MALLOC_FLEX(C) THANDLE_MALLOC_FLEX(C)
#define UNIQUE_THANDLE_FREE(C) THANDLE_FREE(C)
#define UNIQUE_THANDLE_ADOPT(C) MU_C2(C, _UNIQUE_ADOPT)
#define UNIQUE_THANDLE_GET_T(C) MU_C2(C, _UNIQUE_GET_T)

/*given a previous type T, UNIQUE_THANDLE_INSPECT introduces a new name for a function that returns the THANDLE_WRAPPER_TYPE_NAME(T) of the object a handle holds - useful in debugging, like THANDLE_INSPECT. The function has no side-effects*/
#define UNIQUE_THANDLE_INSPECT(C) MU_C2(C, _UNIQUE_INSPECT)

/*macro to be used in headers*/
/*declares the functions that move and dispose a UNIQUE_THANDLE(T), named after C. UNIQUE_THANDLE_MACRO(T) introduces UNIQUE_THANDLE(T) before*/
/*unlike THANDLE_LL_TYPE_DECLARE, it declares only functions: a translation unit defines the slot struct of T once, and REAL_UNIQUE_THANDLE_DECLARE declares the functions of REAL_T for the UNIQUE_THANDLE(T) that UNIQUE_THANDLE_TYPE_DECLARE(T) introduced*/
#define UNIQUE_THANDLE_LL_TYPE_DECLARE(C, T)                                                                            \
    MOCKABLE_FUNCTION(, void, UNIQUE_THANDLE_INITIALIZE_MOVE(C), UNIQUE_THANDLE(T), t1, UNIQUE_THANDLE(T), t2);         \
    MOCKABLE_FUNCTION(, void, UNIQUE_THANDLE_DISPOSE(C), UNIQUE_THANDLE(T), t);

#endif /*UNIQUE_THANDLE_LL_H*/
