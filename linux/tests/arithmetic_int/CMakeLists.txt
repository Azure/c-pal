#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName arithmetic_int) 

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    ../../src/arithmetic_linux.c
)

set(${theseTestsName}_h_files
)

build_test_artifacts(${theseTestsName} "tests/c_pal/common" ADDITIONAL_LIBS pal_interfaces) 

if(WIN32)

    if("${building}" STREQUAL "exe")
        set_target_properties(${theseTestsName}_exe PROPERTIES LINK_FLAGS "/ignore:4217")
    endif()
    
    if("${building}" STREQUAL "dll")
        set_target_properties(${theseTestsName}_dll PROPERTIES LINK_FLAGS "/ignore:4217")
    endif()
endif()
