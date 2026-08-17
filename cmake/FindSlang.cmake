# Find the Slang SDK without invoking slangc.
# Accepted layouts:
#   -DSLANG_ROOT=C:/sdk/slang
#   $ENV{SLANG_ROOT}
#   a CMake package exposing slang::slang or Slang::slang

find_package(slang CONFIG QUIET)
if(TARGET slang::slang AND NOT TARGET Slang::slang)
    add_library(Slang::slang ALIAS slang::slang)
endif()

if(NOT TARGET Slang::slang)
    set(_slang_roots "${SLANG_ROOT}" "$ENV{SLANG_ROOT}")
    find_path(SLANG_INCLUDE_DIR slang.h HINTS ${_slang_roots} PATH_SUFFIXES include)
    find_library(SLANG_LIBRARY NAMES slang slang.lib HINTS ${_slang_roots} PATH_SUFFIXES lib bin)
    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(Slang REQUIRED_VARS SLANG_INCLUDE_DIR SLANG_LIBRARY)
    if(Slang_FOUND)
        add_library(Slang::slang UNKNOWN IMPORTED)
        set_target_properties(Slang::slang PROPERTIES
            IMPORTED_LOCATION "${SLANG_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${SLANG_INCLUDE_DIR}")
    endif()
endif()
