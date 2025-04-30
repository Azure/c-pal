// Copyright (C) Microsoft Corporation. All rights reserved.

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/thandle.h"
#include "c_pal/tqueue.h"

#include "c_pal/tqueue_threadpool_work_item.h"

THANDLE_TYPE_DEFINE(TQUEUE_TYPEDEF_NAME(THANDLE(THREADPOOL_WORK_ITEM)));
TQUEUE_TYPE_DEFINE(THANDLE(THREADPOOL_WORK_ITEM));
