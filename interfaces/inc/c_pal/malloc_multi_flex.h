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

    #define ARG_LIST_VALUE_ARG(arg_2) \
    , uint32_t arg_2

    #define ARG_LIST_VALUES(...) \
        MU_FOR_EACH_1(ARG_LIST_VALUE_ARG, __VA_ARGS__)

    #define ADD_MULTI_FLEX_ARG_VALUE_ARG(arg) \
        + arg

    #define ADD_MULTIFLEX_ARG_VALUES(...) \
        MU_FOR_EACH_1(ADD_MULTI_FLEX_ARG_VALUE_ARG, __VA_ARGS__)

    #define ASSIGN_INTERNAL_STRUCT_PTR(arg) \
        parent_struct_pointer->arg = pointer_iterator;\
        pointer_iterator=(char*)pointer_iterator + arg;

    #define ASSIGN_INTERNAL_STRUCT_PTRS(...) \
        MU_FOR_EACH_1(ASSIGN_INTERNAL_STRUCT_PTR, __VA_ARGS__)

    #define MALLOC_MULTI_FLEX(type, ...)\
    static void* pointer_iterator;\
    static void* malloc_multi_flex(size_t parent_struct_size ARG_LIST_VALUES(__VA_ARGS__))   \
    {\
        type* parent_struct_pointer = malloc(parent_struct_size ADD_MULTIFLEX_ARG_VALUES(__VA_ARGS__));\
        pointer_iterator = (char*)parent_struct_pointer + parent_struct_size; \
        ASSIGN_INTERNAL_STRUCT_PTRS(__VA_ARGS__)\
        return parent_struct_pointer;\
    }\

#ifdef __cplusplus
}
#endif

#endif // MALLOC_MULTI_FLEX_H
