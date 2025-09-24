// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for reals_win32_ut

#include "testrunnerswitcher.h"

#include "c_pal/threadapi.h"
#include "c_pal/srw_lock.h"
#include "c_pal/srw_lock_ll.h"
#include "c_pal/string_utils.h"
#include "c_pal/timer.h"
#include "c_pal/interlocked.h"
#include "c_pal/gballoc_ll.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/call_once.h"
#include "c_pal/lazy_init.h"
#include "c_pal/sync.h"
#include "c_pal/arithmetic.h"
#include "c_pal/uuid.h"
#include "c_pal/execution_engine.h"
#include "c_pal/thandle_log_context_handle.h"
#include "c_pal/socket_transport.h"
#include "c_pal/async_socket.h"
#include "c_pal/job_object_helper.h"

#include "macro_utils/macro_utils.h"

#ifdef REGISTER_GLOBAL_MOCK_HOOK
#undef REGISTER_GLOBAL_MOCK_HOOK
#endif

#define REGISTER_GLOBAL_MOCK_HOOK(original, real) \
    (original == real) ? (void)0 : (void)1;

#include "real_threadapi.h"
#include "real_srw_lock.h"
#include "real_srw_lock_ll.h"
#include "real_string_utils.h"
#include "real_timer.h"
#include "real_interlocked.h"
#include "real_gballoc_ll.h"
#include "real_gballoc_hl.h"
#include "real_call_once.h"
#include "real_lazy_init.h"
#include "real_sync.h"
#include "real_arithmetic.h"
#include "real_uuid.h"
#include "real_execution_engine.h"
#include "real_thandle_log_context_handle.h"
#include "real_socket_transport.h"
#include "real_async_socket.h"
#include "real_job_object_helper.h"