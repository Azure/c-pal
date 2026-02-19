#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

# Negative tests (no job objects created in current process)
set(theseTestsName job_object_helper_negative_int)
set(${theseTestsName}_test_files
    ${theseTestsName}.c
)
set(${theseTestsName}_c_files)
set(${theseTestsName}_h_files)
build_test_artifacts(${theseTestsName} "tests/c_pal/win32" ADDITIONAL_LIBS c_pal)

# Basic creation test (creates job object in current process - 1 test per process)
set(theseTestsName job_object_helper_create_int)
set(${theseTestsName}_test_files
    ${theseTestsName}.c
)
set(${theseTestsName}_c_files)
set(${theseTestsName}_h_files)
build_test_artifacts(${theseTestsName} "tests/c_pal/win32" ADDITIONAL_LIBS c_pal)
add_dependencies(${theseTestsName}_exe_${CMAKE_PROJECT_NAME} job_object_helper_tester)

# Memory limits test (creates job object in current process - 1 test per process)
set(theseTestsName job_object_helper_memory_int)
set(${theseTestsName}_test_files
    ${theseTestsName}.c
)
set(${theseTestsName}_c_files)
set(${theseTestsName}_h_files)
build_test_artifacts(${theseTestsName} "tests/c_pal/win32" ADDITIONAL_LIBS c_pal)

# Multi-process tests (creates job objects in child processes only)
set(theseTestsName job_object_helper_multi_process_int)
set(${theseTestsName}_test_files
    ${theseTestsName}.c
)
set(${theseTestsName}_c_files)
set(${theseTestsName}_h_files)
build_test_artifacts(${theseTestsName} "tests/c_pal/win32" ADDITIONAL_LIBS c_pal)
add_dependencies(${theseTestsName}_exe_${CMAKE_PROJECT_NAME} job_object_helper_tester)

# Singleton/CPU compounding test (creates job object in current process - 1 test per process)
set(theseTestsName job_object_helper_singleton_int)
set(${theseTestsName}_test_files
    ${theseTestsName}.c
)
set(${theseTestsName}_c_files)
set(${theseTestsName}_h_files)
build_test_artifacts(${theseTestsName} "tests/c_pal/win32" ADDITIONAL_LIBS c_pal)
