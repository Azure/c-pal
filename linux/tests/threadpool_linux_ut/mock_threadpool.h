// Copyright (c) Microsoft. All rights reserved.

#ifndef MOCK_THREADPOOL_H
#define MOCK_THREADPOOL_H

#include <stdio.h>


#include "umock_c/umock_c_prod.h"

MOCKABLE_FUNCTION(, int, mocked_timer_create, clockid_t,clockid, struct sigevent*, sevp, timer_t *, timerid);
MOCKABLE_FUNCTION(, int, mocked_timer_settime, timer_t, timerid, int, flags, const struct itimerspec* , new_value, struct itimerspec* , old_value);

#endif //MOCK_THREADPOOL_H