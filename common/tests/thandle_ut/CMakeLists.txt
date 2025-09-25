#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName thandle_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
    thandle_user.c
    thandle_user_33_characters.c
    thandle_flex_user.c
)

set(${theseTestsName}_h_files
    ../../inc/c_pal/thandle.h
    thandle_user.h
    thandle_user_33_characters.h
    thandle_flex_user.h
)

build_test_artifacts(${theseTestsName} "tests/c_pal" ADDITIONAL_LIBS c_pal c_pal_reals 
    ENABLE_TEST_FILES_PRECOMPILED_HEADERS "${CMAKE_CURRENT_LIST_DIR}/thandle_ut_pch.h"
)
