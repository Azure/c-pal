#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName job_object_helper_int)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    job_object_helper_mocked.c
)

set(${theseTestsName}_h_files
)

include_directories(../../inc)

build_test_artifacts(${theseTestsName} "tests/c_pal/win32" ADDITIONAL_LIBS pal_interfaces pal_ll_interfaces c_pal_reals c_pal_ll_reals)
