#Copyright (C) Microsoft Corporation. All rights reserved.


set(job_object_helper_tester_c_files
    job_object_helper_tester_process.c
)

set(job_object_helper_tester_h_files
)

include_directories(
   ../../common/inc
   ../../pal/inc
   ../../c_pal_ll/interfaces/inc
   ../../win32/inc
   ../../deps/macro-utils-c/inc
   ../../deps/umock-c/inc
   ../../deps/c-logging/v2/inc
)

add_executable(job_object_helper_tester ${job_object_helper_tester_c_files} ${job_object_helper_tester_h_files})

target_link_libraries(job_object_helper_tester pal_win32 pal_ll_win32)
set_output_folder_properties(job_object_helper_tester)

set_target_properties(job_object_helper_tester PROPERTIES
    FOLDER "tools")
