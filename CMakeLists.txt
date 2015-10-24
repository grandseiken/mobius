cmake_minimum_required(VERSION 2.6)
# Set output directories.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
# Add dependencies.
add_subdirectory(dependencies)

set(BLENDER_PATH blender CACHE STRING
    "Path to blender for exporting assets")
if(NOT MSVC)
 set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -std=c++11)
endif()

set(GENFILES_DIRECTORY "${CMAKE_BINARY_DIR}/gen")
set(MOBIUS_TOOL_OUTPUTS "${GENFILES_DIRECTORY}/tools/simplex_lut.h")
set(MOBIUS_PROTO_OUTPUTS
  "${GENFILES_DIRECTORY}/mobius.pb.cc"
  "${GENFILES_DIRECTORY}/mobius.pb.h")
file(GLOB MOBIUS_SHADER_INCLUDE_FILES
     RELATIVE ${CMAKE_SOURCE_DIR} "src/shaders/*.glsl.h")
file(GLOB MOBIUS_SHADER_FILES
     RELATIVE "${CMAKE_SOURCE_DIR}/src" "src/shaders/*.glsl")
file(GLOB MOBIUS_SOURCE_FILES
     RELATIVE ${CMAKE_SOURCE_DIR} "src/*.cc" "src/*.h")
file(GLOB MOBIUS_BLEND_FILES
     RELATIVE "${CMAKE_SOURCE_DIR}/assets" "assets/*.world.blend")

add_executable(simplex_lut EXCLUDE_FROM_ALL src/tools/simplex_lut.cc)
add_custom_command(
  OUTPUT "${GENFILES_DIRECTORY}/tools/simplex_lut.h"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${GENFILES_DIRECTORY}/tools"
  COMMAND simplex_lut > "${GENFILES_DIRECTORY}/tools/simplex_lut.h"
  DEPENDS simplex_lut VERBATIM)

add_custom_command(
  OUTPUT ${MOBIUS_PROTO_OUTPUTS}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GENFILES_DIRECTORY}
  COMMAND protoc src/mobius.proto
  --proto_path=src --cpp_out=${GENFILES_DIRECTORY}
  VERBATIM)

set(MOBIUS_DATA_OUTPUTS
  "${GENFILES_DIRECTORY}/data/demo.world.pb"
  "${GENFILES_DIRECTORY}/data/player.mesh.pb")
add_custom_command(
  OUTPUT "${GENFILES_DIRECTORY}/data/demo.world.pb"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${GENFILES_DIRECTORY}/data"
  COMMAND cat src/data/demo.world.pb | bin/protoc --proto_path=src
          --encode=mobius.proto.world src/mobius.proto >
          "${GENFILES_DIRECTORY}/data/demo.world.pb"
  DEPENDS protoc src/data/demo.world.pb VERBATIM)
add_custom_command(
  OUTPUT "${GENFILES_DIRECTORY}/data/player.mesh.pb"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${GENFILES_DIRECTORY}/data"
  COMMAND cat src/data/player.mesh.pb | bin/protoc --proto_path=src
          --encode=mobius.proto.mesh src/mobius.proto >
          "${GENFILES_DIRECTORY}/data/player.mesh.pb"
  DEPENDS protoc src/data/player.mesh.pb VERBATIM)

foreach(BLEND_FILE ${MOBIUS_BLEND_FILES})
  string(REGEX REPLACE "\\.[^.]*$" "" BARENAME ${BLEND_FILE})
  set(INPUT_FILE "assets/${BLEND_FILE}")
  set(INTERMEDIATE_FILE "${GENFILES_DIRECTORY}/${BLEND_FILE}.pb")
  set(OUTPUT_FILE "${GENFILES_DIRECTORY}/data/${BARENAME}.pb")
  list(APPEND MOBIUS_DATA_OUTPUTS ${OUTPUT_FILE})

  add_custom_command(
    OUTPUT ${OUTPUT_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GENFILES_DIRECTORY}/data"
    COMMAND EXPORT_PATH=${INTERMEDIATE_FILE} ${BLENDER_PATH}
            ${INPUT_FILE} --background --python assets/export.py
    COMMAND cat ${INTERMEDIATE_FILE} | bin/protoc --proto_path=src
            --encode=mobius.proto.world src/mobius.proto > ${OUTPUT_FILE}
    DEPENDS ${INPUT_FILE} assets/export.py VERBATIM)
endforeach()

set(MOBIUS_SHADER_OUTPUTS "")
foreach(SHADER_FILE ${MOBIUS_SHADER_FILES})
  set(INPUT_FILE "src/${SHADER_FILE}")
  set(INTERMEDIATE_FILE "gen/${SHADER_FILE}")
  set(OUTPUT_FILE "${GENFILES_DIRECTORY}/${SHADER_FILE}.h")
  list(APPEND MOBIUS_SHADER_OUTPUTS ${OUTPUT_FILE})

  add_custom_command(
    OUTPUT ${OUTPUT_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GENFILES_DIRECTORY}/shaders"
    COMMAND echo "#version 330\\n" > ${INTERMEDIATE_FILE}
    COMMAND cpp ${INPUT_FILE} >> ${INTERMEDIATE_FILE}
    COMMAND xxd -i ${INTERMEDIATE_FILE} > ${OUTPUT_FILE}
    DEPENDS ${INPUT_FILE} ${MOBIUS_SHADER_INCLUDE_FILES} VERBATIM)
endforeach()

find_package(GLEW REQUIRED)
add_executable(
  mobius
  ${MOBIUS_SOURCE_FILES}
  ${MOBIUS_TOOL_OUTPUTS}
  ${MOBIUS_PROTO_OUTPUTS}
  ${MOBIUS_SHADER_OUTPUTS}
  ${MOBIUS_DATA_OUTPUTS})
add_dependencies(mobius mobius_data)
target_link_libraries(
  mobius ${GLEW_LIBRARIES}
  libprotobuf sfml-audio sfml-graphics sfml-window sfml-system)
target_include_directories(
  mobius PRIVATE SYSTEM ${GENFILES_DIRECTORY}
  dependencies/glm dependencies/protobuf/src dependencies/sfml/include)
