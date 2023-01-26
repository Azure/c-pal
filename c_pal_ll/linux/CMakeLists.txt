#Copyright (C) Microsoft Corporation. All rights reserved.

set(pal_ll_linux_h_files
)

set(pal_ll_linux_c_files
    src/interlocked_linux.c
    src/sync_linux.c
)

FILE(GLOB pal_ll_linux_md_files "devdoc/*.md")
SOURCE_GROUP(devdoc FILES ${pal_ll_linux_md_files})

include_directories(${CMAKE_CURRENT_LIST_DIR}/inc)

add_library(pal_ll_linux ${pal_ll_linux_h_files} ${pal_ll_linux_c_files} ${pal_ll_linux_md_files})
set_target_properties(pal_ll_linux PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(pal_ll_linux pal_interfaces rt pthread)
target_include_directories(pal_ll_linux PUBLIC ${CMAKE_CURRENT_LIST_DIR}/inc)

# Set CMAKE_INSTALL_* if not defined
include(GNUInstallDirs)

if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
    set(CMAKE_INSTALL_LIBDIR "lib")
endif()

# Install pal_ll_linux
set(package_location "cmake")

install(TARGETS pal_ll_linux EXPORT pal_ll_linuxTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/../bin
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES ${pal_ll_linux_h_files} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/c_pal)

add_subdirectory(linux_reals)
add_subdirectory(tests)
