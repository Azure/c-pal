// Copyright (c) Microsoft. All rights reserved.

#include "windows.h"

// Timer mock prototypes from shared header
#include "../mocked/inc/mock_threadpool_timer.h"

// Include real interlocked implementation directly (not mocked)
#include "../../../c_pal_ll/win32/src/interlocked_win32.c"

#include "../../src/process_watchdog_win32.c"
