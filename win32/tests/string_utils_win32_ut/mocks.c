// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "windows.h"
#include "mocks.h"

#define FileTimeToSystemTime mocked_FileTimeToSystemTime

#include "../../src/string_utils.c"
