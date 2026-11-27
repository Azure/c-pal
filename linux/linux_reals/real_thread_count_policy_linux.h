// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef REAL_THREAD_COUNT_POLICY_LINUX_H
#define REAL_THREAD_COUNT_POLICY_LINUX_H

#include <stdbool.h>
#include <stdint.h>

#include "macro_utils/macro_utils.h"

#include "c_pal/thread_count_policy_linux.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_THREAD_COUNT_POLICY_LINUX_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2, \
        thread_count_policy_create, \
        thread_count_policy_destroy, \
        thread_count_policy_can_add_thread, \
        thread_count_policy_on_progress, \
        thread_count_policy_on_add_thread_failed, \
        thread_count_policy_get_add_thread_time, \
        thread_count_policy_get_retire_thread_time \
    )

THREAD_COUNT_POLICY_HANDLE real_thread_count_policy_create(uint32_t min_thread_count, uint32_t max_thread_count);
void real_thread_count_policy_destroy(THREAD_COUNT_POLICY_HANDLE thread_count_policy);
bool real_thread_count_policy_can_add_thread(THREAD_COUNT_POLICY_HANDLE thread_count_policy, uint32_t thread_count);
void real_thread_count_policy_on_progress(THREAD_COUNT_POLICY_HANDLE thread_count_policy, uint64_t now_ms);
void real_thread_count_policy_on_add_thread_failed(THREAD_COUNT_POLICY_HANDLE thread_count_policy, uint64_t now_ms);
uint64_t real_thread_count_policy_get_add_thread_time(THREAD_COUNT_POLICY_HANDLE thread_count_policy, uint32_t queued_work_count, uint32_t starting_thread_count, uint32_t idle_thread_count, uint32_t thread_count);
uint64_t real_thread_count_policy_get_retire_thread_time(THREAD_COUNT_POLICY_HANDLE thread_count_policy, uint32_t thread_count, uint32_t idle_thread_count, uint64_t oldest_idle_since_ms, uint64_t now_ms);

#endif // REAL_THREAD_COUNT_POLICY_LINUX_H
