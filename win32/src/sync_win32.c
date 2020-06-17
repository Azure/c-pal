// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "windows.h"
#include "sync.h"

IMPLEMENT_MOCKABLE_FUNCTION(, bool, wait_on_address, volatile_atomic int32_t*, address, int32_t*, compare_address, uint32_t, timeout_ms)
{
    (void) address;
    (void) compare_address;
    (void) timeout_ms;
    return false;
}
IMPLEMENT_MOCKABLE_FUNCTION(, void, wake_by_address_all, void*, address)
{
    (void) address;
}
IMPLEMENT_MOCKABLE_FUNCTION(, void, wake_by_address_single, void*, address)
{
    (void) address;
}
