// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_INTERLOCKED_HL_H
#define REAL_INTERLOCKED_HL_H

#include "macro_utils/macro_utils.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_INTERLOCKED_HL_GLOBAL_MOCK_HOOK()                    \
    MU_FOR_EACH_1(R2,                      \
        InterlockedHL_Add64WithCeiling, \
        InterlockedHL_WaitForValue, \
        InterlockedHL_WaitForValue64, \
        InterlockedHL_WaitForNotValue, \
        InterlockedHL_WaitForNotValue64, \
        InterlockedHL_SetAndWake, \
        InterlockedHL_SetAndWake64, \
        InterlockedHL_SetAndWakeAll, \
        InterlockedHL_SetAndWakeAll64, \
        InterlockedHL_CompareExchangeIf, \
        InterlockedHL_CompareExchange64If, \
        InterlockedHL_DecrementAndWake, \
        InterlockedHL_DecrementAndWake64 \
    )

#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"



    INTERLOCKED_HL_RESULT real_InterlockedHL_Add64WithCeiling(int64_t volatile_atomic* Addend, int64_t Ceiling, int64_t Value, int64_t* originalAddend);
    INTERLOCKED_HL_RESULT real_InterlockedHL_WaitForValue(int32_t volatile_atomic* address_to_check, int32_t value_to_wait, uint32_t timeout_ms);
    INTERLOCKED_HL_RESULT real_InterlockedHL_WaitForValue64(int64_t volatile_atomic* address_to_check, int64_t value_to_wait, uint32_t timeout_ms);
    INTERLOCKED_HL_RESULT real_InterlockedHL_WaitForNotValue(int32_t volatile_atomic* address_to_check, int32_t value_to_wait, uint32_t timeout_ms);
    INTERLOCKED_HL_RESULT real_InterlockedHL_WaitForNotValue64(int64_t volatile_atomic* address_to_check, int64_t value_to_wait, uint32_t timeout_ms);
    INTERLOCKED_HL_RESULT real_InterlockedHL_SetAndWake(int32_t volatile_atomic* address, int32_t value);
    INTERLOCKED_HL_RESULT real_InterlockedHL_SetAndWake64(int64_t volatile_atomic* address, int64_t value);
    INTERLOCKED_HL_RESULT real_InterlockedHL_SetAndWakeAll(int32_t volatile_atomic* address, int32_t value);
    INTERLOCKED_HL_RESULT real_InterlockedHL_SetAndWakeAll64(int64_t volatile_atomic* address, int64_t value);
    INTERLOCKED_HL_RESULT real_InterlockedHL_CompareExchangeIf(int32_t volatile_atomic* target, int32_t exchange, INTERLOCKED_COMPARE_EXCHANGE_IF compare, int32_t* original_target);
    INTERLOCKED_HL_RESULT real_InterlockedHL_CompareExchange64If(int64_t volatile_atomic* target, int64_t exchange, INTERLOCKED_COMPARE_EXCHANGE_64_IF compare, int64_t* original_target);
    INTERLOCKED_HL_RESULT real_InterlockedHL_DecrementAndWake(int32_t volatile_atomic* address);
    INTERLOCKED_HL_RESULT real_InterlockedHL_DecrementAndWake64(int64_t volatile_atomic* address);




#endif //REAL_INTERLOCKED_HL_H
