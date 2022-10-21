// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef THREADPOOL_UTILS_H
#define THREADPOOL_UTILS_H

#ifdef __cplusplus
#include <cstdarg>
#include <cinttypes>
#include <cstdio>
#else
#include <stddef.h>
#include <stdarg.h>
#include <inttypes.h>
#include <stdio.h>
#endif

#include "macro_utils/macro_utils.h"

#include "umock_c/umock_c_prod.h"

typedef struct THREADPOOL_TAG* THREADPOOL_HANDLE;

typedef void(*THREADPOOL_TASK_FUNC)(void *);

MOCKABLE_FUNCTION(, THREADPOOL_HANDLE, threadpool_create, uint32_t, thread_count);
MOCKABLE_FUNCTION(, void, threadpool_destroy, THREADPOOL_HANDLE, thread_handle);
MOCKABLE_FUNCTION(, int, threadpool_add_task, THREADPOOL_HANDLE, thread_handle, THREADPOOL_TASK_FUNC, task_func, void*, task_arg);


#endif
