// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_TIMER_H
#define REAL_TIMER_H

#include "macro_utils/macro_utils.h"

#include "c_pal/timer.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_TIMER_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2, \
        timer_create_new, \
        timer_start, \
        timer_get_elapsed, \
        timer_global_get_elapsed_ms, \
        timer_destroy \
)

#ifdef __cplusplus
extern "C" {
#endif


TIMER_HANDLE real_timer_create_new(void);

int real_timer_start(TIMER_HANDLE handle);

double real_timer_get_elapsed(TIMER_HANDLE handle);

double real_timer_get_elapsed_ms(TIMER_HANDLE handle);

double real_timer_global_get_elapsed_s(void);

double real_timer_global_get_elapsed_ms(void);

double real_timer_global_get_elapsed_us(void);

void real_timer_destroy(TIMER_HANDLE handle);

#ifdef __cplusplus
}
#endif

#endif // REAL_TIMER_H
