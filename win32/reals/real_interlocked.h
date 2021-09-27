// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_INTERLOCKED_H
#define REAL_INTERLOCKED_H

#ifdef __cplusplus
#include <cstdint>
#include <cstddef>
#else
#include <stdint.h>
#include <stddef.h>
#endif

#include "macro_utils/macro_utils.h"
#include "c_pal/interlocked.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

/*a list of interlocked APIs that are common on x86 and x64*/
#define ALL_COMMON_APIS                             \
        interlocked_add                             ,\
        interlocked_add_64                          ,\
        interlocked_and                             ,\
        interlocked_and_16                          ,\
        interlocked_and_64                          ,\
        interlocked_and_8                           ,\
        interlocked_compare_exchange                ,\
        interlocked_compare_exchange_16             ,\
        interlocked_compare_exchange_64             ,\
        interlocked_compare_exchange_pointer        ,\
        interlocked_decrement                       ,\
        interlocked_decrement_16                    ,\
        interlocked_decrement_64                    ,\
        interlocked_exchange                        ,\
        interlocked_exchange_16                     ,\
        interlocked_exchange_64                     ,\
        interlocked_exchange_8                      ,\
        interlocked_exchange_add                    ,\
        interlocked_exchange_add_64                 ,\
        interlocked_exchange_pointer                ,\
        interlocked_increment                       ,\
        interlocked_increment_16                    ,\
        interlocked_increment_64                    ,\
        interlocked_or                              ,\
        interlocked_or_16                           ,\
        interlocked_or_64                           ,\
        interlocked_or_8                            ,\
        interlocked_xor                             ,\
        interlocked_xor_16                          ,\
        interlocked_xor_64                          ,\
        interlocked_xor_8                           \

#ifdef _WIN64
/*add interlocked_compare_exchange_128 to the list of APIs*/
#define ALL_APIS ALL_COMMON_APIS, interlocked_compare_exchange_128
#else
/*for win32 there's no additional APIs*/
#define ALL_APIS ALL_COMMON_APIS
#endif

#define REGISTER_INTERLOCKED_GLOBAL_MOCK_HOOK()     \
    MU_FOR_EACH_1(R2,                               \
        ALL_APIS                                    \
    )

#ifdef __cplusplus
extern "C" {
#endif
int32_t real_interlocked_add(volatile_atomic int32_t* addend, int32_t value);
int64_t real_interlocked_add_64(volatile_atomic int64_t* addend, int64_t value);
int32_t real_interlocked_and(volatile_atomic int32_t* destination, int32_t value);
int16_t real_interlocked_and_16(volatile_atomic int16_t* destination, int16_t value);
int64_t real_interlocked_and_64(volatile_atomic int64_t* destination, int64_t value);
int8_t real_interlocked_and_8(volatile_atomic int8_t* destination, int8_t value);
int32_t real_interlocked_compare_exchange(volatile_atomic int32_t* destination, int32_t exchange, int32_t comperand);
#ifdef _WIN64
bool real_interlocked_compare_exchange_128(volatile_atomic int64_t* destination, int64_t exchange_high, int64_t exchange_low, int64_t* comperand_result);
#endif
int16_t real_interlocked_compare_exchange_16(volatile_atomic int16_t* destination, int16_t exchange, int16_t comperand);
int64_t real_interlocked_compare_exchange_64(volatile_atomic int64_t* destination, int64_t exchange, int64_t comperand);
void* real_interlocked_compare_exchange_pointer(void* volatile_atomic* destination, void* exchange, void* comperand);
int32_t real_interlocked_decrement(volatile_atomic int32_t* addend);
int16_t real_interlocked_decrement_16(volatile_atomic int16_t* addend);
int64_t real_interlocked_decrement_64(volatile_atomic int64_t* addend);
int32_t real_interlocked_exchange(volatile_atomic int32_t* target, int32_t value);
int16_t real_interlocked_exchange_16(volatile_atomic int16_t* target, int16_t value);
int64_t real_interlocked_exchange_64(volatile_atomic int64_t* target, int64_t value);
int8_t real_interlocked_exchange_8(volatile_atomic int8_t* target, int8_t value);
int32_t real_interlocked_exchange_add(volatile_atomic int32_t* addend, int32_t value);
int64_t real_interlocked_exchange_add_64(volatile_atomic int64_t* addend, int64_t value);
void* real_interlocked_exchange_pointer(void* volatile_atomic* target, void* value);
int32_t real_interlocked_increment(volatile_atomic int32_t* addend);
int16_t real_interlocked_increment_16(volatile_atomic int16_t* addend);
int64_t real_interlocked_increment_64(volatile_atomic int64_t* addend);
int32_t real_interlocked_or(volatile_atomic int32_t* destination, int32_t value);
int16_t real_interlocked_or_16(volatile_atomic int16_t* destination, int16_t value);
int64_t real_interlocked_or_64(volatile_atomic int64_t* destination, int64_t value);
int8_t real_interlocked_or_8(volatile_atomic int8_t* destination, int8_t value);
int32_t real_interlocked_xor(volatile_atomic int32_t* destination, int32_t value);
int16_t real_interlocked_xor_16(volatile_atomic int16_t* destination, int16_t value);
int64_t real_interlocked_xor_64(volatile_atomic int64_t* destination, int64_t value);
int8_t real_interlocked_xor_8(volatile_atomic int8_t* destination, int8_t value);

#ifdef __cplusplus
}
#endif

#endif //REAL_INTERLOCKED_H
