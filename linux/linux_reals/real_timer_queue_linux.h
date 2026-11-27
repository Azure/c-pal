// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef REAL_TIMER_QUEUE_LINUX_H
#define REAL_TIMER_QUEUE_LINUX_H

#include <stdbool.h>
#include <stdint.h>

#include "macro_utils/macro_utils.h"

#include "c_pal/timer_queue_linux.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_TIMER_QUEUE_LINUX_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2, \
        timer_queue_create, \
        timer_queue_destroy, \
        timer_queue_reserve, \
        timer_queue_unreserve, \
        timer_queue_entry_init, \
        timer_queue_arm, \
        timer_queue_disarm, \
        timer_queue_get_next_due_time, \
        timer_queue_pop_due \
    )

TIMER_QUEUE_HANDLE real_timer_queue_create(void);
void real_timer_queue_destroy(TIMER_QUEUE_HANDLE timer_queue);
int real_timer_queue_reserve(TIMER_QUEUE_HANDLE timer_queue);
void real_timer_queue_unreserve(TIMER_QUEUE_HANDLE timer_queue);
void real_timer_queue_entry_init(TIMER_QUEUE_ENTRY* entry);
bool real_timer_queue_arm(TIMER_QUEUE_HANDLE timer_queue, TIMER_QUEUE_ENTRY* entry, uint64_t due_time_ms, uint32_t period_ms);
void real_timer_queue_disarm(TIMER_QUEUE_HANDLE timer_queue, TIMER_QUEUE_ENTRY* entry);
uint64_t real_timer_queue_get_next_due_time(TIMER_QUEUE_HANDLE timer_queue);
TIMER_QUEUE_ENTRY* real_timer_queue_pop_due(TIMER_QUEUE_HANDLE timer_queue, uint64_t now_ms);

#endif // REAL_TIMER_QUEUE_LINUX_H
