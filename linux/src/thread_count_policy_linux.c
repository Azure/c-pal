// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/thread_count_policy_linux.h"

#define THREAD_COUNT_POLICY_DEFAULT_MAX_THREAD_COUNT 500
#define STARVATION_THRESHOLD_MS 50
#define ADD_THREAD_RETRY_DELAY_MS 100
#define IDLE_THREAD_TIMEOUT_MS 60000

typedef struct THREAD_COUNT_POLICY_TAG
{
    uint32_t min_thread_count;
    uint32_t max_thread_count;
    uint64_t last_progress_ms;
    uint64_t earliest_add_thread_ms;
} THREAD_COUNT_POLICY;

THREAD_COUNT_POLICY_HANDLE thread_count_policy_create(uint32_t min_thread_count, uint32_t max_thread_count)
{
    THREAD_COUNT_POLICY* result;

    if ((max_thread_count != 0) && (max_thread_count < min_thread_count))
    {
        /* Codes_SRS_THREAD_COUNT_POLICY_LINUX_42_001: [ If max_thread_count is not 0 and is less than min_thread_count, thread_count_policy_create shall fail and return NULL. ]*/
        LogError("Invalid arguments: uint32_t min_thread_count=%" PRIu32 ", uint32_t max_thread_count=%" PRIu32 "", min_thread_count, max_thread_count);
        result = NULL;
    }
    else
    {
        /* Codes_SRS_THREAD_COUNT_POLICY_LINUX_42_002: [ thread_count_policy_create shall allocate a new policy. ]*/
        result = malloc(sizeof(THREAD_COUNT_POLICY));
        if (result == NULL)
        {
            /* Codes_SRS_THREAD_COUNT_POLICY_LINUX_42_009: [ If any error occurs, thread_count_policy_create shall fail and return NULL. ]*/
            LogError("malloc(sizeof(THREAD_COUNT_POLICY)=%zu) failed", sizeof(THREAD_COUNT_POLICY));
        }
        else
        {
            /* Codes_SRS_THREAD_COUNT_POLICY_LINUX_42_003: [ thread_count_policy_create shall store min_thread_count as the minimum thread count. ]*/
            result->min_thread_count = min_thread_count;

            if (max_thread_count == 0)
            {
                /* Codes_SRS_THREAD_COUNT_POLICY_LINUX_42_004: [ If max_thread_count is 0, thread_count_policy_create shall use 500 as the maximum thread count, or min_thread_count if it is larger. ]*/
                result->max_thread_count = (min_thread_count > THREAD_COUNT_POLICY_DEFAULT_MAX_THREAD_COUNT) ? min_thread_count : THREAD_COUNT_POLICY_DEFAULT_MAX_THREAD_COUNT;
            }
            else
            {
                /* Codes_SRS_THREAD_COUNT_POLICY_LINUX_42_005: [ If max_thread_count is not 0, thread_count_policy_create shall use max_thread_count as the maximum thread count. ]*/
                result->max_thread_count = max_thread_count;
            }

            /* Codes_SRS_THREAD_COUNT_POLICY_LINUX_42_006: [ thread_count_policy_create shall set the last progress time to 0. ]*/
            result->last_progress_ms = 0;

            /* Codes_SRS_THREAD_COUNT_POLICY_LINUX_42_007: [ thread_count_policy_create shall set the earliest time to add a thread to 0. ]*/
            result->earliest_add_thread_ms = 0;

            /* Codes_SRS_THREAD_COUNT_POLICY_LINUX_42_008: [ thread_count_policy_create shall succeed and return the policy. ]*/
        }
    }

    return result;
}

void thread_count_policy_destroy(THREAD_COUNT_POLICY_HANDLE thread_count_policy)
{
    if (thread_count_policy == NULL)
    {
        /* Codes_SRS_THREAD_COUNT_POLICY_LINUX_42_010: [ If thread_count_policy is NULL, thread_count_policy_destroy shall return. ]*/
        LogError("Invalid arguments: THREAD_COUNT_POLICY_HANDLE thread_count_policy=%p", thread_count_policy);
    }
    else
    {
        /* Codes_SRS_THREAD_COUNT_POLICY_LINUX_42_011: [ thread_count_policy_destroy shall free thread_count_policy. ]*/
        free(thread_count_policy);
    }
}

