// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_THANDLE_LOG_CONTEXT_HANDLE_H
#define REAL_THANDLE_LOG_CONTEXT_HANDLE_H

#include "macro_utils/macro_utils.h"

#include "c_logging/log_context.h"

#include "c_pal/thandle_ptr.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_THANDLE_LOG_CONTEXT_HANDLE_GLOBAL_MOCK_HOOK()                                                                                  \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_MOVE(PTR(LOG_CONTEXT_HANDLE)),            THANDLE_MOVE(PTR(real_LOG_CONTEXT_HANDLE)))                     \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_INITIALIZE(PTR(LOG_CONTEXT_HANDLE)),      THANDLE_INITIALIZE(PTR(real_LOG_CONTEXT_HANDLE)))               \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_INITIALIZE_MOVE(PTR(LOG_CONTEXT_HANDLE)), THANDLE_INITIALIZE_MOVE(PTR(real_LOG_CONTEXT_HANDLE)))          \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_ASSIGN(PTR(LOG_CONTEXT_HANDLE)),          THANDLE_ASSIGN(PTR(real_LOG_CONTEXT_HANDLE)))                   \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_PTR_CREATE_WITH_MOVE(LOG_CONTEXT_HANDLE), THANDLE_PTR_CREATE_WITH_MOVE(real_LOG_CONTEXT_HANDLE))          \

#include "c_logging/log_context.h"

#include "umock_c/umock_c_prod.h"

typedef struct LOG_CONTEXT_HANDLE_TAG* real_LOG_CONTEXT_HANDLE;

THANDLE_PTR_DECLARE(real_LOG_CONTEXT_HANDLE);

#endif /*REAL_THANDLE_LOG_CONTEXT_HANDLE_H*/
