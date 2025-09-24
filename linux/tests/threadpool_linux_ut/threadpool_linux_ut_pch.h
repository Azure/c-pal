// Copyright (c) Microsoft. All rights reserved.


// Precompiled header for threadpool_linux_ut

#include <inttypes.h>
#include <stdlib.h>
#include <semaphore.h>
#include <time.h>
#include <bits/types/sigevent_t.h>         // for sigevent, sigev_notify_fun...
#include <bits/types/sigval_t.h>           // for sigval_t
#include <bits/types/timer_t.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umock_c_negative_tests.h"

#include "real_gballoc_ll.h"

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/threadapi.h"
#include "c_pal/tqueue.h"

#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/lazy_init.h"
#include "c_pal/sync.h"
#include "c_pal/execution_engine.h"
#include "c_pal/execution_engine_linux.h"

#include "c_pal/tqueue_threadpool_work_item.h"

#undef ENABLE_MOCKS

#include "c_pal/thandle.h" // IWYU pragma: keep
#include "c_pal/thandle_ll.h"

#include "real_interlocked.h"
#include "real_gballoc_hl.h"
#include "real_tqueue_threadpool_work_item.h"
#include "real_lazy_init.h"
#include "real_interlocked_hl.h"

#include "c_pal/threadpool.h"

#define DEFAULT_TASK_ARRAY_SIZE 2048
#define MIN_THREAD_COUNT 5
#define MAX_THREAD_COUNT 10
#define MAX_THREADPOOL_TIMER_COUNT 64
#define MAX_TIMERS 1024