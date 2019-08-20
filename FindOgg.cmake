# - Try to find ogg
# Once done this will define
#
# OGG_FOUND - system has libogg
# OGG_INCLUDE_DIRS - the libogg include directory
# OGG_LIBRARIES - The libogg libraries

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_OGG ogg QUIET)
endif()

find_path(OGG_INCLUDE_DIRS ogg/ogg.h
                           PATHS ${PC_OGG_INCLUDEDIR})
find_library(OGG_LIBRARIES ogg
                           PATHS ${PC_OGG_LIBDIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Ogg REQUIRED_VARS OGG_INCLUDE_DIRS OGG_LIBRARIES)

mark_as_advanced(OGG_INCLUDE_DIRS OGG_LIBRARIES)
