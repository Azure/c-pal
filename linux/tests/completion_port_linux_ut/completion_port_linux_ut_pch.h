// Copyright(C) Microsoft Corporation.All rights reserved.


// Precompiled header for completion_port_linux_ut

#include <stdlib.h>
#include <inttypes.h>
#include <sys/epoll.h>
#include <errno.h>
#include <string.h>                          // for memset

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "real_gballoc_ll.h"    // IWYU pragma: keep

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS

#include "c_pal/gballoc_hl.h"        // IWYU pragma: keep
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/refcount.h"  // IWYU pragma: keep
#include "c_pal/socket_handle.h"
#include "c_pal/s_list.h"
#include "c_pal/sync.h"
#include "c_pal/threadapi.h"

#undef ENABLE_MOCKS

#include "real_refcount.h"  // IWYU pragma: keep
#include "real_interlocked.h"
#include "real_interlocked_hl.h"
#include "real_gballoc_hl.h" // IWYU pragma: keep
#include "real_s_list.h" // IWYU pragma: keep

#include "c_pal/completion_port_linux.h"

#define TEST_MAX_EVENTS_NUM     64
#define EVENTS_TIMEOUT_MS       2*1000

#define TEST_COMPLETION_PORT_CALLBACK_EXECUTING  2
#define TEST_COMPLETION_PORT_CALLBACK_EXECUTED   3