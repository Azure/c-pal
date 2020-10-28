// Copyright (c) Microsoft. All rights reserved.

#include <time.h>


#define clock_gettime mocked_clock_gettime

int mocked_clock_gettime(clockid_t clockid, struct timespec *tp);

#include "../../src/timer_linux.c"
