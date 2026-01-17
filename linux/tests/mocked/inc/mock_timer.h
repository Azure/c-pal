// Copyright (c) Microsoft. All rights reserved.

// This header provides mocks for POSIX timer APIs.
// Include this header BEFORE including the source file that uses these APIs.

#ifndef MOCK_TIMER_H
#define MOCK_TIMER_H

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations for POSIX timer types
typedef int clockid_t;
typedef void* timer_t;
struct sigevent;
struct itimerspec;

#define timer_create mocked_timer_create
#define timer_settime mocked_timer_settime
#define timer_delete mocked_timer_delete

MOCKABLE_FUNCTION(, int, mocked_timer_create, clockid_t, clockid, struct sigevent*, sevp, timer_t*, timerid);
MOCKABLE_FUNCTION(, int, mocked_timer_settime, timer_t, timerid, int, flags, const struct itimerspec*, new_value, struct itimerspec*, old_value);
MOCKABLE_FUNCTION(, int, mocked_timer_delete, timer_t, timerid);

#ifdef __cplusplus
}
#endif

#endif // MOCK_TIMER_H
