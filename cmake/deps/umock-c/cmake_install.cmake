# Install script for directory: /mnt/c/azure/azure-c-pal/deps/umock-c

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/mnt/c/azure/azure-c-pal/cmake/deps/umock-c/libumock_c.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/umock_c" TYPE FILE FILES
    "/mnt/c/azure/azure-c-pal/deps/umock-c/./inc/umock_c/umock_c.h"
    "/mnt/c/azure/azure-c-pal/deps/umock-c/./inc/umock_c/umock_c_internal.h"
    "/mnt/c/azure/azure-c-pal/deps/umock-c/./inc/umock_c/umock_c_negative_tests.h"
    "/mnt/c/azure/azure-c-pal/deps/umock-c/./inc/umock_c/umock_c_prod.h"
    "/mnt/c/azure/azure-c-pal/deps/umock-c/./inc/umock_c/umockalloc.h"
    "/mnt/c/azure/azure-c-pal/deps/umock-c/./inc/umock_c/umockcall.h"
    "/mnt/c/azure/azure-c-pal/deps/umock-c/./inc/umock_c/umockcallrecorder.h"
    "/mnt/c/azure/azure-c-pal/deps/umock-c/./inc/umock_c/umocktypename.h"
    "/mnt/c/azure/azure-c-pal/deps/umock-c/./inc/umock_c/umocktypes.h"
    "/mnt/c/azure/azure-c-pal/deps/umock-c/./inc/umock_c/umocktypes_bool.h"
    "/mnt/c/azure/azure-c-pal/deps/umock-c/./inc/umock_c/umocktypes_c.h"
    "/mnt/c/azure/azure-c-pal/deps/umock-c/./inc/umock_c/umocktypes_stdint.h"
    "/mnt/c/azure/azure-c-pal/deps/umock-c/./inc/umock_c/umocktypes_charptr.h"
    "/mnt/c/azure/azure-c-pal/deps/umock-c/./inc/umock_c/umocktypes_struct.h"
    "/mnt/c/azure/azure-c-pal/deps/umock-c/./inc/umock_c/umocktypes_wcharptr.h"
    "/mnt/c/azure/azure-c-pal/deps/umock-c/./inc/umock_c/umockcallpairs.h"
    "/mnt/c/azure/azure-c-pal/deps/umock-c/./inc/umock_c/umockstring.h"
    "/mnt/c/azure/azure-c-pal/deps/umock-c/./inc/umock_c/umockautoignoreargs.h"
    "/mnt/c/azure/azure-c-pal/deps/umock-c/./inc/umock_c/umock_log.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/umock_cTargets.cmake")
    file(DIFFERENT EXPORT_FILE_CHANGED FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/umock_cTargets.cmake"
         "/mnt/c/azure/azure-c-pal/cmake/deps/umock-c/CMakeFiles/Export/cmake/umock_cTargets.cmake")
    if(EXPORT_FILE_CHANGED)
      file(GLOB OLD_CONFIG_FILES "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/umock_cTargets-*.cmake")
      if(OLD_CONFIG_FILES)
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/umock_cTargets.cmake\" will be replaced.  Removing files [${OLD_CONFIG_FILES}].")
        file(REMOVE ${OLD_CONFIG_FILES})
      endif()
    endif()
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "/mnt/c/azure/azure-c-pal/cmake/deps/umock-c/CMakeFiles/Export/cmake/umock_cTargets.cmake")
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^()$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "/mnt/c/azure/azure-c-pal/cmake/deps/umock-c/CMakeFiles/Export/cmake/umock_cTargets-noconfig.cmake")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES
    "/mnt/c/azure/azure-c-pal/deps/umock-c/configs/umock_cFunctions.cmake"
    "/mnt/c/azure/azure-c-pal/deps/umock-c/configs/umock_cConfig.cmake"
    "/mnt/c/azure/azure-c-pal/cmake/deps/umock-c/umock_c/umock_cConfigVersion.cmake"
    )
endif()

