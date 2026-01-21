#Copyright (C) Microsoft Corporation. All rights reserved.

set(theseTestsName timed_test_suite_ut)

set(${theseTestsName}_c_files
    ${theseTestsName}.c
    main.c
)

set(${theseTestsName}_h_files
    ${theseTestsName}.h
)

add_executable(${theseTestsName} ${${theseTestsName}_c_files} ${${theseTestsName}_h_files})

set_target_properties(${theseTestsName}
    PROPERTIES
    FOLDER "tests/c_pal"
)

target_compile_definitions(${theseTestsName} PUBLIC USE_CTEST)

target_include_directories(${theseTestsName} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../../inc
    ${CMAKE_CURRENT_LIST_DIR}/../../../interfaces/inc
)

target_link_libraries(${theseTestsName} testrunnerswitcher c_logging_v2)

if(${run_unittests})
    add_test(NAME ${theseTestsName} COMMAND ${theseTestsName})
endif()
