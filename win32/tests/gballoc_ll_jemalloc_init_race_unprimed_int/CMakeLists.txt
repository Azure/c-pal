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

# Regression guard: races the process's first jemalloc allocation across many fresh child processes
# WITHOUT priming and fails if any child crashes, hangs, or returns non-zero. With jemalloc 5.3.1's
# __declspec(thread) TSD fix this is safe; a regression of the lazy-init race would make a child fail.
# Unlike the old demonstration test this is a normal (non-WILL_FAIL) test - it must pass. TIMEOUT
# covers spawning SPAWN_COUNT children plus per-child overhead.
build_test_artifacts(${theseTestsName} "tests/c_pal/common" ADDITIONAL_LIBS jemalloc pal_interfaces c_pal)

if("${building}" STREQUAL "exe")
    set_tests_properties(${theseTestsName} PROPERTIES TIMEOUT 300)
endif()
