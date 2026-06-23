#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName gballoc_ll_jemalloc_init_race_primed_int)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    ../../src/gballoc_ll_jemalloc.c
)

set(${theseTestsName}_h_files
)

# Gated regression guard for SRS_GBALLOC_LL_JEMALLOC_02_011: with jemalloc primed in the suite
# initializer the race is safe and this test passes deterministically.
build_test_artifacts(${theseTestsName} "tests/c_pal/common" ADDITIONAL_LIBS jemalloc pal_interfaces c_pal)
