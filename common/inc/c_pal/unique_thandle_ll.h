// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef UNIQUE_THANDLE_LL_H
#define UNIQUE_THANDLE_LL_H

#include <stddef.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/containing_record.h"
#include "c_pal/thandle_ll.h"

#include "umock_c/umock_c_prod.h"

/*UNIQUE_THANDLE(T) is a handle to a T that has a single owner.
A handle is an array of one slot, like jmp_buf. The compiler rejects UNIQUE_THANDLE(T) copy = handle;, assignments of handles and functions that return a handle.
A UNIQUE_THANDLE(T) parameter points to the caller's handle, and a const UNIQUE_THANDLE(T) parameter only reads it.
The slot holds a const pointer to a struct that is only declared. UNIQUE_THANDLE_ADOPT, UNIQUE_THANDLE_INITIALIZE_MOVE and UNIQUE_THANDLE_DISPOSE write the slot, and UNIQUE_THANDLE_GET_T converts its pointer to a T*.*/
#define UNIQUE_THANDLE(T) MU_C2(T, _UNIQUE_THANDLE)

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

/*given a previous type T, this introduces UNIQUE_THANDLE_ADOPT, which puts an object that UNIQUE_THANDLE_MALLOC or UNIQUE_THANDLE_MALLOC_FLEX returned, and that no handle holds, in a handle that is NULL or not initialized*/
#define UNIQUE_THANDLE_LL_ADOPT_MACRO(C, T)                                                                                            \
MU_SUPPRESS_WARNING(4505) /*warning C4505: 'function': unreferenced function with internal linkage has been removed*/                  \
static void UNIQUE_THANDLE_ADOPT(C)(UNIQUE_THANDLE(T) t, T* object)                                                                    \
{                                                                                                                                      \
    if (t == NULL)                                                                                                                     \
    {                                                                                                                                  \
        LogError("invalid argument UNIQUE_THANDLE(" MU_TOSTRING(T) ") t=%p, " MU_TOSTRING(T) "* object=%p", (void*)t, (void*)object);  \
    }                                                                                                                                  \
    else                                                                                                                               \
    {                                                                                                                                  \
        *(const struct UNIQUE_THANDLE_OBJECT_TAG(T)* volatile*)&t->object = (const struct UNIQUE_THANDLE_OBJECT_TAG(T)*)(void*)object; \
    }                                                                                                                                  \
}                                                                                                                                      \
MU_UNSUPPRESS_WARNING(4505)

/*given a previous type T, this introduces UNIQUE_THANDLE_GET_T, which returns the T that a handle holds, or NULL if t is NULL*/
#define UNIQUE_THANDLE_LL_GET_T_MACRO(C, T)                                                                             \
MU_SUPPRESS_WARNING(4505) /*warning C4505: 'function': unreferenced function with internal linkage has been removed*/   \
static T* UNIQUE_THANDLE_GET_T(C)(const UNIQUE_THANDLE(T) t)                                                            \
{                                                                                                                       \
    return (t == NULL) ? NULL : (T*)(void*)t->object;                                                                   \
}                                                                                                                       \
MU_UNSUPPRESS_WARNING(4505)

/*this is only useful during debugging, like THANDLE_INSPECT: from a UNIQUE_THANDLE(T) it returns the THANDLE_WRAPPER_TYPE_NAME(T) of its object, or NULL. Example: write ITEM_UNIQUE_INSPECT(item) in the Visual Studio watch window of a Debug build.
To see the fields of the object, cast its pointer: (ITEM*)item->object*/
#define UNIQUE_THANDLE_LL_INSPECT_MACRO(C, T)                                                                                                     \
const THANDLE_WRAPPER_TYPE_NAME(T)* UNIQUE_THANDLE_INSPECT(C)(const UNIQUE_THANDLE(T) t)                                                          \
{                                                                                                                                                 \
    return ((t == NULL) || (t->object == NULL)) ? NULL : CONTAINING_RECORD((const T*)(const void*)t->object, THANDLE_WRAPPER_TYPE_NAME(T), data); \
}

/*given a previous type T, this introduces UNIQUE_THANDLE_INITIALIZE_MOVE*/
#define UNIQUE_THANDLE_LL_INITIALIZE_MOVE_MACRO(C, T)                                                                                          \
void UNIQUE_THANDLE_INITIALIZE_MOVE(C)(UNIQUE_THANDLE(T) t1, UNIQUE_THANDLE(T) t2)                                                             \
{                                                                                                                                              \
    if (                                                                                                                                       \
        /*Codes_SRS_UNIQUE_THANDLE_43_008: [ If t1 is NULL then UNIQUE_THANDLE_INITIALIZE_MOVE shall return. ]*/                               \
        (t1 == NULL) ||                                                                                                                        \
        /*Codes_SRS_UNIQUE_THANDLE_43_009: [ If t2 is NULL then UNIQUE_THANDLE_INITIALIZE_MOVE shall return. ]*/                               \
        (t2 == NULL) ||                                                                                                                        \
        /*Codes_SRS_UNIQUE_THANDLE_43_010: [ If t1 is equal to t2 then UNIQUE_THANDLE_INITIALIZE_MOVE shall return. ]*/                        \
        (t1 == t2)                                                                                                                             \
        )                                                                                                                                      \
    {                                                                                                                                          \
        LogError("invalid argument UNIQUE_THANDLE(" MU_TOSTRING(T) ") t1=%p, UNIQUE_THANDLE(" MU_TOSTRING(T) ") t2=%p", (void*)t1, (void*)t2); \
    }                                                                                                                                          \
    else                                                                                                                                       \
    {                                                                                                                                          \
        /*Codes_SRS_UNIQUE_THANDLE_43_011: [ UNIQUE_THANDLE_INITIALIZE_MOVE shall store *t2 in *t1 and set *t2 to NULL. ]*/                    \
        *(const struct UNIQUE_THANDLE_OBJECT_TAG(T)* volatile*)&t1->object = t2->object;                                                       \
        *(const struct UNIQUE_THANDLE_OBJECT_TAG(T)* volatile*)&t2->object = NULL;                                                             \
    }                                                                                                                                          \
}

