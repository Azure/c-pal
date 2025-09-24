// Copyright(C) Microsoft Corporation.All rights reserved.


// Precompiled header for platform_linux_ut

#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"

#define ENABLE_MOCKS

#include "c_pal/completion_port_linux.h"
#include "platform_mocked.h"

#undef ENABLE_MOCKS

#include "c_pal/platform.h"
#include "c_pal/platform_linux.h"