// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/containing_record.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/min_heap.h"

#include "c_pal/timer_queue_linux.h"

typedef struct TIMER_QUEUE_TAG
{
    MIN_HEAP_HANDLE armed_timers;
} TIMER_QUEUE;

TIMER_QUEUE_HANDLE timer_queue_create(void)
{
    /* Codes_SRS_TIMER_QUEUE_LINUX_42_001: [ timer_queue_create shall allocate a new timer queue by calling malloc. ]*/
    TIMER_QUEUE* result = malloc(sizeof(TIMER_QUEUE));
    if (result == NULL)
    {
        /* Codes_SRS_TIMER_QUEUE_LINUX_42_004: [ If any error occurs, timer_queue_create shall free the resources it allocated, fail and return NULL. ]*/
        LogError("malloc(sizeof(TIMER_QUEUE)=%zu) failed", sizeof(TIMER_QUEUE));
    }
    else
    {
        /* Codes_SRS_TIMER_QUEUE_LINUX_42_002: [ timer_queue_create shall create the heap of armed timers by calling min_heap_create. ]*/
        result->armed_timers = min_heap_create();
        if (result->armed_timers == NULL)
        {
            /* Codes_SRS_TIMER_QUEUE_LINUX_42_004: [ If any error occurs, timer_queue_create shall free the resources it allocated, fail and return NULL. ]*/
            LogError("min_heap_create failed");
            free(result);
            result = NULL;
        }
        else
        {
            /* Codes_SRS_TIMER_QUEUE_LINUX_42_003: [ timer_queue_create shall succeed and return the timer queue. ]*/
        }
    }

    return result;
}

void timer_queue_destroy(TIMER_QUEUE_HANDLE timer_queue)
{
    if (timer_queue == NULL)
    {
        /* Codes_SRS_TIMER_QUEUE_LINUX_42_005: [ If timer_queue is NULL, timer_queue_destroy shall return. ]*/
        LogError("Invalid arguments: TIMER_QUEUE_HANDLE timer_queue=%p", timer_queue);
    }
    else
    {
        /* Codes_SRS_TIMER_QUEUE_LINUX_42_006: [ timer_queue_destroy shall destroy the heap of armed timers by calling min_heap_destroy. ]*/
        min_heap_destroy(timer_queue->armed_timers);

        /* Codes_SRS_TIMER_QUEUE_LINUX_42_007: [ timer_queue_destroy shall free timer_queue. ]*/
        free(timer_queue);
    }
}

int timer_queue_reserve(TIMER_QUEUE_HANDLE timer_queue)
{
    int result;

    if (timer_queue == NULL)
    {
        /* Codes_SRS_TIMER_QUEUE_LINUX_42_008: [ If timer_queue is NULL, timer_queue_reserve shall fail and return a non-zero value. ]*/
        LogError("Invalid arguments: TIMER_QUEUE_HANDLE timer_queue=%p", timer_queue);
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_TIMER_QUEUE_LINUX_42_009: [ timer_queue_reserve shall reserve room in the heap of armed timers by calling min_heap_reserve. ]*/
        if (min_heap_reserve(timer_queue->armed_timers) != 0)
        {
            /* Codes_SRS_TIMER_QUEUE_LINUX_42_010: [ If min_heap_reserve fails, timer_queue_reserve shall fail and return a non-zero value. ]*/
            LogError("min_heap_reserve failed");
            result = MU_FAILURE;
        }
        else
        {
            /* Codes_SRS_TIMER_QUEUE_LINUX_42_011: [ timer_queue_reserve shall succeed and return 0. ]*/
            result = 0;
        }
    }

    return result;
}

void timer_queue_unreserve(TIMER_QUEUE_HANDLE timer_queue)
{
    if (timer_queue == NULL)
    {
        /* Codes_SRS_TIMER_QUEUE_LINUX_42_012: [ If timer_queue is NULL, timer_queue_unreserve shall return. ]*/
        LogError("Invalid arguments: TIMER_QUEUE_HANDLE timer_queue=%p", timer_queue);
    }
    else
    {
        /* Codes_SRS_TIMER_QUEUE_LINUX_42_013: [ timer_queue_unreserve shall give back the room by calling min_heap_unreserve. ]*/
        min_heap_unreserve(timer_queue->armed_timers);
    }
}

