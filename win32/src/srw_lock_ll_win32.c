// Copyright (C) Microsoft Corporation. All rights reserved.

#include "macro_utils/macro_utils.h"

#include "c_pal/srw_lock_ll.h"

int srw_lock_ll_init(SRW_LOCK_LL* srw_lock_ll)
{
    (void)srw_lock_ll;
    return MU_FAILURE;
}

void srw_lock_ll_deinit(SRW_LOCK_LL* srw_lock_ll)
{
    (void)srw_lock_ll;
}

void srw_lock_ll_acquire_exclusive(SRW_LOCK_LL* srw_lock_ll)
{
    (void)srw_lock_ll;
}

SRW_LOCK_LL_TRY_ACQUIRE_RESULT srw_lock_ll_try_acquire_exclusive(SRW_LOCK_LL* srw_lock_ll)
{
    (void)srw_lock_ll;
}

void srw_lock_ll_release_exclusive(SRW_LOCK_LL* srw_lock_ll)
{
    (void)srw_lock_ll;
}

void srw_lock_ll_acquire_shared(SRW_LOCK_LL* srw_lock_ll)
{
    (void)srw_lock_ll;
}

SRW_LOCK_LL_TRY_ACQUIRE_RESULT srw_lock_ll_try_acquire_shared(SRW_LOCK_LL* srw_lock_ll)
{
    (void)srw_lock_ll;
}

void srw_lock_ll_release_shared(SRW_LOCK_LL* srw_lock_ll)
{
    (void)srw_lock_ll;
}
