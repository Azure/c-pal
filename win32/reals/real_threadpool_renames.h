// Copyright (c) Microsoft. All rights reserved.

#define threadpool_create               real_threadpool_create
#define threadpool_destroy              real_threadpool_destroy
#define threadpool_open_async           real_threadpool_open_async
#define threadpool_close                real_threadpool_close
#define threadpool_schedule_work        real_threadpool_schedule_work
#define threadpool_timer_start          real_threadpool_timer_start
#define threadpool_timer_restart        real_threadpool_timer_restart
#define threadpool_timer_cancel         real_threadpool_timer_cancel
#define threadpool_timer_destroy        real_threadpool_timer_destroy
