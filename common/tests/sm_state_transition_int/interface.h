// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef INTERFACE_H
#define INTERFACE_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#include <stdbool.h>
#endif

#include "macro_utils/macro_utils.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* CONCRETE_INTERFACE_HANDLE;
typedef void(*ON_INTERFACE_OPEN_COMPLETE)(void* context, bool open_result);
typedef void(*ON_INTERFACE_ERROR)(void* context);

typedef CONCRETE_INTERFACE_HANDLE(*INTERFACE_CREATE)(const void* interface_parameters);
typedef void(*INTERFACE_DESTROY)(CONCRETE_INTERFACE_HANDLE handle);
typedef int(*INTERFACE_OPEN_ASYNC)(CONCRETE_INTERFACE_HANDLE handle, ON_INTERFACE_OPEN_COMPLETE on_open_complete, void* on_open_complete_ctx, ON_INTERFACE_ERROR on_error, void* on_error_ctx);
typedef void(*INTERFACE_CLOSE)(CONCRETE_INTERFACE_HANDLE handle);

typedef struct INTERFACE_DESCRIPTION_TAG
{
    INTERFACE_CREATE create;
    INTERFACE_DESTROY destroy;
    INTERFACE_OPEN_ASYNC open_async;
    INTERFACE_CLOSE close;
} INTERFACE_DESCRIPTION;

#ifdef __cplusplus
}
#endif

#endif
