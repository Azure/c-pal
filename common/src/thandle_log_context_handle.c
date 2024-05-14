// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "macro_utils/macro_utils.h"

#include "c_logging/log_context.h"

#include "c_pal/gballoc_hl.h"  // IWYU pragma: keep
#include "c_pal/gballoc_hl_redirect.h" // IWYU pragma: keep
#include "c_pal/interlocked.h" // IWYU pragma: keep

#include "c_pal/thandle_ll.h" // IWYU pragma: keep
#include "c_pal/thandle.h" // IWYU pragma: keep
#include "c_pal/thandle_ptr.h"

#include "c_pal/thandle_log_context_handle.h"

THANDLE_PTR_DEFINE(LOG_CONTEXT_HANDLE);
