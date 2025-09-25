#Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName interlocked_macros_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_h_files
    ../../inc/c_pal/interlocked_macros.h
)

build_test_artifacts(${theseTestsName} "tests/c_pal_ll/int" ADDITIONAL_LIBS pal_ll_interfaces c_pal 
    ENABLE_TEST_FILES_PRECOMPILED_HEADERS "${CMAKE_CURRENT_LIST_DIR}/interlocked_macros_ut_pch.h"
)