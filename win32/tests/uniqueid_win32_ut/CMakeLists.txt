#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName uniqueid_win32_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    uniqueid_win32_mocked.c
)

set(${theseTestsName}_h_files
    ../../../interfaces/inc/c_pal/uniqueid.h
)

build_test_artifacts(${theseTestsName} ON "tests/c_pal/win32" ADDITIONAL_LIBS rpcrt4.lib pal_interfaces)

