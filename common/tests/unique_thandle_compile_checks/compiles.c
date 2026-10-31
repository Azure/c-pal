// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/*the documented use of UNIQUE_THANDLE. It has to compile with warnings treated as errors, which shows that the cases in the other files fail for their own reasons*/

#include <stddef.h>
#include <stdint.h>

#include "macro_utils/macro_utils.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/unique_thandle.h"

#include "unique_thandle_compile_checks.h"

struct ITEM_TAG
{
    int value;
};

UNIQUE_THANDLE_TYPE_DEFINE(ITEM);

/*a list whose capacity is set when it is allocated*/
typedef struct FLEX_LIST_TAG
{
    uint32_t count;
    UNIQUE_THANDLE(ITEM) items[];
} FLEX_LIST;

static UNIQUE_THANDLE(ITEM) g_item = { { NULL } };

int item_create(int value, UNIQUE_THANDLE(ITEM) item)
{
    int result;
    ITEM* new_item = UNIQUE_THANDLE_MALLOC(ITEM)(NULL);
    if (new_item == NULL)
    {
        result = MU_FAILURE;
    }
    else
    {
        new_item->value = value;
        UNIQUE_THANDLE_ADOPT(ITEM)(item, new_item);
        result = 0;
    }
    return result;
}

int item_get_value(const UNIQUE_THANDLE(ITEM) item)
{
    const ITEM* object = UNIQUE_THANDLE_GET_T(ITEM)(item);
    return (object == NULL) ? 0 : object->value;
}

int items_get_sum(const UNIQUE_THANDLE(ITEM) items[], uint32_t count)
{
    int result = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        result += item_get_value(items[i]);
    }
    return result;
}

int list_add(LIST* list, UNIQUE_THANDLE(ITEM) item)
{
    int result;
    if (list->count == sizeof(list->items) / sizeof(list->items[0]))
    {
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

int list_get_first_value(const LIST* list)
{
    return item_get_value(list->items[0]);
}

int caller(LIST* list, FLEX_LIST* flex_list)
{
    static UNIQUE_THANDLE(ITEM) function_scope_item = { { NULL } };
    int result;
    UNIQUE_THANDLE(ITEM) item = { { NULL } };
    UNIQUE_THANDLE(ITEM) not_initialized;
    if (item_create(42, item) != 0)
    {
        result = MU_FAILURE;
    }
    else
    {
        result = item_get_value(item);
        if (list_add(list, item) != 0)
        {
            result = MU_FAILURE;
        }
        else
        {
            result += list_get_first_value(list) + items_get_sum(list->items, list->count);
        }
        UNIQUE_THANDLE_DISPOSE(ITEM)(item);
    }

    if (item_create(43, not_initialized) == 0)
    {
        UNIQUE_THANDLE_INITIALIZE_MOVE(ITEM)(flex_list->items[flex_list->count], not_initialized);
        flex_list->count++;
        result += items_get_sum(flex_list->items, flex_list->count);
    }
    else
    {
        result = MU_FAILURE;
    }

    UNIQUE_THANDLE_INITIALIZE_MOVE(ITEM)(g_item, function_scope_item);
    UNIQUE_THANDLE_DISPOSE(ITEM)(g_item);
    return result;
}
