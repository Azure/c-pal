#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName gballoc_ll_win32heap_ut) 

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    gballoc_ll_win32heap_mocked.c
)

set(${theseTestsName}_h_files
)

build_test_artifacts(${theseTestsName} "tests/c_pal/common" ADDITIONAL_LIBS pal_interfaces win32_reals) 

if("${building}" STREQUAL "exe")
    set_target_properties(${theseTestsName}_exe_${CMAKE_PROJECT_NAME} PROPERTIES LINK_FLAGS "/ignore:4217")
endif()

if("${building}" STREQUAL "dll")
    set_target_properties(${theseTestsName}_dll_${CMAKE_PROJECT_NAME} PROPERTIES LINK_FLAGS "/ignore:4217")
endif()
