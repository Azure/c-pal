#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName sync_win32_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
mock_sync.c
)

set(${theseTestsName}_h_files
../../../interfaces/inc/c_pal/sync.h
mock_sync.h
)

build_test_artifacts(${theseTestsName} "tests/c_pal/win32" ADDITIONAL_LIBS pal_interfaces c_pal_reals 
    ENABLE_TEST_FILES_PRECOMPILED_HEADERS "${CMAKE_CURRENT_LIST_DIR}/sync_win32_ut_pch.h"
)