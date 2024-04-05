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

    #define ARG_OVERFLOW_CHECK(arg) \
        if ((MU_C2(arg, _size) != 0 && SIZE_MAX / MU_C2(arg, _size) < MU_C2(arg, _count)) || (SIZE_MAX - size_tracker < MU_C2(arg, _count) * MU_C2(arg, _size)))\
        {\
            return NULL;\
        }\

    #define ARGS_OVERFLOW_CHECK(...) \
        size_t size_tracker = sizeofmain;\
        MU_FOR_EACH_1(ARG_OVERFLOW_CHECK, __VA_ARGS__)

    #define ARG_LIST_VALUE_ARG(arg) \
        , uint32_t MU_C2(arg, _count), uint32_t MU_C2(arg, _size)

    #define ARG_LIST_VALUES(...) \
        MU_FOR_EACH_1(ARG_LIST_VALUE_ARG, __VA_ARGS__)

    #define ASSIGN_INTERNAL_STRUCT_PTR(arg) \
        parent_struct_pointer->arg = pointer_iterator;\
        pointer_iterator = (char*)pointer_iterator + MU_C2(arg, _count) * MU_C2(arg, _size);;

    #define ASSIGN_INTERNAL_STRUCT_PTRS(...) \
        MU_FOR_EACH_1(ASSIGN_INTERNAL_STRUCT_PTR, __VA_ARGS__)

    //Codes_MALLOC_MULTI_FLEX_24_005: [ MALLOC_MULTI_FLEX shall expand type to the name of the malloc function in the format of: MALLOC_MULTI_FLEX_type. ]
    #define MALLOC_MULTI_FLEX(type) \
        MU_C2(malloc_multi_flex, type) \

    #define DEFINE_MALLOC_MULTI_FLEX(type, ...)\
        static void* MALLOC_MULTI_FLEX(type)(size_t parent_struct_size ARG_LIST_VALUES(__VA_ARGS__))   \
        {\
            //Codes_SRS_MALLOC_MULTI_FLEX_24_001: [ If the total amount of memory required to allocate the type along with its members exceeds SIZE_MAX then DEFINE_MALLOC_MULTI_FLEX shall fail and return NULL. ]\
            ARGS_OVERFLOW_CHECK(__VA_ARGS__)\
            //Codes_SRS_MALLOC_MULTI_FLEX_24_002: [ DEFINE_MALLOC_MULTI_FLEX shall call malloc to allocate memory for the struct and its members. ]\
            type* parent_struct_pointer = malloc(parent_struct_size + size_tracker);\
            void* pointer_iterator = (char*)parent_struct_pointer + parent_struct_size; \
            //Codes_SRS_MALLOC_MULTI_FLEX_24_003: [ DEFINE_MALLOC_MULTI_FLEX shall assign address pointers to all the member arrays. ]\
            ASSIGN_INTERNAL_STRUCT_PTRS(__VA_ARGS__)\
            //Codes_SRS_MALLOC_MULTI_FLEX_24_004: [ DEFINE_MALLOC_MULTI_FLEX shall succeed and return the address returned by malloc. ]\
            return parent_struct_pointer;\
        }\

#ifdef __cplusplus
}
#endif

#endif // MALLOC_MULTI_FLEX_H
