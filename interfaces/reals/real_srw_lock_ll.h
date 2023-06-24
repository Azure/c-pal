// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_SRW_LOCK_LL_H
#define REAL_SRW_LOCK_LL_H

#include "macro_utils/macro_utils.h"

#include "c_pal/srw_lock_ll.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_SRW_LOCK_LL_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2, \
        srw_lock_ll_init, \
        srw_lock_ll_deinit, \
        srw_lock_ll_acquire_exclusive, \
        srw_lock_ll_try_acquire_exclusive, \
        srw_lock_ll_release_exclusive, \
        srw_lock_ll_acquire_shared, \
        srw_lock_ll_try_acquire_shared, \
        srw_lock_ll_release_shared \
)

#ifdef __cplusplus
extern "C" {
#endif

int real_srw_lock_ll_init(SRW_LOCK_LL* srw_lock_ll);

void real_srw_lock_ll_deinit(SRW_LOCK_LL* srw_lock_ll);

void real_srw_lock_ll_acquire_exclusive(SRW_LOCK_LL* srw_lock_ll);

SRW_LOCK_LL_TRY_ACQUIRE_RESULT real_srw_lock_ll_try_acquire_exclusive(SRW_LOCK_LL* srw_lock_ll);

void real_srw_lock_ll_release_exclusive(SRW_LOCK_LL* srw_lock_ll);

void real_srw_lock_ll_acquire_shared(SRW_LOCK_LL* srw_lock_ll);

SRW_LOCK_LL_TRY_ACQUIRE_RESULT real_srw_lock_ll_try_acquire_shared(SRW_LOCK_LL* srw_lock_ll);

void real_srw_lock_ll_release_shared(SRW_LOCK_LL* srw_lock_ll);

#ifdef __cplusplus
}
#endif

#endif // REAL_SRW_LOCK_LL_H
