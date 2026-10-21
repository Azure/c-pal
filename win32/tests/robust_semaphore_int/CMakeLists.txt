#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName robust_semaphore_int)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
)

set(${theseTestsName}_h_files
)

build_test_artifacts(${theseTestsName} "tests/c_pal/win32" ADDITIONAL_LIBS c_pal)

# the tests start robust_semaphore_tester to hold a unit from another process
if(TARGET ${theseTestsName}_exe_${CMAKE_PROJECT_NAME})
    add_dependencies(${theseTestsName}_exe_${CMAKE_PROJECT_NAME} robust_semaphore_tester)
endif()
if(TARGET ${theseTestsName}_dll_${CMAKE_PROJECT_NAME})
    add_dependencies(${theseTestsName}_dll_${CMAKE_PROJECT_NAME} robust_semaphore_tester)
endif()
