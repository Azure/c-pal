// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef TIMER_QUEUE_LINUX_H
#define TIMER_QUEUE_LINUX_H

#ifdef __cplusplus
#include <cstdbool>
#include <cstdint>
#else
#include <stdbool.h>
#include <stdint.h>
#endif

#include "c_pal/min_heap.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TIMER_QUEUE_NO_DUE_TIME UINT64_MAX

// placed inside the caller's timer structure, CONTAINING_RECORD gets the structure back
typedef struct TIMER_QUEUE_ENTRY_TAG
{
    MIN_HEAP_ENTRY heap_entry;
    uint32_t period_ms;
} TIMER_QUEUE_ENTRY;

typedef struct TIMER_QUEUE_TAG* TIMER_QUEUE_HANDLE;

MOCKABLE_FUNCTION(, TIMER_QUEUE_HANDLE, timer_queue_create);
MOCKABLE_FUNCTION(, void, timer_queue_destroy, TIMER_QUEUE_HANDLE, timer_queue);
MOCKABLE_FUNCTION(, int, timer_queue_reserve, TIMER_QUEUE_HANDLE, timer_queue);
MOCKABLE_FUNCTION(, void, timer_queue_unreserve, TIMER_QUEUE_HANDLE, timer_queue);
MOCKABLE_FUNCTION(, void, timer_queue_entry_init, TIMER_QUEUE_ENTRY*, entry);
MOCKABLE_FUNCTION(, bool, timer_queue_arm, TIMER_QUEUE_HANDLE, timer_queue, TIMER_QUEUE_ENTRY*, entry, uint64_t, due_time_ms, uint32_t, period_ms);
MOCKABLE_FUNCTION(, void, timer_queue_disarm, TIMER_QUEUE_HANDLE, timer_queue, TIMER_QUEUE_ENTRY*, entry);
MOCKABLE_FUNCTION(, uint64_t, timer_queue_get_next_due_time, TIMER_QUEUE_HANDLE, timer_queue);
MOCKABLE_FUNCTION(, TIMER_QUEUE_ENTRY*, timer_queue_pop_due, TIMER_QUEUE_HANDLE, timer_queue, uint64_t, now_ms);

#ifdef __cplusplus
}
#endif

#endif // TIMER_QUEUE_LINUX_H
