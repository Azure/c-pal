// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef UNIQUE_THANDLE_COMPILE_CHECKS_H
#define UNIQUE_THANDLE_COMPILE_CHECKS_H

#include <stdint.h>

#include "macro_utils/macro_utils.h"

#include "c_pal/unique_thandle.h"

/*the ITEM and the list of the example in unique_thandle_requirements.md*/
typedef struct ITEM_TAG ITEM;

UNIQUE_THANDLE_TYPE_DECLARE(ITEM);

/*another type, whose handles are of another type than the handles of ITEM*/
typedef struct OTHER_TAG OTHER;

UNIQUE_THANDLE_TYPE_DECLARE(OTHER);

int item_create(int value, UNIQUE_THANDLE(ITEM) item);
int item_get_value(const UNIQUE_THANDLE(ITEM) item);
int items_get_sum(const UNIQUE_THANDLE(ITEM) items[], uint32_t count);

typedef struct LIST_TAG
{
    uint32_t count;
    UNIQUE_THANDLE(ITEM) items[4];
} LIST;

int list_add(LIST* list, UNIQUE_THANDLE(ITEM) item);
int list_get_first_value(const LIST* list);

#endif /*UNIQUE_THANDLE_COMPILE_CHECKS_H*/
