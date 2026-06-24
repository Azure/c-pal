#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName interlocked_linux_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
mock_interlocked.c
)

set(${theseTestsName}_h_files
../../../interfaces/inc/c_pal/interlocked.h
mock_interlocked.h
)

build_test_artifacts(${theseTestsName} "tests/c_pal/linux" ADDITIONAL_LIBS pal_ll_interfaces 
    ENABLE_TEST_FILES_PRECOMPILED_HEADERS "${CMAKE_CURRENT_LIST_DIR}/interlocked_linux_ut_pch.h"
)