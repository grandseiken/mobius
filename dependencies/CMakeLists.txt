if(MSVC)
  add_definitions(/W0)
else()
  add_definitions(-w)
endif()

# Protobuf.
set(BUILD_TESTING OFF CACHE BOOL "")
add_subdirectory(protobuf/cmake EXCLUDE_FROM_ALL)

# GLEW.
set(glew-cmake_BUILD_STATIC ON CACHE BOOL "")
add_subdirectory(glew-cmake EXCLUDE_FROM_ALL)

# SFML.
set(BUILD_SHARED_LIBS FALSE CACHE BOOL "")
set(SFML_BUILD_DOC FALSE CACHE BOOL "")
set(SFML_BUILD_EXAMPLES FALSE CACHE BOOL "")
set(SFML_INSTALL_PKGCONFIG_FILES FALSE CACHE BOOL "")
add_subdirectory(sfml EXCLUDE_FROM_ALL)
