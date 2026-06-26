#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName gballoc_ll_jemalloc_init_race_int)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    ../../src/gballoc_ll_jemalloc.c
)

set(${theseTestsName}_h_files
)

# Regression guard: races the process's first jemalloc allocation across many fresh child processes
# and fails if any child crashes, hangs, or returns non-zero. jemalloc's one-time lazy init must be
# safe when several threads make the first allocation at once; a regression would make a child fail.
# TIMEOUT covers spawning SPAWN_COUNT children plus per-child overhead.
build_test_artifacts(${theseTestsName} "tests/c_pal/common" ADDITIONAL_LIBS jemalloc pal_interfaces c_pal)

if("${building}" STREQUAL "exe")
    set_tests_properties(${theseTestsName} PROPERTIES TIMEOUT 300)
endif()
