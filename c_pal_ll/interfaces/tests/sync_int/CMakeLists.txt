#Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName sync_int)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_h_files
    ../../inc/c_pal/sync.h
)

build_test_artifacts(${theseTestsName} "tests/c_pal_ll/int" ADDITIONAL_LIBS c_pal)
