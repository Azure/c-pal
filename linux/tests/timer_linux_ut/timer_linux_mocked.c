// Copyright (c) Microsoft. All rights reserved.

#include <time.h>

// No idea why iwyu warns about this since we include time.h but...
// IWYU pragma: no_forward_declare timespec

#define clock_gettime mocked_clock_gettime

int mocked_clock_gettime(clockid_t clockid, struct timespec *tp);

#include "../../src/timer_linux.c"
