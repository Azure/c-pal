// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef THANDLE_PTR_H
#define THANDLE_PTR_H

#include "c_pal/thandle.h"
#include "c_pal/thandle_ll.h"         // for THANDLE

#include "macro_utils/macro_utils.h"  // for MU_COUNT_ARG_0, MU_DISPATCH_EMP...

/*all the macros in this file depend on "T". T is "a pointer to a type", so it looks like "COORD_STRUCT*". */

/*this introduces a new *name* for a type that is a typedef of struct THANDLE_PTR_STRUCT_TAG_NAME {...}*/
#define PTR(T) MU_C2(PTR_STRUCT_, T)

/*this introduces a new *name* for a structure type that contains "T pointer"*/
#define PTR_STRUCT_TAG_TYPE_NAME(T) MU_C2(PTR(T), _TAG)

/*this introduces a new *name* for a function pointer type that takes a T and frees it*/
#define THANDLE_PTR_FREE_FUNC_TYPE_NAME(T) MU_C2(THANDLE_PTR_FREE_FUNC_, T)

/*this introduces a new *type* which is a pointer to the free type for T*/
#define THANDLE_PTR_FREE_FUNC_TYPE(T) typedef void (*THANDLE_PTR_FREE_FUNC_TYPE_NAME(T))(T arg);

/*this introduces a new *type* which is a structure with a field of type T"*/
#define PTR_STRUCT_TYPE_TYPEDEF(T)                                                                              \
typedef struct PTR_STRUCT_TAG_TYPE_NAME(T)                                                                      \
{                                                                                                               \
    T pointer; /*original pointer passed to THANDLE_PTR_CREATE_WITH_MOVE*/                                      \
    THANDLE_PTR_FREE_FUNC_TYPE_NAME(T) dispose; /*original dispose passed to THANDLE_PTR_CREATE_WITH_MOVE*/     \
} PTR(T);

/*this introduces one new *name* for a function which is used to capture a T ptr and move it under the THANDLE(PTR(T))*/
#define THANDLE_PTR_CREATE_WITH_MOVE(T) MU_C2(THANDLE_PTR_CREATE_WITH_MOVE_, T)

/*this introduces a new *name* for a function that calls the dispose as passed to THANDLE_PTR_CREATE_WITH_MOVE*/
#define THANDLE_PTR_DISPOSE(T) MU_C2(THANDLE_PTR_DISPOSE_, T)

#include "umock_c/umock_c_prod.h"

/*this introduces the declaration of a function that returns a THANDLE(PTR(T))*/
#define THANDLE_PTR_DECLARE(T) \
    THANDLE_PTR_FREE_FUNC_TYPE(T);                                                                                                      \
    PTR_STRUCT_TYPE_TYPEDEF(T);                                                                                                         \
    THANDLE_TYPE_DECLARE(PTR(T));                                                                                                       \
    MOCKABLE_FUNCTION(,THANDLE(PTR(T)), THANDLE_PTR_CREATE_WITH_MOVE(T), T, pointer, THANDLE_PTR_FREE_FUNC_TYPE_NAME(T), dispose );     \

/*this introduces the definition of the function declared above*/
#define THANDLE_PTR_DEFINE(T)                                                                                                                                           \
    THANDLE_TYPE_DEFINE(PTR(T));                                                                                                                                        \
    static void THANDLE_PTR_DISPOSE(T)(PTR(T)* ptr)                                                                                                                     \
    {                                                                                                                                                                   \
        /*Codes_SRS_THANDLE_PTR_02_002: [ If the original dispose is non-NULL then THANDLE_PTR_DISPOSE(T) shall call dispose. ]*/                                       \
        if(ptr->dispose!=NULL)                                                                                                                                          \
        {                                                                                                                                                               \
            ptr->dispose(ptr->pointer);                                                                                                                                 \
        }                                                                                                                                                               \
        else                                                                                                                                                            \
        {                                                                                                                                                               \
            /*Codes_SRS_THANDLE_PTR_02_003: [ THANDLE_PTR_DISPOSE(T) shall return. ]*/                                                                                  \
            /*do nothing*/                                                                                                                                              \
        }                                                                                                                                                               \
    }                                                                                                                                                                   \
    THANDLE(PTR(T)) THANDLE_PTR_CREATE_WITH_MOVE(T)(T pointer, THANDLE_PTR_FREE_FUNC_TYPE_NAME(T) dispose )                                                             \
    {                                                                                                                                                                   \
        PTR(T) temp =                                                                                                                                                   \
        {                                                                                                                                                               \
            .pointer = pointer, /* this is "move" */                                                                                                                    \
            .dispose = dispose                                                                                                                                          \
        };                                                                                                                                                              \
        /*Codes_SRS_THANDLE_PTR_02_001: [ THANDLE_PTR_CREATE_WITH_MOVE(T) shall return what THANDLE_CREATE_FROM_CONTENT(PTR(T))(THANDLE_PTR_DISPOSE(T)) returns. ]*/    \
        return THANDLE_CREATE_FROM_CONTENT(PTR(T))(&temp, THANDLE_PTR_DISPOSE(T), NULL);                                                                                \
    }

#endif /*THANDLE_PTR_H*/
