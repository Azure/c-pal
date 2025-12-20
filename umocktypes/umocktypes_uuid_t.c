// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "ctest.h"

#include "umock_c/umocktypes.h"

#include "umock_c/umock_log.h"

#include "c_pal/uuid.h"

#include "c_pal/umocktypes_uuid_t.h"

void uuid_ptr_ToString(char* string, size_t bufferSize, const UUID_T* val)
{
    (void)snprintf(string, bufferSize, "%" PRI_UUID_T "", UUID_T_VALUES(*val));
}

int uuid_ptr_Compare(const UUID_T* left, const UUID_T* right)
{
    return !UUID_T_IS_EQUAL(*left, *right);
}

void UUID_T_ToString(char* string, size_t bufferSize, const UUID_T val)
{
    (void)snprintf(string, bufferSize, "%" PRI_UUID_T "", UUID_T_VALUES(val));
}

int UUID_T_Compare(const UUID_T left, const UUID_T right)
{
    return UUID_T_IS_EQUAL(left, right) ? 0 : 1;
}

char* umocktypes_stringify_UUID_T(const UUID_T* value)
{
    char* result;

    if (value == NULL)
    {
        UMOCK_LOG("umocktypes_stringify_uuid: NULL value.");
        result = NULL;
    }
    else
    {
        // 2 characters per byte plus 4 dashes
        size_t length = sizeof(UUID_T) * 2 + 4;
        result = malloc(length + 1);
        if (result == NULL)
        {
            UMOCK_LOG("umocktypes_stringify_uuid: Cannot allocate memory for result.");
        }
        else
        {
            (void)snprintf(result, length + 1, "%" PRI_UUID_T, UUID_T_VALUES(*value));
        }
    }

    return result;
}

int umocktypes_are_equal_UUID_T(const UUID_T* left, const UUID_T* right)
{
    int result;

    if ((left == NULL) || (right == NULL))
    {
        UMOCK_LOG("umocktypes_are_equal_uuid: Bad arguments:left = %p, right = %p.", left, right);
        result = -1;
    }
    else
    {
        result = UUID_T_IS_EQUAL(*left, *right) ? 1 : 0;
    }

    return result;
}

int umocktypes_copy_UUID_T(UUID_T* destination, const UUID_T* source)
{
    int result;

    if ((destination == NULL) || (source == NULL))
    {
        UMOCK_LOG("umocktypes_copy_uuid: Bad arguments: destination = %p, source = %p.",
            destination, source);
        result = MU_FAILURE;
    }
    else
    {
        *destination = *source;
        result = 0;
    }

    return result;
}

void umocktypes_free_UUID_T(UUID_T* value)
{
    (void)value;
}

int umocktypes_UUID_T_register_types(void)
{
    int result;

    if (
        (REGISTER_TYPE(UUID_T, UUID_T) != 0) ||
        (REGISTER_TYPE(const UUID_T, UUID_T) != 0) ||
        (REGISTER_TYPE(const_UUID_T, UUID_T) != 0)
        )
    {
        UMOCK_LOG("umocktypes_UUID_T_register_types: Cannot register types.");
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }

    return result;
}

CTEST_DEFINE_EQUALITY_ASSERTION_FUNCTIONS_FOR_TYPE(uuid_ptr, );
CTEST_DEFINE_EQUALITY_ASSERTION_FUNCTIONS_FOR_TYPE(UUID_T, );
