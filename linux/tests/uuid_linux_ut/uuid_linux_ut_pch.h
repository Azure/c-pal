// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for uuid_linux_ut

#ifndef UUID_LINUX_UT_PCH_H
#define UUID_LINUX_UT_PCH_H

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>

#include <uuid/uuid.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep
#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "c_pal/uuid.h"

#define TEST_DATA_0  0x01
#define TEST_DATA_1  0x12
#define TEST_DATA_2  0x23
#define TEST_DATA_3  0x34
#define TEST_DATA_4  0x45
#define TEST_DATA_5  0x56
#define TEST_DATA_6  0x67
#define TEST_DATA_7  0x78
#define TEST_DATA_8  0x89
#define TEST_DATA_9  0x9A
#define TEST_DATA_10 0xAB
#define TEST_DATA_11 0xBC
#define TEST_DATA_12 0xCD
#define TEST_DATA_13 0xDE
#define TEST_DATA_14 0xEF
#define TEST_DATA_15 0xF0

#endif // UUID_LINUX_UT_PCH_H
