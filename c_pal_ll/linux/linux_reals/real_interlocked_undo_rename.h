// Copyright (c) Microsoft. All rights reserved.

// Intentionally leave out include guard
// This file may be included after real_interlocked_renames.h in order to undo the rename
// For example, to use reals for a THANDLE definition but mocks after that

#undef interlocked_add
#undef interlocked_add_64
#undef interlocked_and
#undef interlocked_and_16
#undef interlocked_and_64
#undef interlocked_and_8
#undef interlocked_compare_exchange
#undef interlocked_compare_exchange_16
#undef interlocked_compare_exchange_64
#undef interlocked_compare_exchange_pointer
#undef interlocked_decrement
#undef interlocked_decrement_16
#undef interlocked_decrement_64
#undef interlocked_exchange
#undef interlocked_exchange_16
#undef interlocked_exchange_64
#undef interlocked_exchange_8
#undef interlocked_exchange_add
#undef interlocked_exchange_add_64
#undef interlocked_exchange_pointer
#undef interlocked_increment
#undef interlocked_increment_16
#undef interlocked_increment_64
#undef interlocked_or
#undef interlocked_or_16
#undef interlocked_or_64
#undef interlocked_or_8
#undef interlocked_xor
#undef interlocked_xor_16
#undef interlocked_xor_64
#undef interlocked_xor_8