// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef GBALLOC_LL_JEMALLOC_INIT_RACE_COMMON_H
#define GBALLOC_LL_JEMALLOC_INIT_RACE_COMMON_H

#include <stdbool.h>

// Shared driver for the gballoc_ll_jemalloc init-race integration tests. The primed and unprimed
// tests are identical except for the single boolean passed here, which is the only differentiator.
//
// When this process is a spawned child (an internal environment marker is set) it runs a single
// 16-thread race of the process's first jemalloc allocation and then exits; when prime is true it
// first primes jemalloc with gballoc_ll_init so that race is safe.
//
// Otherwise (the parent) it spawns fresh child processes for a fixed 3 minute budget, stopping early
// if any child crashes, hangs, or returns non-zero, and ASSERT_FAILs when that happens. With
// prime==true every child should finish and return 0, so the test passes; with prime==false a child
// fails almost immediately, and that test is registered WILL_FAIL so the failure inverts to a pass.
void gballoc_ll_jemalloc_init_race_run(bool prime);

#endif // GBALLOC_LL_JEMALLOC_INIT_RACE_COMMON_H
