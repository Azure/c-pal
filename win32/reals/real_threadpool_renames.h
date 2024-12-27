// Copyright (c) Microsoft. All rights reserved.

#define THREADPOOL                      real_THREADPOOL
#define THREADPOOL_WORK_ITEM            real_THREADPOOL_WORK_ITEM

#define threadpool_create               real_threadpool_create
#define threadpool_open                 real_threadpool_open
#define threadpool_close                real_threadpool_close
#define threadpool_create_work_item     real_threadpool_create_work_item
#define threadpool_schedule_work_item   real_threadpool_schedule_work_item
#define threadpool_schedule_work        real_threadpool_schedule_work
#define threadpool_timer_start          real_threadpool_timer_start
#define threadpool_timer_restart        real_threadpool_timer_restart
#define threadpool_timer_cancel         real_threadpool_timer_cancel

#define THREADPOOL_WIN32_STATE          real_THREADPOOL_WIN32_STATE
