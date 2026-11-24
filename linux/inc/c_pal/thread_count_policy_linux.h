// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef THREAD_COUNT_POLICY_LINUX_H
#define THREAD_COUNT_POLICY_LINUX_H

#ifdef __cplusplus
#include <cstdbool>
#include <cstdint>
#else
#include <stdbool.h>
#include <stdint.h>
#endif

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

#define THREAD_COUNT_POLICY_NEVER UINT64_MAX

typedef struct THREAD_COUNT_POLICY_TAG* THREAD_COUNT_POLICY_HANDLE;

MOCKABLE_FUNCTION(, THREAD_COUNT_POLICY_HANDLE, thread_count_policy_create, uint32_t, min_thread_count, uint32_t, max_thread_count);
MOCKABLE_FUNCTION(, void, thread_count_policy_destroy, THREAD_COUNT_POLICY_HANDLE, thread_count_policy);
MOCKABLE_FUNCTION(, bool, thread_count_policy_can_add_thread, THREAD_COUNT_POLICY_HANDLE, thread_count_policy, uint32_t, thread_count);
MOCKABLE_FUNCTION(, void, thread_count_policy_on_progress, THREAD_COUNT_POLICY_HANDLE, thread_count_policy, uint64_t, now_ms);
MOCKABLE_FUNCTION(, void, thread_count_policy_on_add_thread_failed, THREAD_COUNT_POLICY_HANDLE, thread_count_policy, uint64_t, now_ms);
MOCKABLE_FUNCTION(, uint64_t, thread_count_policy_get_add_thread_time, THREAD_COUNT_POLICY_HANDLE, thread_count_policy, uint32_t, queued_work_count, uint32_t, starting_thread_count, uint32_t, idle_thread_count, uint32_t, thread_count);
MOCKABLE_FUNCTION(, uint64_t, thread_count_policy_get_retire_thread_time, THREAD_COUNT_POLICY_HANDLE, thread_count_policy, uint32_t, thread_count, uint32_t, idle_thread_count, uint64_t, oldest_idle_since_ms, uint64_t, now_ms);

#ifdef __cplusplus
}
#endif

#endif // THREAD_COUNT_POLICY_LINUX_H
