// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef TIMER_H
#define TIMER_H

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct TIMER_HANDLE_DATA_TAG* TIMER_HANDLE;

MOCKABLE_FUNCTION(, TIMER_HANDLE, timer_create_new);
MOCKABLE_FUNCTION(, int, timer_start, TIMER_HANDLE, handle);
MOCKABLE_FUNCTION(, double, timer_get_elapsed, TIMER_HANDLE, timer);
MOCKABLE_FUNCTION(, double, timer_get_elapsed_ms, TIMER_HANDLE, timer);
MOCKABLE_FUNCTION(, void, timer_destroy, TIMER_HANDLE, timer);
MOCKABLE_FUNCTION(, double, timer_global_get_elapsed);
MOCKABLE_FUNCTION(, double, timer_global_get_elapsed_ms);
MOCKABLE_FUNCTION(, double, timer_global_get_elapsed_us);

#ifdef __cplusplus
}
#endif

#endif /* TIMER_H */
