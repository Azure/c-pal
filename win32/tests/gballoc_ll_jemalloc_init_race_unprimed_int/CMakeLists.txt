#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName gballoc_ll_jemalloc_init_race_unprimed_int)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    ../../src/gballoc_ll_jemalloc.c
)

set(${theseTestsName}_h_files
)

# This test deliberately races the process's first jemalloc allocation and demonstrates the lazy-init
# crash that SRS_GBALLOC_LL_JEMALLOC_02_011 fixes. It hunts the failure by respawning child processes
# until one crashes/hangs, so it reliably fails; it is registered WILL_FAIL (CTest inverts the expected
# failure to a pass). TIMEOUT exceeds the in-test ~9 minute hunt budget.
build_test_artifacts(${theseTestsName} "tests/c_pal/common" ADDITIONAL_LIBS jemalloc pal_interfaces c_pal)

if("${building}" STREQUAL "exe")
    set_tests_properties(${theseTestsName} PROPERTIES WILL_FAIL TRUE TIMEOUT 700)
endif()