bool thread_count_policy_can_add_thread(THREAD_COUNT_POLICY_HANDLE thread_count_policy, uint32_t thread_count)
{
    bool result;

    if (thread_count_policy == NULL)
    {
        /* Codes_SRS_THREAD_COUNT_POLICY_LINUX_42_012: [ If thread_count_policy is NULL, thread_count_policy_can_add_thread shall return false. ]*/
        LogError("Invalid arguments: THREAD_COUNT_POLICY_HANDLE thread_count_policy=%p, uint32_t thread_count=%" PRIu32 "", thread_count_policy, thread_count);
        result = false;
    }
    else
    {
        /* Codes_SRS_THREAD_COUNT_POLICY_LINUX_42_013: [ thread_count_policy_can_add_thread shall return whether thread_count is less than the maximum thread count. ]*/
        result = (thread_count < thread_count_policy->max_thread_count);
    }

    return result;
}

void thread_count_policy_on_progress(THREAD_COUNT_POLICY_HANDLE thread_count_policy, uint64_t now_ms)
{
    if (thread_count_policy == NULL)
    {
        /* Codes_SRS_THREAD_COUNT_POLICY_LINUX_42_014: [ If thread_count_policy is NULL, thread_count_policy_on_progress shall return. ]*/
        LogError("Invalid arguments: THREAD_COUNT_POLICY_HANDLE thread_count_policy=%p, uint64_t now_ms=%" PRIu64 "", thread_count_policy, now_ms);
    }
    else
    {
        if (now_ms <= thread_count_policy->last_progress_ms)
        {
            /* do nothing */
        }
        else
        {
            /* Codes_SRS_THREAD_COUNT_POLICY_LINUX_42_015: [ If now_ms is later than the last progress time, thread_count_policy_on_progress shall set the last progress time to now_ms. ]*/
            thread_count_policy->last_progress_ms = now_ms;
        }
    }
}

void thread_count_policy_on_add_thread_failed(THREAD_COUNT_POLICY_HANDLE thread_count_policy, uint64_t now_ms)
{
    if (thread_count_policy == NULL)
    {
        /* Codes_SRS_THREAD_COUNT_POLICY_LINUX_42_016: [ If thread_count_policy is NULL, thread_count_policy_on_add_thread_failed shall return. ]*/
        LogError("Invalid arguments: THREAD_COUNT_POLICY_HANDLE thread_count_policy=%p, uint64_t now_ms=%" PRIu64 "", thread_count_policy, now_ms);
    }
    else
    {
        /* Codes_SRS_THREAD_COUNT_POLICY_LINUX_42_017: [ thread_count_policy_on_add_thread_failed shall set the earliest time to add a thread to now_ms plus 100. ]*/
        thread_count_policy->earliest_add_thread_ms = now_ms + ADD_THREAD_RETRY_DELAY_MS;
    }
}

