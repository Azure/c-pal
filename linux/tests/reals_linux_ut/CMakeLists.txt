#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName reals_linux_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

build_c_tests(${theseTestsName} ON "tests/azure_c_pal/linux" ADDITIONAL_LIBS pal_linux reals_linux)
