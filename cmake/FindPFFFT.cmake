find_path(
  PFFFT_INCLUDE_DIRS pffft.h
  PATHS
    ENV PFFFT_ROOT
    ENV PFFFT_INCLUDE_DIR
    ${PFFFT_ROOT}
    /usr
    /usr/local
  PATH_SUFFIXES
    ${CMAKE_INSTALL_INCLUDEDIR}
)
find_library(
  PFFFT_LIBRARIES NAMES pffft
  PATHS
    ENV PFFFT_ROOT
    ENV PFFFT_LIB_DIR
    ${PFFFT_ROOT}
    /usr
    /usr/local
  PATH_SUFFIXES
    ${CMAKE_INSTALL_LIBDIR}
)

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args(
  PFFFT
  DEFAULT_MSG
    PFFFT_INCLUDE_DIRS
    PFFFT_LIBRARIES
)

if( PFFFT_FOUND AND NOT TARGET PFFFT::PFFFT )
  add_library( PFFFT::PFFFT UNKNOWN IMPORTED )
  set_target_properties(
    PFFFT::PFFFT PROPERTIES
    IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
    IMPORTED_LOCATION "${PFFFT_LIBRARIES}"
    INTERFACE_INCLUDE_DIRECTORIES "${PFFFT_INCLUDE_DIRS}"
  )
endif()
