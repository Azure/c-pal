#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName single_performance_counter_win32_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
   single_performance_counter_mocked.c
)

set(${theseTestsName}_h_files
   ../../../interfaces/inc/c_pal/single_performance_counter.h
)

build_test_artifacts(${theseTestsName} "tests/c_pal" ADDITIONAL_LIBS c_pal_reals c_pal_umocktypes 
    ENABLE_TEST_FILES_PRECOMPILED_HEADERS "${CMAKE_CURRENT_LIST_DIR}/single_performance_counter_win32_ut_pch.h"
)