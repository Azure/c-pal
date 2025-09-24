// Copyright(C) Microsoft Corporation.All rights reserved.


// Precompiled header for socket_transport_linux_ut

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "real_gballoc_ll.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS

#include "socket_mocked.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h" // IWYU pragma: keep
#include "c_pal/socket_handle.h"
#include "c_pal/sm.h"

#undef ENABLE_MOCKS

#include "umock_c/umock_c_prod.h"   // IWYU pragma: keep
#include "real_gballoc_hl.h"        // IWYU pragma: keep
#include "../reals/real_sm.h"

#include "c_pal/socket_transport.h"

#define MAX_SOCKET_ARRAY            10