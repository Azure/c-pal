#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#add a library with the types that can be used for umock.
add_library(c_pal_umocktypes umocktypes_uuid_t.c ./inc/c_pal/umocktypes_uuid_t.h)

#include c_pal's include folders
include_directories($<TARGET_PROPERTY:c_pal,INTERFACE_INCLUDE_DIRECTORIES>)

#add the current folder to include paths
target_include_directories(c_pal_umocktypes PUBLIC ./inc)

add_subdirectory(tests)