#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName platform_win32_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    platform_win32_mocked.c
)

set(${theseTestsName}_h_files
)

build_test_artifacts(${theseTestsName} "tests/c_pal/win32" ADDITIONAL_LIBS pal_interfaces)
