// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "rpc.h"
#include "windows.h"



#define UuidCreate mocked_UuidCreate
#define UuidToStringA mocked_UuidToStringA
#define RpcStringFreeA mocked_RpcStringFreeA

extern RPC_STATUS mocked_UuidCreate(
    UUID __RPC_FAR* Uuid);
extern RPC_STATUS mocked_UuidToStringA(
    const UUID __RPC_FAR* Uuid,
    RPC_CSTR __RPC_FAR* StringUuid);
extern RPC_STATUS mocked_RpcStringFreeA(
    RPC_CSTR __RPC_FAR* String);



#include "../../src/uniqueid_win32.c"
