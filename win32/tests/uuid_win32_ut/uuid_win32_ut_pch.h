// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for uuid_win32_ut

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "rpc.h"

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS
#include "umock_c/umock_c_prod.h"
#undef ENABLE_MOCKS

#include "c_pal/uuid.h"

#define TEST_DATA_1 0x12233445
#define TEST_DATA_2 0x5667
#define TEST_DATA_3 0x7889
#define TEST_DATA_4_0 0x9A
#define TEST_DATA_4_1 0xAB
#define TEST_DATA_4_2 0xBC
#define TEST_DATA_4_3 0xCD
#define TEST_DATA_4_4 0xDE
#define TEST_DATA_4_5 0xEF
#define TEST_DATA_4_6 0xF0
#define TEST_DATA_4_7 0x01