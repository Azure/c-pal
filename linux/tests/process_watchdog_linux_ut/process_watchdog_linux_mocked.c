// Copyright (c) Microsoft. All rights reserved.

// Define macros BEFORE including system headers to redirect API calls
#define timer_create mocked_timer_create
#define timer_settime mocked_timer_settime
#define timer_delete mocked_timer_delete

#include <signal.h>
#include <time.h>

// Timer mock prototypes from shared header
#include "mock_timer.h"

#include "../../src/process_watchdog_linux.c"
