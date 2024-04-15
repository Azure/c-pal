// Copyright (c) Microsoft. All rights reserved.

#ifndef MALLOC_MULTI_FLEX_H
#define MALLOC_MULTI_FLEX_H

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
#include <cstdalign>
#else
#include <stddef.h>
#include <stdint.h>
#include <stdalign.h>
#endif

#include "macro_utils/macro_utils.h"

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

    //arg validation
    #define MALLOC_MULTI_FLEX_ARG_VALIDATION(arg1, arg2) \
        if (MU_C2(arg2, _count) > 0 && sizeof(arg1) != sizeof(parent_struct_pointer->arg2[0]))\
        {\
            LogError("Data type size type mistmatch. Declared type has size = %zu but actual member has size = %zu", sizeof(arg1), sizeof(parent_struct_pointer->arg2[0]));\
            free(parent_struct_pointer);\
            return NULL;\
        }

    #define MALLOC_MULTI_FLEX_ARG_VALIDATION_ARRAY_FIELDS(...) \
            MU_FOR_EACH_2(MALLOC_MULTI_FLEX_ARG_VALIDATION, __VA_ARGS__)

    #define MALLOC_MULTI_FLEX_ARGS_VALIDATION(array_fields) \
            MU_C2A(MALLOC_MULTI_FLEX_ARG_VALIDATION_, array_fields)

    //overflow check
    #define MALLOC_MULTI_FLEX_ARG_OVERFLOW_CHECK(arg1, arg2) \
        if ((sizeof(arg1) != 0 && SIZE_MAX / sizeof(arg1) < MU_C2(arg2, _count)) || (SIZE_MAX - size_required < MU_C2(arg2, _count) * sizeof(arg1)) || (SIZE_MAX - (size_required + MU_C2(arg2, _count) * sizeof(arg1)) < alignof(arg1)))\
        {\
            LogError("Size required to allocate memory for struct exceeds %" PRIu64 "", SIZE_MAX);\
            return NULL;\
        }\
        size_required += MU_C2(arg2, _count) * sizeof(arg1) + alignof(arg1) - 1;

    #define MALLOC_MULTI_FLEX_ARG_OVERFLOW_CHECK_ARRAY_FIELDS(...) \
        MU_FOR_EACH_2(MALLOC_MULTI_FLEX_ARG_OVERFLOW_CHECK, __VA_ARGS__)

    #define MALLOC_MULTI_FLEX_ARGS_OVERFLOW_CHECK(array_fields) \
        MU_C2A(MALLOC_MULTI_FLEX_ARG_OVERFLOW_CHECK_, array_fields)

    #define MALLOC_MULTI_FLEX_DEFINE_ARG_LIST_VALUE(arg1, arg2) \
        , size_t MU_C2(arg2, _count)

    #define MALLOC_MULTI_FLEX_DEFINE_ARG_LIST_VALUE_ARRAY_FIELDS(...) \
        MU_FOR_EACH_2(MALLOC_MULTI_FLEX_DEFINE_ARG_LIST_VALUE, __VA_ARGS__)

    #define MALLOC_MULTI_FLEX_DEFINE_ARG_LIST_VALUES(array_fields) \
            MU_C2A(MALLOC_MULTI_FLEX_DEFINE_ARG_LIST_VALUE_, array_fields)

    #define MALLOC_MULTI_FLEX_DECLARE_ARG_LIST_VALUE(arg1, arg2) \
            , size_t, MU_C2(arg2, _count)

    #define MALLOC_MULTI_FLEX_DECLARE_ARG_LIST_VALUE_ARRAY_FIELDS(...) \
            MU_FOR_EACH_2(MALLOC_MULTI_FLEX_DECLARE_ARG_LIST_VALUE, __VA_ARGS__)

    #define MALLOC_MULTI_FLEX_DECLARE_ARG_LIST_VALUES(array_fields) \
            MU_C2A(MALLOC_MULTI_FLEX_DECLARE_ARG_LIST_VALUE_, array_fields)

    #define MALLOC_MULTI_FLEX_ASSIGN_INTERNAL_STRUCT_PTR(arg1, arg2) \
        parent_struct_pointer->arg2 = (arg1*)(pointer_iterator + alignof(arg1) - ((uintptr_t)pointer_iterator % alignof(arg1)));\
        pointer_iterator = (uintptr_t)pointer_iterator + MU_C2(arg2, _count) * sizeof(arg1);

    #define MALLOC_MULTI_FLEX_ASSIGN_INTERNAL_STRUCT_PTR_ARRAY_FIELDS(...) \
        MU_FOR_EACH_2(MALLOC_MULTI_FLEX_ASSIGN_INTERNAL_STRUCT_PTR, __VA_ARGS__)

    #define MALLOC_MULTI_FLEX_ASSIGN_INTERNAL_STRUCT_PTRS(array_fields) \
            MU_C2A(MALLOC_MULTI_FLEX_ASSIGN_INTERNAL_STRUCT_PTR_, array_fields)

    /* Codes_MALLOC_MULTI_FLEX_24_005: [ MALLOC_MULTI_FLEX shall expand type to the name of the malloc function in the format of: MALLOC_MULTI_FLEX_type. ]*/
    #define MALLOC_MULTI_FLEX(type) \
        MU_C2(malloc_multi_flex_, type) \

    #define MALLOC_MULTI_FLEX_DEFINE_FIELD_MEMBER(arg1, arg2) \
            arg1 arg2;

    #define MALLOC_MULTI_FLEX_DEFINE_MEMBERS_FIELDS(...) \
            MU_FOR_EACH_2(MALLOC_MULTI_FLEX_DEFINE_FIELD_MEMBER, __VA_ARGS__)

    #define MALLOC_MULTI_FLEX_DEFINE_ARRAY_FIELD_MEMBER(arg1, arg2) \
                arg1* arg2;

    #define MALLOC_MULTI_FLEX_DEFINE_MEMBERS_ARRAY_FIELDS(...) \
                MU_FOR_EACH_2(MALLOC_MULTI_FLEX_DEFINE_ARRAY_FIELD_MEMBER, __VA_ARGS__)

    #define GENERATE_MULTI_MALLOC_STRUCT(fields) \
            MU_C2A(MALLOC_MULTI_FLEX_DEFINE_MEMBERS_, fields) \

    #define DECLARE_MALLOC_MULTI_FLEX(type, fields, array_fields)\
        typedef struct MU_C2(type, _TAG) \
        { \
            GENERATE_MULTI_MALLOC_STRUCT(fields)\
            GENERATE_MULTI_MALLOC_STRUCT(array_fields)\
        } type; \
        MOCKABLE_FUNCTION(, void*, MALLOC_MULTI_FLEX(type), size_t, parent_struct_size MALLOC_MULTI_FLEX_DECLARE_ARG_LIST_VALUES(array_fields)); \

    #define DEFINE_MALLOC_MULTI_FLEX(type, fields, array_fields)\
        void* MALLOC_MULTI_FLEX(type)(size_t parent_struct_size MALLOC_MULTI_FLEX_DEFINE_ARG_LIST_VALUES(array_fields)) \
        {\
            size_t size_required = parent_struct_size;\
            /* Codes_SRS_MALLOC_MULTI_FLEX_24_001: [ If the total amount of memory required to allocate the type along with its members exceeds SIZE_MAX then DEFINE_MALLOC_MULTI_FLEX shall fail and return NULL. ]*/ \
            MALLOC_MULTI_FLEX_ARGS_OVERFLOW_CHECK(array_fields)\
            /* Codes_SRS_MALLOC_MULTI_FLEX_24_002: [ DEFINE_MALLOC_MULTI_FLEX shall call malloc to allocate memory for the struct and its members. ]*/ \
            type* parent_struct_pointer = malloc(size_required);\
            if (parent_struct_pointer == NULL)\
            {\
                LogError("malloc(size_required = %zu) failed", size_required);\
                return NULL;\
            }\
            uintptr_t pointer_iterator = (uintptr_t)parent_struct_pointer + parent_struct_size; \
            MALLOC_MULTI_FLEX_ARGS_VALIDATION(array_fields)\
            /* Codes_SRS_MALLOC_MULTI_FLEX_24_003: [ DEFINE_MALLOC_MULTI_FLEX shall assign address pointers to all the member arrays. ]*/ \
            MALLOC_MULTI_FLEX_ASSIGN_INTERNAL_STRUCT_PTRS(array_fields)\
            /* Codes_SRS_MALLOC_MULTI_FLEX_24_004: [ DEFINE_MALLOC_MULTI_FLEX shall succeed and return the address returned by malloc. ]*/ \
            return parent_struct_pointer;\
        }\

#ifdef __cplusplus
}
#endif

#endif // MALLOC_MULTI_FLEX_H
