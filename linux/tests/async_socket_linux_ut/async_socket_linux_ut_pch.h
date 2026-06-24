// Copyright (c) Microsoft. All rights reserved.

// Precompiled header for async_socket_linux_ut

#ifndef ASYNC_SOCKET_LINUX_UT_PCH_H
#define ASYNC_SOCKET_LINUX_UT_PCH_H

#include <stdlib.h>
#include <inttypes.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/types.h>                       // for ssize_t

#include "macro_utils/macro_utils.h"  // IWYU pragma: keep

#include "real_gballoc_ll.h"    // IWYU pragma: keep

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes.h"              // for IMPLEMENT_UMOCK_C_ENUM_TYPE
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umock_c_negative_tests.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS

#include "c_pal/completion_port_linux.h"
#include "c_pal/execution_engine.h"
#include "c_pal/gballoc_hl.h"        // IWYU pragma: keep
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_pal/platform_linux.h"
#include "c_pal/sync.h"
#include "c_pal/socket_handle.h"

#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_interlocked.h"
#include "real_gballoc_hl.h" // IWYU pragma: keep

#include "c_pal/async_socket.h"

#define TEST_MAX_EVENTS_NUM     64

#endif // ASYNC_SOCKET_LINUX_UT_PCH_H