void timer_queue_entry_init(TIMER_QUEUE_ENTRY* entry)
{
    if (entry == NULL)
    {
        /* Codes_SRS_TIMER_QUEUE_LINUX_42_014: [ If entry is NULL, timer_queue_entry_init shall return. ]*/
        LogError("Invalid arguments: TIMER_QUEUE_ENTRY* entry=%p", entry);
    }
    else
    {
        /* Codes_SRS_TIMER_QUEUE_LINUX_42_015: [ timer_queue_entry_init shall initialize the heap entry of entry by calling min_heap_entry_init. ]*/
        min_heap_entry_init(&entry->heap_entry);

        /* Codes_SRS_TIMER_QUEUE_LINUX_42_016: [ timer_queue_entry_init shall set the period of entry to 0. ]*/
        entry->period_ms = 0;
    }
}

bool timer_queue_arm(TIMER_QUEUE_HANDLE timer_queue, TIMER_QUEUE_ENTRY* entry, uint64_t due_time_ms, uint32_t period_ms)
{
    bool result;

    if (
        /* Codes_SRS_TIMER_QUEUE_LINUX_42_017: [ If timer_queue is NULL, timer_queue_arm shall return false. ]*/
        (timer_queue == NULL) ||
        /* Codes_SRS_TIMER_QUEUE_LINUX_42_018: [ If entry is NULL, timer_queue_arm shall return false. ]*/
        (entry == NULL)
        )
    {
        LogError("Invalid arguments: TIMER_QUEUE_HANDLE timer_queue=%p, TIMER_QUEUE_ENTRY* entry=%p, uint64_t due_time_ms=%" PRIu64 ", uint32_t period_ms=%" PRIu32 "",
            timer_queue, entry, due_time_ms, period_ms);
        result = false;
    }
    else
    {
        /* Codes_SRS_TIMER_QUEUE_LINUX_42_019: [ timer_queue_arm shall remove entry from the heap of armed timers by calling min_heap_remove. ]*/
        min_heap_remove(timer_queue->armed_timers, &entry->heap_entry);

        /* Codes_SRS_TIMER_QUEUE_LINUX_42_020: [ timer_queue_arm shall set the period of entry to period_ms. ]*/
        entry->period_ms = period_ms;

        /* Codes_SRS_TIMER_QUEUE_LINUX_42_021: [ timer_queue_arm shall insert entry in the heap of armed timers with due_time_ms as key by calling min_heap_insert. ]*/
        min_heap_insert(timer_queue->armed_timers, &entry->heap_entry, due_time_ms);

        /* Codes_SRS_TIMER_QUEUE_LINUX_42_022: [ timer_queue_arm shall return whether entry is the first timer of the heap of armed timers obtained by calling min_heap_peek. ]*/
        result = (min_heap_peek(timer_queue->armed_timers) == &entry->heap_entry);
    }

    return result;
}

void timer_queue_disarm(TIMER_QUEUE_HANDLE timer_queue, TIMER_QUEUE_ENTRY* entry)
{
    if (
        /* Codes_SRS_TIMER_QUEUE_LINUX_42_023: [ If timer_queue is NULL, timer_queue_disarm shall return. ]*/
        (timer_queue == NULL) ||
        /* Codes_SRS_TIMER_QUEUE_LINUX_42_024: [ If entry is NULL, timer_queue_disarm shall return. ]*/
        (entry == NULL)
        )
    {
        LogError("Invalid arguments: TIMER_QUEUE_HANDLE timer_queue=%p, TIMER_QUEUE_ENTRY* entry=%p", timer_queue, entry);
    }
    else
    {
        /* Codes_SRS_TIMER_QUEUE_LINUX_42_025: [ timer_queue_disarm shall remove entry from the heap of armed timers by calling min_heap_remove. ]*/
        min_heap_remove(timer_queue->armed_timers, &entry->heap_entry);
    }
}

