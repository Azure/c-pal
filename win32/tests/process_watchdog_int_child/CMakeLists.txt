#Copyright (C) Microsoft Corporation. All rights reserved.

set(process_watchdog_int_child_c_files
    process_watchdog_int_child.c
)

add_executable(process_watchdog_int_child ${process_watchdog_int_child_c_files})

target_include_directories(process_watchdog_int_child PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../../../interfaces/tests/process_watchdog_int_common)

target_link_libraries(process_watchdog_int_child c_pal c_logging_v2)

# Put child executable in same directory as process_watchdog_int test (not its own subdirectory)
foreach(config_iter DEBUG RELWITHDEBINFO RELEASE MINSIZEREL)
    set_target_properties(process_watchdog_int_child
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${config_iter} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config_iter}}/process_watchdog_int_exe_${CMAKE_PROJECT_NAME}")
endforeach()

set_target_properties(process_watchdog_int_child PROPERTIES
    FOLDER "tests/c_pal/win32")