uint64_t thread_count_policy_get_add_thread_time(THREAD_COUNT_POLICY_HANDLE thread_count_policy, uint32_t queued_work_count, uint32_t starting_thread_count, uint32_t idle_thread_count, uint32_t thread_count)
{
    uint64_t result;

    if (thread_count_policy == NULL)
    {
        /* Codes_SRS_THREAD_COUNT_POLICY_LINUX_42_018: [ If thread_count_policy is NULL, thread_count_policy_get_add_thread_time shall return THREAD_COUNT_POLICY_NEVER. ]*/
        LogError("Invalid arguments: THREAD_COUNT_POLICY_HANDLE thread_count_policy=%p, uint32_t queued_work_count=%" PRIu32 ", uint32_t starting_thread_count=%" PRIu32 ", uint32_t idle_thread_count=%" PRIu32 ", uint32_t thread_count=%" PRIu32 "",
            thread_count_policy, queued_work_count, starting_thread_count, idle_thread_count, thread_count);
        result = THREAD_COUNT_POLICY_NEVER;
    }
    else
    {
        if (
            /* Codes_SRS_THREAD_COUNT_POLICY_LINUX_42_019: [ If queued_work_count is not greater than starting_thread_count, thread_count_policy_get_add_thread_time shall return THREAD_COUNT_POLICY_NEVER. ]*/
            (queued_work_count <= starting_thread_count) ||
            /* Codes_SRS_THREAD_COUNT_POLICY_LINUX_42_020: [ If idle_thread_count is not 0, thread_count_policy_get_add_thread_time shall return THREAD_COUNT_POLICY_NEVER. ]*/
            (idle_thread_count != 0) ||
            /* Codes_SRS_THREAD_COUNT_POLICY_LINUX_42_021: [ If thread_count is not less than the maximum thread count, thread_count_policy_get_add_thread_time shall return THREAD_COUNT_POLICY_NEVER. ]*/
            (thread_count >= thread_count_policy->max_thread_count)
            )
        {
            result = THREAD_COUNT_POLICY_NEVER;
        }
        else
        {
            if (thread_count == 0)
            {
                /* Codes_SRS_THREAD_COUNT_POLICY_LINUX_42_027: [ If thread_count is 0, thread_count_policy_get_add_thread_time shall return the earliest time to add a thread. ]*/
                // no worker thread can make progress
                result = thread_count_policy->earliest_add_thread_ms;
            }
            else
            {
                /* Codes_SRS_THREAD_COUNT_POLICY_LINUX_42_022: [ Otherwise, thread_count_policy_get_add_thread_time shall return the later of the last progress time plus 50 and the earliest time to add a thread. ]*/
                uint64_t starvation_time = thread_count_policy->last_progress_ms + STARVATION_THRESHOLD_MS;
                result = (starvation_time > thread_count_policy->earliest_add_thread_ms) ? starvation_time : thread_count_policy->earliest_add_thread_ms;
            }
        }
    }

    return result;
}

uint64_t thread_count_policy_get_retire_thread_time(THREAD_COUNT_POLICY_HANDLE thread_count_policy, uint32_t thread_count, uint32_t idle_thread_count, uint64_t oldest_idle_since_ms, uint64_t now_ms)
{
    uint64_t result;

    if (thread_count_policy == NULL)
    {
        /* Codes_SRS_THREAD_COUNT_POLICY_LINUX_42_023: [ If thread_count_policy is NULL, thread_count_policy_get_retire_thread_time shall return THREAD_COUNT_POLICY_NEVER. ]*/
        LogError("Invalid arguments: THREAD_COUNT_POLICY_HANDLE thread_count_policy=%p, uint32_t thread_count=%" PRIu32 ", uint32_t idle_thread_count=%" PRIu32 ", uint64_t oldest_idle_since_ms=%" PRIu64 ", uint64_t now_ms=%" PRIu64 "",
            thread_count_policy, thread_count, idle_thread_count, oldest_idle_since_ms, now_ms);
        result = THREAD_COUNT_POLICY_NEVER;
    }
    else
    {
        if (thread_count <= thread_count_policy->min_thread_count)
        {
            /* Codes_SRS_THREAD_COUNT_POLICY_LINUX_42_024: [ If thread_count is not greater than the minimum thread count, thread_count_policy_get_retire_thread_time shall return THREAD_COUNT_POLICY_NEVER. ]*/
            result = THREAD_COUNT_POLICY_NEVER;
        }
        else
        {
            if (idle_thread_count == 0)
            {
                /* Codes_SRS_THREAD_COUNT_POLICY_LINUX_42_025: [ If idle_thread_count is 0, thread_count_policy_get_retire_thread_time shall return now_ms plus 60000. ]*/
                // worker threads going idle do not notify the caller, so it checks again within the idle timeout
                result = now_ms + IDLE_THREAD_TIMEOUT_MS;
            }
            else
            {
                /* Codes_SRS_THREAD_COUNT_POLICY_LINUX_42_026: [ Otherwise, thread_count_policy_get_retire_thread_time shall return oldest_idle_since_ms plus 60000. ]*/
                result = oldest_idle_since_ms + IDLE_THREAD_TIMEOUT_MS;
            }
        }
    }

    return result;
}
