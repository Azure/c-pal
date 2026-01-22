#Copyright (C) Microsoft Corporation. All rights reserved.

set(process_watchdog_int_child_c_files
    process_watchdog_int_child.c
)

add_executable(process_watchdog_int_child ${process_watchdog_int_child_c_files})

target_link_libraries(process_watchdog_int_child c_pal c_logging_v2)

set_output_folder_properties(process_watchdog_int_child)

set_target_properties(process_watchdog_int_child PROPERTIES
    FOLDER "tests/c_pal/linux")
