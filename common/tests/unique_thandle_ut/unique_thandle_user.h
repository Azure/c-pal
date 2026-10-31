// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef UNIQUE_THANDLE_USER_H
#define UNIQUE_THANDLE_USER_H

#include <stdint.h>

#include "macro_utils/macro_utils.h"

#include "c_pal/unique_thandle.h"
#include "c_pal/unique_thandle_ll.h" // for UNIQUE_THANDLE

/*the ITEM and the list of the example in unique_thandle_requirements.md. ITEM is a struct in unique_thandle_user.c. Its UNIQUE_THANDLE functions are declared here and defined in unique_thandle_user.c*/
typedef struct ITEM_TAG ITEM;

UNIQUE_THANDLE_TYPE_DECLARE(ITEM);

typedef struct LIST_TAG* LIST_HANDLE;

/*puts a new item in item*/
int item_create(int value, UNIQUE_THANDLE(ITEM) item);

/*uses the item during the call*/
int item_get_value(const UNIQUE_THANDLE(ITEM) item);

LIST_HANDLE list_create(uint32_t capacity);

/*disposes the items of the list*/
void list_destroy(LIST_HANDLE list);

/*takes the item when it succeeds*/
int list_add(LIST_HANDLE list, UNIQUE_THANDLE(ITEM) item);

uint32_t list_get_count(LIST_HANDLE list);

#endif /*UNIQUE_THANDLE_USER_H*/
