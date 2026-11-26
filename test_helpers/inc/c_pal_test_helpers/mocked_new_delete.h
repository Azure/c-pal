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

MOCKABLE_FUNCTION(, void*, mocked_new_delete_malloc, size_t, size);

int mocked_new_delete_enable(void);

void mocked_new_delete_disable(void);

void mocked_new_delete_lock(void);

void mocked_new_delete_unlock(void);

#ifdef __cplusplus
}
#endif

#endif // MOCKED_NEW_DELETE_H
