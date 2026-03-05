#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(int_test_names
    job_object_helper_negative_int
    job_object_helper_create_int
    job_object_helper_memory_int
    job_object_helper_multi_process_int
    job_object_helper_singleton_int
    job_object_helper_reconfigure_int
)

# Tests that need the helper tester executable built first
set(tests_needing_tester
    job_object_helper_create_int
    job_object_helper_multi_process_int
)

foreach(theseTestsName IN LISTS int_test_names)
    set(${theseTestsName}_test_files
        ${theseTestsName}.c
    )
    set(${theseTestsName}_c_files)
    set(${theseTestsName}_h_files)
    build_test_artifacts(${theseTestsName} "tests/c_pal/win32" ADDITIONAL_LIBS c_pal)

    if(${theseTestsName} IN_LIST tests_needing_tester)
        add_dependencies(${theseTestsName}_exe_${CMAKE_PROJECT_NAME} job_object_helper_tester)
    endif()
endforeach()
