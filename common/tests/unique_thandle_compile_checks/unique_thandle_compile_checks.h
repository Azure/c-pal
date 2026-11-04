// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef UNIQUE_THANDLE_COMPILE_CHECKS_H
#define UNIQUE_THANDLE_COMPILE_CHECKS_H

#include <stdint.h>

#include "macro_utils/macro_utils.h"

#include "c_pal/unique_thandle.h"

/*the headers of the examples in unique_thandle_requirements.md: item.h, list.h and other.h*/

/*item.h*/
typedef struct ITEM_TAG ITEM;

UNIQUE_THANDLE_TYPE_DECLARE(ITEM);

int item_create(int value, UNIQUE_THANDLE(ITEM) item);

/*list.h*/
typedef struct NODE_TAG
{
    struct NODE_TAG* next;
    UNIQUE_THANDLE(ITEM) item;
} NODE;

typedef struct LIST_TAG
{
    NODE head; /*holds no item*/
    NODE* end; /*the last node, &head when the list is empty*/
} LIST;

typedef LIST* LIST_HANDLE;

int list_insert_num(LIST_HANDLE list, int n);
int list_insert_item(LIST_HANDLE list, UNIQUE_THANDLE(ITEM) item);

/*other.h*/
typedef struct OTHER_TAG OTHER;

UNIQUE_THANDLE_TYPE_DECLARE(OTHER);

#endif /*UNIQUE_THANDLE_COMPILE_CHECKS_H*/
