// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "macro_utils/macro_utils.h"

#include "c_logging/log_context.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/thandle_ll.h"
#include "c_pal/thandle.h"
#include "c_pal/thandle_ptr.h"

#include "c_pal/thandle_logging_context.h"

THANDLE_PTR_DEFINE(LOG_CONTEXT_HANDLE);