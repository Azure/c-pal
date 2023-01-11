// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef PLATFORM_LINUX_H
#define PLATFORM_LINUX_H

#include "umock_c/umock_c_prod.h"

#include "c_pal/completion_port_linux.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    MOCKABLE_FUNCTION(, COMPLETION_PORT_HANDLE, platform_get_completion_port);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PLATFORM_LINUX_H */