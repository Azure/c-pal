// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


/*this header contains macros for ref_counting a variable.

There are no upper bound checks related to uint32_t overflow because we expect that bigger issues are in
the system when more than 4 billion references exist to the same variable. In the case when such an overflow
occurs, the object's ref count will reach zero (while still having 0xFFFFFFFF references) and likely the
controlling code will take the decision to free the object's resources. Then, any of the 0xFFFFFFFF references
will interact with deallocated memory / resources resulting in an undefined behavior.
*/

#ifndef REFCOUNT_H
#define REFCOUNT_H

#ifdef __cplusplus
#include <cstdlib>
#include <cstdint>
#else
#include <stdlib.h>
#include <stdint.h>
#endif

#include "azure_macro_utils/macro_utils.h"

// Include the platform-specific file that defines atomic functionality
#include "azure_c_pal/gballoc_hl.h"
#include "azure_c_pal/gballoc_hl_redirect.h"
#include "azure_c_pal/interlocked.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define REFCOUNT_TYPE(type) \
struct MU_C2(MU_C2(REFCOUNT_, type), _TAG)

#define REFCOUNT_SHORT_TYPE(type) \
MU_C2(REFCOUNT_, type)

#define REFCOUNT_TYPE_DECLARE_CREATE(type) MU_C2(REFCOUNT_SHORT_TYPE(type), _Create)
#define REFCOUNT_TYPE_DECLARE_CREATE_WITH_EXTRA_SIZE(type) MU_C2(REFCOUNT_SHORT_TYPE(type), _Create_With_Extra_Size)
#define REFCOUNT_TYPE_CREATE(type) MU_C2(REFCOUNT_SHORT_TYPE(type), _Create)()
#define REFCOUNT_TYPE_CREATE_WITH_EXTRA_SIZE(type, size) MU_C2(REFCOUNT_SHORT_TYPE(type), _Create_With_Extra_Size)(size)
#define REFCOUNT_TYPE_DECLARE_DESTROY(type) MU_C2(REFCOUNT_SHORT_TYPE(type), _Destroy)
#define REFCOUNT_TYPE_DESTROY(type, var) MU_C2(REFCOUNT_SHORT_TYPE(type), _Destroy)(var)

/*this introduces a new refcount'd type based on another type */
/*and an initializer for that new type that also sets the ref count to 1. The type must not have a flexible array*/
/*the newly allocated memory shall be free'd by free()*/
/*and the ref counting is handled internally by the type in the _Create/ _Create_With_Extra_Size /_Clone /_Destroy functions */

/* Codes_SRS_REFCOUNT_01_005: [ REFCOUNT_TYPE_CREATE_WITH_EXTRA_SIZE shall allocate memory for the type that is ref counted (type) plus extra memory enough to hold size bytes. ]*/
/* Codes_SRS_REFCOUNT_01_006: [ On success it shall return a non-NULL handle to the allocated ref counted type type. ]*/
/* Codes_SRS_REFCOUNT_01_007: [ If any error occurs, REFCOUNT_TYPE_CREATE_WITH_EXTRA_SIZE shall return NULL. ]*/
#define DEFINE_CREATE_WITH_EXTRA_SIZE(type, malloc_func) \
static type* REFCOUNT_TYPE_DECLARE_CREATE_WITH_EXTRA_SIZE(type)(size_t size) \
{ \
    /* Codes_SRS_REFCOUNT_01_011: [ DEFINE_REFCOUNT_TYPE_WITH_CUSTOM_ALLOC shall behave like DEFINE_REFCOUNT_TYPE, but use malloc_func and free_func for memory allocation and free.  ]*/ \
    REFCOUNT_TYPE(type)* ref_counted = (REFCOUNT_TYPE(type)*)malloc_func(sizeof(REFCOUNT_TYPE(type)) + size); \
    type* result; \
    if (ref_counted == NULL) \
    { \
        result = NULL; \
    } \
    else \
    { \
        result = &ref_counted->counted; \
        INIT_REF(type, result); \
    } \
    return result; \
} \