/*given a previous type T, this introduces UNIQUE_THANDLE_DISPOSE. *t is set to NULL before the dispose function of the object runs*/
#define UNIQUE_THANDLE_LL_DISPOSE_MACRO(C, T)                                                                                                                       \
void UNIQUE_THANDLE_DISPOSE(C)(UNIQUE_THANDLE(T) t)                                                                                                                 \
{                                                                                                                                                                   \
    if (t == NULL)                                                                                                                                                  \
    {                                                                                                                                                               \
        /*Codes_SRS_UNIQUE_THANDLE_43_012: [ If t is NULL then UNIQUE_THANDLE_DISPOSE shall return. ]*/                                                             \
        LogError("invalid argument UNIQUE_THANDLE(" MU_TOSTRING(T) ") t=%p", (void*)t);                                                                             \
    }                                                                                                                                                               \
    else if (t->object == NULL)                                                                                                                                     \
    {                                                                                                                                                               \
        /*Codes_SRS_UNIQUE_THANDLE_43_013: [ If *t is NULL then UNIQUE_THANDLE_DISPOSE shall return. ]*/                                                            \
    }                                                                                                                                                               \
    else                                                                                                                                                            \
    {                                                                                                                                                               \
        const T* previous = (const T*)(const void*)t->object;                                                                                                       \
        /*Codes_SRS_UNIQUE_THANDLE_43_014: [ UNIQUE_THANDLE_DISPOSE shall set *t to NULL. ]*/                                                                       \
        *(const struct UNIQUE_THANDLE_OBJECT_TAG(T)* volatile*)&t->object = NULL;                                                                                   \
        /*Codes_SRS_UNIQUE_THANDLE_43_015: [ UNIQUE_THANDLE_DISPOSE shall then call the dispose function passed when the handle was created, if it is not NULL. ]*/ \
        /*Codes_SRS_UNIQUE_THANDLE_43_016: [ UNIQUE_THANDLE_DISPOSE shall free the memory of the handle. ]*/                                                        \
        THANDLE_DEC_REF(C)(previous);                                                                                                                               \
    }                                                                                                                                                               \
}

/*macro to be used in a .c file*/
/*given a previous type T whose UNIQUE_THANDLE(T) is introduced, this defines the functions declared by UNIQUE_THANDLE_LL_TYPE_DECLARE and the static functions for the .c file that defines T, named after C. T's memory is allocated and freed with the given functions*/
/*it uses the THANDLE_LL building blocks that allocate, free and release an object, and THANDLE(T) for them. THANDLE_ASSIGN, THANDLE_INITIALIZE, THANDLE_MOVE, THANDLE_INITIALIZE_MOVE and THANDLE_INC_REF are not defined for T*/
#define UNIQUE_THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(C, T, malloc_function, malloc_flex_function, free_function) \
    THANDLE_MACRO(T);                                                                                                   \
    THANDLE_LL_TYPE_STRUCT_TYPE_DEFINE(T)                                                                               \
    THANDLE_LL_TYPE_STRUCT_VAR_DEFINE(T, malloc_function, malloc_flex_function, free_function)                          \
                                                                                                                        \
    MU_DEFINE_STRUCT(THANDLE_WRAPPER_TYPE_NAME(T), THANDLE_EXTRA_FIELDS(T), T, data);                                   \
                                                                                                                        \
    THANDLE_LL_MALLOC_WITH_MALLOC_FUNCTIONS_MACRO(C, T)                                                                 \
    THANDLE_LL_MALLOC_MACRO(C, T)                                                                                       \
    THANDLE_LL_MALLOC_WITH_EXTRA_SIZE_WITH_MALLOC_FUNCTIONS_MACRO(C, T)                                                 \
    THANDLE_LL_MALLOC_WITH_EXTRA_SIZE_MACRO(C, T)                                                                       \
    THANDLE_LL_FREE_MACRO(C, T)                                                                                         \
    THANDLE_LL_DEC_REF_MACRO(C, T)                                                                                      \
                                                                                                                        \
    UNIQUE_THANDLE_LL_ADOPT_MACRO(C, T)                                                                                 \
    UNIQUE_THANDLE_LL_GET_T_MACRO(C, T)                                                                                 \
    UNIQUE_THANDLE_LL_INSPECT_MACRO(C, T)                                                                               \
    UNIQUE_THANDLE_LL_INITIALIZE_MOVE_MACRO(C, T)                                                                       \
    UNIQUE_THANDLE_LL_DISPOSE_MACRO(C, T)

/*macro to be used in a .c file*/
/*same as UNIQUE_THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS, with T's memory allocated and freed with malloc, malloc_flex and free*/
#define UNIQUE_THANDLE_LL_TYPE_DEFINE(C, T)                                                                             \
    UNIQUE_THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(C, T, NULL, NULL, NULL)

#endif /*UNIQUE_THANDLE_LL_H*/
