#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName gballoc_ll_jemalloc_init_race_unprimed_int)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    ../../src/gballoc_ll_jemalloc.c
    ../gballoc_ll_jemalloc_init_race_common/gballoc_ll_jemalloc_init_race_common.c
)

set(${theseTestsName}_h_files
)

# This test deliberately races the process's first jemalloc allocation and demonstrates the lazy-init
# crash that SRS_GBALLOC_LL_JEMALLOC_02_011 fixes. The shared driver spawns children until one crashes
# or hangs, so it reliably fails; it is registered WILL_FAIL (CTest inverts the expected failure to a
# pass). TIMEOUT covers the 3 minute spawn budget plus per-child overhead.
include_directories(${CMAKE_CURRENT_LIST_DIR}/../gballoc_ll_jemalloc_init_race_common)

build_test_artifacts(${theseTestsName} "tests/c_pal/common" ADDITIONAL_LIBS jemalloc pal_interfaces c_pal)

if("${building}" STREQUAL "exe")
    set_tests_properties(${theseTestsName} PROPERTIES WILL_FAIL TRUE TIMEOUT 300)
endif()
