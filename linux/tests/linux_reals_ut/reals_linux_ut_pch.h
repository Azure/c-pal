// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for reals_linux_ut

#include "testrunnerswitcher.h"

#include "c_pal/arithmetic.h" // IWYU pragma: keep
#include "c_pal/srw_lock.h" // IWYU pragma: keep
#include "c_pal/string_utils.h" // IWYU pragma: keep
#include "c_pal/threadpool.h" // IWYU pragma: keep
#include "c_pal/uuid.h" // IWYU pragma: keep
#include "c_pal/execution_engine.h" // IWYU pragma: keep
#include "c_pal/interlocked.h" // IWYU pragma: keep
#include "c_pal/thandle_log_context_handle.h" // IWYU pragma: keep
#include "c_pal/socket_transport.h" // IWYU pragma: keep
#include "c_pal/async_socket.h" // IWYU pragma: keep

#define REGISTER_GLOBAL_MOCK_HOOK(original, real) \
    (original == real) ? (void)0 : (void)1;

#include "real_arithmetic.h"
#include "real_srw_lock.h"
#include "real_string_utils.h"
#include "real_sync.h"
#include "real_threadapi.h"
#include "real_threadpool.h"
#include "real_uuid.h"
#include "real_execution_engine.h"
#include "real_interlocked.h"
#include "real_thandle_log_context_handle.h"
#include "real_socket_transport.h"
#include "real_async_socket.h"