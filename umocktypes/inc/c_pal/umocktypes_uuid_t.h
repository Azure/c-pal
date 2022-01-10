// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef UMOCKTYPES_UUID_T_H
#define UMOCKTYPES_UUID_T_H

#include "ctest.h"

#include "c_pal/uuid.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef UUID_T* uuid_ptr;

    void uuid_ptr_ToString(char* string, size_t bufferSize, const UUID_T* val);
    int uuid_ptr_Compare(const UUID_T* left, const UUID_T* right);

    void UUID_T_ToString(char* string, size_t bufferSize, const UUID_T val);
    int UUID_T_Compare(const UUID_T left, const UUID_T right);

    extern int umocktypes_UUID_T_register_types(void);

    extern char* umocktypes_stringify_uuid(const UUID_T** value);
    extern int umocktypes_are_equal_uuid(const UUID_T** left, const UUID_T** right);
    extern int umocktypes_copy_uuid(UUID_T** destination, const UUID_T** source);
    extern void umocktypes_free_uuid(UUID_T** value);

    CTEST_DECLARE_EQUALITY_ASSERTION_FUNCTIONS_FOR_TYPE(uuid_ptr);

#ifdef __cplusplus
}
#endif

#endif /* UMOCKTYPES_UUID_T_H */
