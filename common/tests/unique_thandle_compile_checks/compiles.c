// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/*the examples of unique_thandle_requirements.md that compile, then other uses of UNIQUE_THANDLE.
It has to compile with warnings treated as errors, which shows that the cases in the other files fail for their own reasons*/

#include <stddef.h>
#include <stdint.h>

#include "macro_utils/macro_utils.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/unique_thandle.h"

#include "unique_thandle_compile_checks.h"

/*item.c*/
struct ITEM_TAG
{
    int value;
};

UNIQUE_THANDLE_TYPE_DEFINE(ITEM);

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

/*list.c*/
int list_insert_num(LIST_HANDLE list, int n)
{
    int result;

    UNIQUE_THANDLE(ITEM) item = UNIQUE_THANDLE_NULL;
    if (item_create(n, item) != 0)
    {
        result = MU_FAILURE;
    }
    else
    {
        if (list_insert_item(list, item) != 0)
        {
            result = MU_FAILURE;
        }
        else
        {
            result = 0;
            goto all_ok;
        }
        UNIQUE_THANDLE_DISPOSE(ITEM)(item);
    }
all_ok:
    return result;
}

int list_insert_item(LIST_HANDLE list, UNIQUE_THANDLE(ITEM) item)
{
    int result;
    if (list == NULL || item == NULL)
    {
        result = MU_FAILURE;
    }
    else
    {
        NODE* node = malloc(sizeof(NODE));
        if (node == NULL)
        {
            result = MU_FAILURE;
        }
        else
        {
            node->next = NULL;
            UNIQUE_THANDLE_INITIALIZE_MOVE(ITEM)(node->item, item);
            list->end->next = node;
            list->end = node;
            result = 0;
        }
    }
    return result;
}

/*a function that reads the item through a handle that is not const*/
int item_get_value(UNIQUE_THANDLE(ITEM) item)
{
    UNIQUE_THANDLE(ITEM) temp = UNIQUE_THANDLE_NULL;

    UNIQUE_THANDLE_INITIALIZE_MOVE(ITEM)(temp, item);

    ITEM* item_ptr = UNIQUE_THANDLE_GET_T(ITEM)(temp);

    return item_ptr->value;
}

int caller(void)
{
    UNIQUE_THANDLE(ITEM) item = UNIQUE_THANDLE_NULL;
    item_create(42, item);

    int val = item_get_value(item);

    /*item is now NULL: item_get_value moved the item into temp, which leaked it. Another call to item_get_value dereferences NULL and crashes*/
    int val2 = item_get_value(item); // will crash

    return val + val2;
}

/*other uses*/

/*a list whose capacity is set when it is allocated*/
typedef struct FLEX_LIST_TAG
{
    uint32_t count;
    UNIQUE_THANDLE(ITEM) items[];
} FLEX_LIST;

static UNIQUE_THANDLE(ITEM) g_item = UNIQUE_THANDLE_NULL;

/*reads an array of handles, which callers pass without a cast*/
static int items_get_sum(const UNIQUE_THANDLE(ITEM) items[], uint32_t count)
{
    int result = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        const ITEM* object = UNIQUE_THANDLE_GET_T(ITEM)(items[i]);
        result += (object == NULL) ? 0 : object->value;
    }
    return result;
}

int other_uses(FLEX_LIST* flex_list)
{
    static UNIQUE_THANDLE(ITEM) function_scope_item = UNIQUE_THANDLE_NULL;
    int result;
    UNIQUE_THANDLE(ITEM) not_initialized;
    if (item_create(43, not_initialized) == 0)
    {
        UNIQUE_THANDLE_INITIALIZE_MOVE(ITEM)(flex_list->items[flex_list->count], not_initialized);
        flex_list->count++;
        result = items_get_sum(flex_list->items, flex_list->count);
    }
    else
    {
        result = MU_FAILURE;
    }

    UNIQUE_THANDLE_INITIALIZE_MOVE(ITEM)(g_item, function_scope_item);
    UNIQUE_THANDLE_DISPOSE(ITEM)(g_item);
    return result;
}