/* Codes_SRS_REFCOUNT_01_002: [ REFCOUNT_TYPE_CREATE shall allocate memory for the type that is ref counted. ]*/
/* Codes_SRS_REFCOUNT_01_003: [ On success it shall return a non-NULL handle to the allocated ref counted type type. ]*/
/* Codes_SRS_REFCOUNT_01_004: [ If any error occurs, REFCOUNT_TYPE_CREATE shall return NULL. ]*/
#define DEFINE_CREATE(type, malloc_func) \
static type* REFCOUNT_TYPE_DECLARE_CREATE(type) (void) \
{ \
    return REFCOUNT_TYPE_DECLARE_CREATE_WITH_EXTRA_SIZE(type)(0); \
} \

/* Codes_SRS_REFCOUNT_01_008: [ REFCOUNT_TYPE_DESTROY shall free the memory allocated by REFCOUNT_TYPE_CREATE or REFCOUNT_TYPE_CREATE_WITH_EXTRA_SIZE. ]*/
/* Codes_SRS_REFCOUNT_01_009: [ If counted_type is NULL, REFCOUNT_TYPE_DESTROY shall return. ]*/
#define DEFINE_DESTROY(type, free_func) \
static void REFCOUNT_TYPE_DECLARE_DESTROY(type)(type* counted_type) \
{ \
    void* ref_counted = (void*)((unsigned char*)counted_type - offsetof(REFCOUNT_TYPE(type), counted)); \
    /* Codes_SRS_REFCOUNT_01_011: [ DEFINE_REFCOUNT_TYPE_WITH_CUSTOM_ALLOC shall behave like DEFINE_REFCOUNT_TYPE, but use malloc_func and free_func for memory allocation and free.  ]*/ \
    free_func(ref_counted); \
}

/* Codes_SRS_REFCOUNT_01_011: [ DEFINE_REFCOUNT_TYPE_WITH_CUSTOM_ALLOC shall behave like DEFINE_REFCOUNT_TYPE, but use malloc_func and free_func for memory allocation and free.  ]*/ \
#define DEFINE_REFCOUNT_TYPE_WITH_CUSTOM_ALLOC(type, malloc_func, free_func) \
REFCOUNT_TYPE(type) \
{ \
    volatile_atomic int32_t count; \
    type counted; \
}; \
DEFINE_CREATE_WITH_EXTRA_SIZE(type, malloc_func) \
DEFINE_CREATE(type, malloc_func) \
DEFINE_DESTROY(type, free_func) \

/* Codes_SRS_REFCOUNT_01_001: [ DEFINE_REFCOUNT_TYPE shall define the create/create_with_Extra_size/destroy functions for the type type. ]*/
#define DEFINE_REFCOUNT_TYPE(type) \
    /* Codes_SRS_REFCOUNT_01_010: [ Memory allocation/free shall be performed by using the functions malloc and free. ]*/ \
    DEFINE_REFCOUNT_TYPE_WITH_CUSTOM_ALLOC(type, malloc, free)

/*assuming that CONSTBUFFER_ARRAY_HANDLE is a type introduced with DEFINE_REFCOUNT_TYPE(CONSTBUFFER_ARRAY_HANDLE_DATA);
and "checkpointContent" is a variable of type CONSTBUFFER_ARRAY_HANDLE
then in order to see "count" / "counted" this is a good string to paste in Visual Studio's Watch window on 64 bit builds:

(struct REFCOUNT_CONSTBUFFER_ARRAY_HANDLE_DATA_TAG*)((unsigned char*)checkpointContent - 8)

*/

#define INC_REF(type, var) interlocked_increment(&((REFCOUNT_TYPE(type)*)((unsigned char*)var - offsetof(REFCOUNT_TYPE(type), counted)))->count)
#define DEC_REF(type, var) interlocked_decrement(&((REFCOUNT_TYPE(type)*)((unsigned char*)var - offsetof(REFCOUNT_TYPE(type), counted)))->count)
#define INIT_REF(type, var) interlocked_exchange(&((REFCOUNT_TYPE(type)*)((unsigned char*)var - offsetof(REFCOUNT_TYPE(type), counted)))->count, 1)

#ifdef __cplusplus
}
#endif

#endif /*REFCOUNT_H*/


