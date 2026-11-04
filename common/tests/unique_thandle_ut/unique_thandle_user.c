// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h" // IWYU pragma: keep
#include "c_pal/gballoc_hl_redirect.h" // IWYU pragma: keep

#include "c_pal/unique_thandle.h"

#include "unique_thandle_user.h"

struct ITEM_TAG
{
    int value;
};

UNIQUE_THANDLE_TYPE_DEFINE(ITEM);

typedef struct LIST_TAG
{
    uint32_t capacity;
    uint32_t count;
    UNIQUE_THANDLE(ITEM) items[];
} LIST;

int item_create(int value, UNIQUE_THANDLE(ITEM) item)
{
    int result;
    if (item == NULL)
    {
        LogError("invalid argument int value=%d, UNIQUE_THANDLE(ITEM) item=%p", value, (void*)item);
        result = MU_FAILURE;
    }
    else
    {
        ITEM* new_item = UNIQUE_THANDLE_MALLOC(ITEM)(NULL);
        if (new_item == NULL)
        {
            LogError("failure in UNIQUE_THANDLE_MALLOC(ITEM)(NULL)");
            result = MU_FAILURE;
        }
        else
        {
            new_item->value = value;
            UNIQUE_THANDLE_ADOPT(ITEM)(item, new_item);
            result = 0;
        }
    }
    return result;
}

int item_get_value(const UNIQUE_THANDLE(ITEM) item)
{
    int result;
    const ITEM* object = UNIQUE_THANDLE_GET_T(ITEM)(item);
    if (object == NULL)
    {
        LogError("invalid argument const UNIQUE_THANDLE(ITEM) item=%p", (const void*)item);
        result = INT_MAX;
    }
    else
    {
        result = object->value;
    }
    return result;
}

LIST_HANDLE list_create(uint32_t capacity)
{
    LIST* result = malloc_flex(sizeof(LIST), capacity, sizeof(UNIQUE_THANDLE(ITEM)));
    if (result == NULL)
    {
        LogError("failure in malloc_flex(sizeof(LIST)=%zu, capacity=%" PRIu32 ", sizeof(UNIQUE_THANDLE(ITEM))=%zu)", sizeof(LIST), capacity, sizeof(UNIQUE_THANDLE(ITEM)));
        /*return as is*/
    }
    else
    {
        /*the handles in items are not initialized: list_add moves each item into the next one*/
        result->capacity = capacity;
        result->count = 0;
    }
    return result;
}

void list_destroy(LIST_HANDLE list)
{
    if (list == NULL)
    {
        LogError("invalid argument LIST_HANDLE list=%p", (void*)list);
    }
    else
    {
        for (uint32_t i = 0; i < list->count; i++)
        {
            UNIQUE_THANDLE_DISPOSE(ITEM)(list->items[i]);
        }
        free(list);
    }
}

int list_add(LIST_HANDLE list, UNIQUE_THANDLE(ITEM) item)
{
    int result;
    if (
        (list == NULL) ||
        (item == NULL)
        )
    {
        LogError("invalid argument LIST_HANDLE list=%p, UNIQUE_THANDLE(ITEM) item=%p", (void*)list, (void*)item);
        result = MU_FAILURE;
    }
    else if (list->count == list->capacity)
    {
        LogError("the list is full, capacity=%" PRIu32 "", list->capacity);
        result = MU_FAILURE;
    }
    else
    {
        UNIQUE_THANDLE_INITIALIZE_MOVE(ITEM)(list->items[list->count], item);
        list->count++;
        result = 0;
    }
    return result;
}

uint32_t list_get_count(LIST_HANDLE list)
{
    uint32_t result;
    if (list == NULL)
    {
        LogError("invalid argument LIST_HANDLE list=%p", (void*)list);
        result = 0;
    }
    else
    {
        result = list->count;
    }
    return result;
}
