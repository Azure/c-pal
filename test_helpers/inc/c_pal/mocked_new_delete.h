// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef MOCKED_NEW_DELETE_H
#define MOCKED_NEW_DELETE_H

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

MOCKABLE_FUNCTION(, void*, mocked_malloc, size_t, size);

MOCKABLE_FUNCTION(, void, mocked_free, void*, ptr);

int use_mocks_for_new_delete(void);

void use_default_malloc_for_new_delete(void);

void lock_mocked_malloc(void);

void unlock_mocked_malloc(void);

#ifdef __cplusplus
}
#endif

#endif /* MOCKED_NEW_DELETE_H */
