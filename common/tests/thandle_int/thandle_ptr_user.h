// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef THANDLE_PTR_USER_H
#define THANDLE_PTR_USER_H

#include "c_pal/thandle.h"
#include "c_pal/thandle_ll.h"         // for THANDLE

#include "macro_utils/macro_utils.h"  // for MU_COUNT_ARG_0, MU_DISPATCH_EMP...

/*all the macros in this file depend on "T". T is "a pointer to a type", so it looks like "COORD_STRUCT*". */

/*this introduces a new *name* for a type that is a typedef of struct THANDLE_PTR_STRUCT_TAG_NAME {...}*/
#define PTR_STRUCT_TYPE_NAME(T) MU_C2(PTR_STRUCT_, T)

/*this introduces a new *name* for a structure type that contains "T pointer"*/
#define PTR_STRUCT_TAG_TYPE_NAME(T) MU_C2(PTR_STRUCT_TYPE_NAME(T), _TAG)

/*this introduces a new *name* for a function pointer type that takes a T and frees it*/
#define THANDLE_PTR_FREE_FUNC_TYPE_NAME(T) MU_C2(THANDLE_PTR_FREE_FUNC_, T)

/*this introduces a new *type* which is a pointer to the free type for T*/
#define THANDLE_PTR_FREE_FUNC_TYPE(T) typedef void (*THANDLE_PTR_FREE_FUNC_TYPE_NAME(T))(T arg);

/*this introduces a new *type* which is a structure with a field of type T"*/
#define PTR_STRUCT_TYPE_TYPEDEF(T) typedef struct PTR_STRUCT_TAG_TYPE_NAME(T) {T pointer;} PTR_STRUCT_TYPE_NAME(T);

/*this introduces one new *name* for a function which is used to capture a T ptr and move it under the THANDLE(PTR(T))*/
#define THANDLE_PTR_CREATE_NAME(T) MU_C2(THANDLE_PTR_CREATE_, T)

/*this introduces the declaration of a function that returns a THANDLE(PTR_STRUCT_TYPE_NAME(T))*/
#define THANDLE_PTR_DECLARE(T) \
    PTR_STRUCT_TYPE_TYPEDEF(T);                                                                                     \
    /*THANDLE_PTR_FREE_FUNC_TYPE(T);*/                                                                              \
    THANDLE_TYPE_DECLARE(PTR_STRUCT_TYPE_NAME(T));                                                                  \

/*this introduces the definition of the function declared above*/
#define THANDLE_PTR_DEFINE(T) \
    THANDLE_TYPE_DEFINE(PTR_STRUCT_TYPE_NAME(T));                                                                   \
    THANDLE(PTR_STRUCT_TYPE_NAME(T)) create(T source)                                                               \
    {                                                                                                               \
        PTR_STRUCT_TYPE_NAME(T) temp = { .pointer = source };                                                       \
        return THANDLE_CREATE_FROM_CONTENT(PTR_STRUCT_TYPE_NAME(T))(&temp, NULL, NULL);                             \
    }

#endif /*THANDLE_PTR_USER_H*/
