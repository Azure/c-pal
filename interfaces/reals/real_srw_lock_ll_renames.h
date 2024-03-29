// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#define srw_lock_ll_init real_srw_lock_ll_init
#define srw_lock_ll_deinit real_srw_lock_ll_deinit
#define srw_lock_ll_acquire_exclusive real_srw_lock_ll_acquire_exclusive
#define srw_lock_ll_try_acquire_exclusive real_srw_lock_ll_try_acquire_exclusive
#define srw_lock_ll_release_exclusive real_srw_lock_ll_release_exclusive
#define srw_lock_ll_acquire_shared real_srw_lock_ll_acquire_shared
#define srw_lock_ll_try_acquire_shared real_srw_lock_ll_try_acquire_shared
#define srw_lock_ll_release_shared real_srw_lock_ll_release_shared

#define SRW_LOCK_LL_TRY_ACQUIRE_RESULT real_SRW_LOCK_LL_TRY_ACQUIRE_RESULT