uint64_t timer_queue_get_next_due_time(TIMER_QUEUE_HANDLE timer_queue)
{
    uint64_t result;

    if (timer_queue == NULL)
    {
        /* Codes_SRS_TIMER_QUEUE_LINUX_42_026: [ If timer_queue is NULL, timer_queue_get_next_due_time shall return TIMER_QUEUE_NO_DUE_TIME. ]*/
        LogError("Invalid arguments: TIMER_QUEUE_HANDLE timer_queue=%p", timer_queue);
        result = TIMER_QUEUE_NO_DUE_TIME;
    }
    else
    {
        /* Codes_SRS_TIMER_QUEUE_LINUX_42_027: [ timer_queue_get_next_due_time shall get the first timer by calling min_heap_peek. ]*/
        MIN_HEAP_ENTRY* first_timer = min_heap_peek(timer_queue->armed_timers);
        if (first_timer == NULL)
        {
            /* Codes_SRS_TIMER_QUEUE_LINUX_42_028: [ If no timer is armed, timer_queue_get_next_due_time shall return TIMER_QUEUE_NO_DUE_TIME. ]*/
            result = TIMER_QUEUE_NO_DUE_TIME;
        }
        else
        {
            /* Codes_SRS_TIMER_QUEUE_LINUX_42_029: [ timer_queue_get_next_due_time shall return the due time of the first timer. ]*/
            result = first_timer->key;
        }
    }

    return result;
}

TIMER_QUEUE_ENTRY* timer_queue_pop_due(TIMER_QUEUE_HANDLE timer_queue, uint64_t now_ms)
{
    TIMER_QUEUE_ENTRY* result;

    if (timer_queue == NULL)
    {
        /* Codes_SRS_TIMER_QUEUE_LINUX_42_030: [ If timer_queue is NULL, timer_queue_pop_due shall return NULL. ]*/
        LogError("Invalid arguments: TIMER_QUEUE_HANDLE timer_queue=%p, uint64_t now_ms=%" PRIu64 "", timer_queue, now_ms);
        result = NULL;
    }
    else
    {
        /* Codes_SRS_TIMER_QUEUE_LINUX_42_031: [ timer_queue_pop_due shall get the first timer by calling min_heap_peek. ]*/
        MIN_HEAP_ENTRY* first_timer = min_heap_peek(timer_queue->armed_timers);
        if (
            /* Codes_SRS_TIMER_QUEUE_LINUX_42_032: [ If no timer is armed, timer_queue_pop_due shall return NULL. ]*/
            (first_timer == NULL) ||
            /* Codes_SRS_TIMER_QUEUE_LINUX_42_033: [ If the due time of the first timer is later than now_ms, timer_queue_pop_due shall return NULL. ]*/
            (first_timer->key > now_ms)
            )
        {
            result = NULL;
        }
        else
        {
            uint64_t due_time_ms = first_timer->key;
            result = CONTAINING_RECORD(first_timer, TIMER_QUEUE_ENTRY, heap_entry);

            /* Codes_SRS_TIMER_QUEUE_LINUX_42_034: [ timer_queue_pop_due shall remove the first timer from the heap of armed timers by calling min_heap_remove. ]*/
            min_heap_remove(timer_queue->armed_timers, first_timer);

            if (result->period_ms == 0)
            {
                /* do nothing */
            }
            else
            {
                /* Codes_SRS_TIMER_QUEUE_LINUX_42_035: [ If the period of the timer is not 0: ]*/

                /* Codes_SRS_TIMER_QUEUE_LINUX_42_036: [ timer_queue_pop_due shall compute the next due time as the due time plus the period. ]*/
                uint64_t next_due_time_ms = due_time_ms + result->period_ms;

                if (next_due_time_ms > now_ms)
                {
                    /* do nothing */
                }
                else
                {
                    /* Codes_SRS_TIMER_QUEUE_LINUX_42_037: [ If the next due time is not later than now_ms, timer_queue_pop_due shall use now_ms plus the period as the next due time. ]*/
                    next_due_time_ms = now_ms + result->period_ms;
                }

                /* Codes_SRS_TIMER_QUEUE_LINUX_42_038: [ timer_queue_pop_due shall insert the timer in the heap of armed timers with the next due time as key by calling min_heap_insert. ]*/
                min_heap_insert(timer_queue->armed_timers, first_timer, next_due_time_ms);
            }

            /* Codes_SRS_TIMER_QUEUE_LINUX_42_039: [ timer_queue_pop_due shall return the timer. ]*/
        }
    }

    return result;
}
