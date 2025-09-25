#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName gballoc_hl_metrics_wout_init_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
../../src/gballoc_hl_metrics.c
)

set(${theseTestsName}_h_files

)

build_test_artifacts(${theseTestsName} "tests/c_pal/common" ADDITIONAL_LIBS c_pal c_pal_reals 
    ENABLE_TEST_FILES_PRECOMPILED_HEADERS "${CMAKE_CURRENT_LIST_DIR}/gballoc_hl_metrics_wout_init_ut_pch.h"
)

if("${building}" STREQUAL "exe")
    set_target_properties(${theseTestsName}_exe_${CMAKE_PROJECT_NAME} PROPERTIES LINK_FLAGS "/ignore:4217")
endif()

if("${building}" STREQUAL "dll")
    set_target_properties(${theseTestsName}_dll_${CMAKE_PROJECT_NAME} PROPERTIES LINK_FLAGS "/ignore:4217")
endif()