#Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName sysinfo_int)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_h_files
    ../../inc/c_pal/sysinfo.h
)

build_test_artifacts(${theseTestsName} ON "tests/c_pal/int" ADDITIONAL_LIBS c_pal)
