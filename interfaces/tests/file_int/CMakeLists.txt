#Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName file_int)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_h_files
    ../../inc/c_pal/file.h
    file_int_helpers.h
)

if(WIN32)
    set(${theseTestsName}_c_files
        file_int_helpers_win32.c
    )
else()
    set(${theseTestsName}_c_files
        file_int_helpers_linux.c
    )
endif()

build_test_artifacts(${theseTestsName} "tests/c_pal/int" ADDITIONAL_LIBS c_pal)
