#Copyright (C) Microsoft Corporation. All rights reserved.

set(sample_h_files
)

set(sample_c_files
    main.c
)

set(sample_rc_files
    ${LOGGING_RC_FILE}
)

add_executable(helgrind_test ${sample_h_files} ${sample_c_files} ${sample_rc_files})

target_link_libraries(helgrind_test pal_interfaces pal_linux rt pthread)
