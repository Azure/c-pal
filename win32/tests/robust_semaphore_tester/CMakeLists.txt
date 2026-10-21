#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(robust_semaphore_tester_c_files
    robust_semaphore_tester.c
)

set(robust_semaphore_tester_h_files
)

add_executable(robust_semaphore_tester ${robust_semaphore_tester_c_files} ${robust_semaphore_tester_h_files})

target_link_libraries(robust_semaphore_tester pal_win32 pal_ll_win32)

set_output_folder_properties(robust_semaphore_tester)

set_target_properties(robust_semaphore_tester PROPERTIES
    FOLDER "tests/c_pal/win32")
