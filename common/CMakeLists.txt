#Copyright (C) Microsoft Corporation. All rights reserved.

set(pal_common_h_files
    ../common/inc/gballoc.h
    ../common/inc/constbuffer.h
    ../common/inc/buffer_.h
    PARENT_SCOPE
)

set(pal_common_c_files
    ../common/src/gballoc.c
    ../common/src/constbuffer.c
    ../common/src/buffer.c
    PARENT_SCOPE
)

FILE(GLOB pal_common_md_files "devdoc/*.md")
SOURCE_GROUP(devdoc FILES ${pal_common_md_files})

include_directories(${CMAKE_CURRENT_LIST_DIR}/inc)

#add_library(pal_common ${pal_common_h_files} ${pal_common_c_files} ${pal_common_md_files})
#target_link_libraries(pal_common pal_interfaces azure_c_logging)
#target_include_directories(pal_common PUBLIC ${CMAKE_CURRENT_LIST_DIR}/inc)

#add_subdirectory(tests)

