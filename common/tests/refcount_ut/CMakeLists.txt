#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName refcount_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
    some_refcount_impl.c
)

set(${theseTestsName}_h_files
    some_refcount_impl.h
)

build_test_artifacts(${theseTestsName} "tests/c_pal/common" ADDITIONAL_LIBS ${PAL_OS} c_pal_reals 
    ENABLE_TEST_FILES_PRECOMPILED_HEADERS "${CMAKE_CURRENT_LIST_DIR}/refcount_ut_pch.h"
)