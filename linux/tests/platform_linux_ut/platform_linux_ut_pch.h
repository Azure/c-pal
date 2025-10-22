// Copyright(C) Microsoft Corporation.All rights reserved.


// Precompiled header for platform_linux_ut

#ifndef PLATFORM_LINUX_UT_PCH_H
#define PLATFORM_LINUX_UT_PCH_H

#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS

#include "c_pal/completion_port_linux.h"
#include "platform_mocked.h"

#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "c_pal/platform.h"
#include "c_pal/platform_linux.h"

#endif // PLATFORM_LINUX_UT_PCH_H
