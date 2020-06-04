#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "umock_c" for configuration ""
set_property(TARGET umock_c APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(umock_c PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "C"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libumock_c.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS umock_c )
list(APPEND _IMPORT_CHECK_FILES_FOR_umock_c "${_IMPORT_PREFIX}/lib/libumock_c.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
