// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_THANDLE_LOG_CONTEXT_HANDLE_H
#define REAL_THANDLE_LOG_CONTEXT_HANDLE_H

#include "macro_utils/macro_utils.h"

#include "c_logging/log_context.h"

#include "c_pal/thandle_ptr.h"

#include "c_pal/thandle_log_context_handle.h"

typedef LOG_CONTEXT_HANDLE real_LOG_CONTEXT_HANDLE; /*real_LOG_CONTEXT_HANDLE is "the same type" as LOG_CONTEXT_HANDLE*/

THANDLE_PTR_DECLARE(real_LOG_CONTEXT_HANDLE)

#define REGISTER_THANDLE_LOG_CONTEXT_HANDLE_GLOBAL_MOCK_HOOK()                                                                                                                                                          \
    /*regular THANDLE hooks. Note: casts are needed because of C11's 6.7.2.3 Tags - Constraint 5 on "distinct types". The structs which are "distinct" here would be PTR(T) and PTR(real_T).*/                          \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_MOVE(PTR(LOG_CONTEXT_HANDLE)),            (THANDLE_MOVE_FUNCTION_TYPE(PTR(LOG_CONTEXT_HANDLE)))                   THANDLE_MOVE(PTR(real_LOG_CONTEXT_HANDLE)));                    \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_INITIALIZE(PTR(LOG_CONTEXT_HANDLE)),      (THANDLE_INITIALIZE_FUNCTION_TYPE(PTR(LOG_CONTEXT_HANDLE)))             THANDLE_INITIALIZE(PTR(real_LOG_CONTEXT_HANDLE)));              \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_INITIALIZE_MOVE(PTR(LOG_CONTEXT_HANDLE)), (THANDLE_INITIALIZE_MOVE_FUNCTION_TYPE(PTR(LOG_CONTEXT_HANDLE)))        THANDLE_INITIALIZE_MOVE(PTR(real_LOG_CONTEXT_HANDLE)));         \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_ASSIGN(PTR(LOG_CONTEXT_HANDLE)),          (THANDLE_ASSIGN_FUNCTION_TYPE(PTR(LOG_CONTEXT_HANDLE)))                 THANDLE_ASSIGN(PTR(real_LOG_CONTEXT_HANDLE)));                  \
    /*create hook*/                                                                                                                                                                                                     \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_PTR_CREATE_WITH_MOVE(LOG_CONTEXT_HANDLE), (THANDLE_PTR_CREATE_WITH_MOVE_FUNCTION_TYPE(LOG_CONTEXT_HANDLE))        THANDLE_PTR_CREATE_WITH_MOVE(real_LOG_CONTEXT_HANDLE));         \


#endif /*REAL_THANDLE_LOG_CONTEXT_HANDLE_H*/
