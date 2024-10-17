#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this cmakelists.txt in intended to build test for every combination of gballoc_hl and gballoc_ll
#to do that, it uses GBALOC_LL_IMPL and GBALLOC_HL_IMPL (which are set in a foreach() loop in the folder above).
#since it would result in multiple of the same target, it uses ${BINARY_DIR} suffix on the CMake binary build folder AND on the targets that it builds (they all need to be unique)


set(theseTestsNameBase gballoc_hl_int)

set(theseTestsName ${theseTestsNameBase}_${BINARY_DIR}_int)

set(${theseTestsName}_test_files
    ${theseTestsNameBase}.c
)

set(${theseTestsName}_c_files
    ../../src/gballoc_ll_${gballoc_ll_impl_lower}.c
    ../../src/gballoc_hl_${gballoc_hl_impl_lower}.c
    ../../src/timer_win32.c #needed because gballoc_hl needs timer to compute "how much time it takes"
    ../../../common/src/call_once.c
    ../../../common/src/lazy_init.c
)

set(${theseTestsName}_h_files
)

if(${GBALOC_LL_IMPL} STREQUAL "MIMALLOC")
    build_test_artifacts(${theseTestsName} "tests/c_pal/common" ADDITIONAL_LIBS pal_interfaces c_pal_ll synchronization mimalloc-obj)
elseif(${GBALOC_LL_IMPL} STREQUAL "JEMALLOC")
    build_test_artifacts(${theseTestsName} "tests/c_pal/common" ADDITIONAL_LIBS pal_interfaces c_pal_ll synchronization jemalloc)
else()
    build_test_artifacts(${theseTestsName} "tests/c_pal/common" ADDITIONAL_LIBS pal_interfaces c_pal_ll synchronization)
endif()

