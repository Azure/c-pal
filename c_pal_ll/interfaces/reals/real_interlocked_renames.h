// Copyright (c) Microsoft. All rights reserved.

#define interlocked_add                        real_interlocked_add
#define interlocked_add_64                     real_interlocked_add_64
#define interlocked_and                        real_interlocked_and
#define interlocked_and_16                     real_interlocked_and_16
#define interlocked_and_64                     real_interlocked_and_64
#define interlocked_and_8                      real_interlocked_and_8
#define interlocked_compare_exchange           real_interlocked_compare_exchange
#ifdef _WIN64                          
#define interlocked_compare_exchange_128       real_interlocked_compare_exchange_128
#endif                                 
#define interlocked_compare_exchange_16        real_interlocked_compare_exchange_16
#define interlocked_compare_exchange_64        real_interlocked_compare_exchange_64
#define interlocked_compare_exchange_pointer   real_interlocked_compare_exchange_pointer
#define interlocked_decrement                  real_interlocked_decrement
#define interlocked_decrement_16               real_interlocked_decrement_16
#define interlocked_decrement_64               real_interlocked_decrement_64
#define interlocked_exchange                   real_interlocked_exchange
#define interlocked_exchange_16                real_interlocked_exchange_16
#define interlocked_exchange_64                real_interlocked_exchange_64
#define interlocked_exchange_8                 real_interlocked_exchange_8
#define interlocked_exchange_add               real_interlocked_exchange_add
#define interlocked_exchange_add_64            real_interlocked_exchange_add_64
#define interlocked_exchange_pointer           real_interlocked_exchange_pointer
#define interlocked_increment                  real_interlocked_increment
#define interlocked_increment_16               real_interlocked_increment_16
#define interlocked_increment_64               real_interlocked_increment_64
#define interlocked_or                         real_interlocked_or
#define interlocked_or_16                      real_interlocked_or_16
#define interlocked_or_64                      real_interlocked_or_64
#define interlocked_or_8                       real_interlocked_or_8
#define interlocked_xor                        real_interlocked_xor
#define interlocked_xor_16                     real_interlocked_xor_16
#define interlocked_xor_64                     real_interlocked_xor_64
#define interlocked_xor_8                      real_interlocked_xor_8
