#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName unique_thandle_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_h_files
    ../../inc/c_pal/unique_thandle.h
    ../../inc/c_pal/unique_thandle_ll.h
)

build_test_artifacts(${theseTestsName} "tests/c_pal" ADDITIONAL_LIBS c_pal c_pal_reals
    ENABLE_TEST_FILES_PRECOMPILED_HEADERS "${CMAKE_CURRENT_LIST_DIR}/unique_thandle_ut_pch.h"
)
