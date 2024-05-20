// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef THANDLE_LOG_CONTEXT_HANDLE_H
#define THANDLE_LOG_CONTEXT_HANDLE_H

#include "macro_utils/macro_utils.h"

#include "c_logging/log_context.h"

#include "c_pal/thandle_ll.h"
#include "c_pal/thandle.h"
#include "c_pal/thandle_ptr.h"

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C"
{
#endif

    THANDLE_PTR_DECLARE(LOG_CONTEXT_HANDLE);

#ifdef __cplusplus
}
#endif

#endif /*THANDLE_LOG_CONTEXT_HANDLE_H*/
