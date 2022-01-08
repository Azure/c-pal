// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "rpc.h"
#include "windows.h"

#define UuidCreate mocked_UuidCreate

extern RPC_STATUS mocked_UuidCreate(
    UUID __RPC_FAR* Uuid);

#include "../../src/uuid_win32.c"
