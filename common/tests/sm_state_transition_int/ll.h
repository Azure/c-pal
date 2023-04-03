// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef LL_H
#define LL_H

#ifdef __cplusplus
#else
#include <stdbool.h>
#endif

#include "interface.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void(*ON_LL_OPEN_COMPLETE)(void* context, bool open_result);
typedef void(*ON_LL_ERROR)(void* context);

typedef void(*SYSTEM_MOCK_API_OPEN_ASYNC)(void* api_ctx, ON_LL_OPEN_COMPLETE open_complete, void* open_complete_ctx, ON_LL_ERROR on_error, void* on_error_ctx);
typedef void(*SYSTEM_MOCK_API_CLOSE)(void* api_ctx);

typedef struct LL_IO_CONFIG_TAG
{
    SYSTEM_MOCK_API_OPEN_ASYNC sys_api_open_async;
    void* sys_api_open_async_ctx;
    SYSTEM_MOCK_API_CLOSE sys_api_close;
    void* sys_api_close_ctx;
} LL_IO_CONFIG;

MOCKABLE_FUNCTION(, const INTERFACE_DESCRIPTION*, ll_get_interface_description);

#ifdef __cplusplus
}
#endif

#endif // LL_H
