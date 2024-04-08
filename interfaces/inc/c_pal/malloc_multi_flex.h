// Copyright (c) Microsoft. All rights reserved.

#ifndef MALLOC_MULTI_FLEX_H
#define MALLOC_MULTI_FLEX_H

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
#else
#include <stddef.h>
#include <stdint.h>
#endif

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

    #define alignof _Alignof

    #define MALLOC_MULTI_FLEX_ARG_OVERFLOW_CHECK(arg1, arg2) \
        if ((sizeof(arg1) != 0 && SIZE_MAX / sizeof(arg1) < MU_C2(arg2, _count)) || (SIZE_MAX - size_required < MU_C2(arg2, _count) * sizeof(arg1)))\
        {\
            return NULL;\
        }\
        size_required += MU_C2(arg2, _count) * sizeof(arg1) + alignof(arg1);

    #define MALLOC_MULTI_FLEX_ARGS_OVERFLOW_CHECK(...) \
        MU_FOR_EACH_2(MALLOC_MULTI_FLEX_ARG_OVERFLOW_CHECK, __VA_ARGS__)

    #define MALLOC_MULTI_FLEX_ARG_LIST_VALUE(arg1, arg2) \
        , uint32_t MU_C2(arg2, _count)

    #define MALLOC_MULTI_FLEX_ARG_LIST_VALUES(...) \
        MU_FOR_EACH_2(MALLOC_MULTI_FLEX_ARG_LIST_VALUE, __VA_ARGS__)

    #define MALLOC_MULTI_FLEX_ASSIGN_INTERNAL_STRUCT_PTR(arg1, arg2) \
        parent_struct_pointer->arg2 = (arg1*)(pointer_iterator + alignof(arg1) - ((uintptr_t)pointer_iterator % alignof(arg1)));\
        pointer_iterator = (uintptr_t)pointer_iterator + MU_C2(arg2, _count) * sizeof(arg1);

    #define MALLOC_MULTI_FLEX_ASSIGN_INTERNAL_STRUCT_PTRS(...) \
        MU_FOR_EACH_2(MALLOC_MULTI_FLEX_ASSIGN_INTERNAL_STRUCT_PTR, __VA_ARGS__)

    /*Codes_MALLOC_MULTI_FLEX_24_005: [ MALLOC_MULTI_FLEX shall expand type to the name of the malloc function in the format of: MALLOC_MULTI_FLEX_type. ]*/
    #define MALLOC_MULTI_FLEX(type) \
        MU_C2(malloc_multi_flex_, type) \

    #define DEFINE_MALLOC_MULTI_FLEX(type, ...)\
        static void* MALLOC_MULTI_FLEX(type)(size_t parent_struct_size MALLOC_MULTI_FLEX_ARG_LIST_VALUES(__VA_ARGS__))   \
        {\
            size_t size_required = parent_struct_size;\
            /* Codes_SRS_MALLOC_MULTI_FLEX_24_001: [ If the total amount of memory required to allocate the type along with its members exceeds SIZE_MAX then DEFINE_MALLOC_MULTI_FLEX shall fail and return NULL. ]*/ \
            MALLOC_MULTI_FLEX_ARGS_OVERFLOW_CHECK(__VA_ARGS__)\
            /* Codes_SRS_MALLOC_MULTI_FLEX_24_002: [ DEFINE_MALLOC_MULTI_FLEX shall call malloc to allocate memory for the struct and its members. ]*/ \
            type* parent_struct_pointer = malloc(size_required);\
            uintptr_t pointer_iterator = (uintptr_t)parent_struct_pointer + parent_struct_size; \
            /* Codes_SRS_MALLOC_MULTI_FLEX_24_003: [ DEFINE_MALLOC_MULTI_FLEX shall assign address pointers to all the member arrays. ]*/ \
            MALLOC_MULTI_FLEX_ASSIGN_INTERNAL_STRUCT_PTRS(__VA_ARGS__)\
            /* Codes_SRS_MALLOC_MULTI_FLEX_24_004: [ DEFINE_MALLOC_MULTI_FLEX shall succeed and return the address returned by malloc. ]*/ \
            return parent_struct_pointer;\
        }\

#ifdef __cplusplus
}
#endif

#endif // MALLOC_MULTI_FLEX_H
