cmake_minimum_required(VERSION 2.6)
project(mobius)

# Enable (fairly) strict warnings.
if(MSVC)
  set(CPREPROCESSOR_FLAG /EP)
  add_definitions(/W3 /WX /wd4244 /wd4267 -D_SCL_SECURE_NO_WARNINGS)
else()
  set(CPREPROCESSOR_FLAG -E)
  add_definitions(-Wall -Wextra -Werror -pedantic)
endif()

# Set output directories.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(GENFILES_DIRECTORY "${CMAKE_BINARY_DIR}/gen")
# Add dependencies.
add_subdirectory(dependencies)

set(BLENDER_PATH blender CACHE STRING
    "Path to blender for exporting assets")
if(NOT MSVC)
 set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -std=c++11)
endif()

# Directories and globs.
file(GLOB MOBIUS_SHADER_INCLUDE_FILES
     RELATIVE ${CMAKE_SOURCE_DIR} "src/shaders/*.glsl.h")
file(GLOB MOBIUS_SHADER_FILES
     RELATIVE "${CMAKE_SOURCE_DIR}/src" "src/shaders/*.glsl")
file(GLOB MOBIUS_SOURCE_FILES
     RELATIVE ${CMAKE_SOURCE_DIR} "src/*.cc" "src/*.h")
file(GLOB MOBIUS_BLEND_FILES
     RELATIVE "${CMAKE_SOURCE_DIR}/assets" "assets/*.world.blend")

# Embed tool.
add_executable(embed EXCLUDE_FROM_ALL src/tools/embed.cc)

# Tool-generated source files.
set(MOBIUS_TOOL_OUTPUTS "${GENFILES_DIRECTORY}/tools/simplex_lut.h")
add_executable(simplex_lut EXCLUDE_FROM_ALL src/tools/simplex_lut.cc)
add_custom_command(
  OUTPUT ${MOBIUS_TOOL_OUTPUTS}
  COMMAND ${CMAKE_COMMAND} -E make_directory "${GENFILES_DIRECTORY}/tools"
  COMMAND simplex_lut > ${MOBIUS_TOOL_OUTPUTS}
  DEPENDS simplex_lut VERBATIM)

# Shader-generated source files.
set(MOBIUS_SHADER_OUTPUTS "")
foreach(SHADER_FILE ${MOBIUS_SHADER_FILES})
  set(INPUT_FILE "${CMAKE_SOURCE_DIR}/src/${SHADER_FILE}")
  set(INTERMEDIATE_FILE "gen/${SHADER_FILE}")
  set(OUTPUT_FILE "${GENFILES_DIRECTORY}/${SHADER_FILE}.h")
  list(APPEND MOBIUS_SHADER_OUTPUTS ${OUTPUT_FILE})

  add_custom_command(
    OUTPUT ${OUTPUT_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GENFILES_DIRECTORY}/shaders"
    COMMAND ${CMAKE_COMMAND} -E echo "#version 330" > ${INTERMEDIATE_FILE}
    COMMAND ${CMAKE_C_COMPILER} ${CPREPROCESSOR_FLAG}
            ${INPUT_FILE} >> ${INTERMEDIATE_FILE}
    COMMAND embed ${INTERMEDIATE_FILE} ${OUTPUT_FILE}
    DEPENDS embed ${INPUT_FILE} ${MOBIUS_SHADER_INCLUDE_FILES} VERBATIM)
endforeach()

# Proto-generated source files.
set(MOBIUS_PROTO_PATH "${CMAKE_SOURCE_DIR}/src")
set(MOBIUS_PROTO "${MOBIUS_PROTO_PATH}/mobius.proto")
set(MOBIUS_PROTO_OUTPUTS
  "${GENFILES_DIRECTORY}/mobius.pb.cc"
  "${GENFILES_DIRECTORY}/mobius.pb.h")
add_custom_command(
  OUTPUT ${MOBIUS_PROTO_OUTPUTS}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GENFILES_DIRECTORY}
  COMMAND protoc ${MOBIUS_PROTO}
  --proto_path=${MOBIUS_PROTO_PATH} --cpp_out=${GENFILES_DIRECTORY} VERBATIM)

# Proto-generated data files.
set(MOBIUS_DATA_OUTPUTS "")
function(proto_data INPUT OUTPUT MESSAGE_TYPE)
  set(MOBIUS_DATA_OUTPUTS ${MOBIUS_DATA_OUTPUTS} ${OUTPUT} PARENT_SCOPE)
  add_custom_command(
    OUTPUT ${OUTPUT}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GENFILES_DIRECTORY}/data"
    COMMAND protoc --proto_path=${MOBIUS_PROTO_PATH}
            --encode=mobius.proto.${MESSAGE_TYPE}
            ${MOBIUS_PROTO} < ${INPUT} > ${OUTPUT}
    DEPENDS protoc ${INPUT} ${MOBIUS_PROTO} VERBATIM)
endfunction()

proto_data("${CMAKE_SOURCE_DIR}/src/data/demo.world.pb"
           "${GENFILES_DIRECTORY}/data/demo.world.pb" world)
proto_data("${CMAKE_SOURCE_DIR}/src/data/player.mesh.pb"
           "${GENFILES_DIRECTORY}/data/player.mesh.pb" mesh)

# Blender-generated data files.
set(BLEND_EXPORT_SCRIPT "${CMAKE_SOURCE_DIR}/assets/export.py")
foreach(BLEND_FILE ${MOBIUS_BLEND_FILES})
  string(REGEX REPLACE "\\.[^.]*$" "" BARENAME ${BLEND_FILE})
  set(INPUT_FILE "${CMAKE_SOURCE_DIR}/assets/${BLEND_FILE}")
  set(INTERMEDIATE_FILE "${GENFILES_DIRECTORY}/${BLEND_FILE}.pb")
  set(OUTPUT_FILE "${GENFILES_DIRECTORY}/data/${BARENAME}.pb")
  add_custom_command(
    OUTPUT ${INTERMEDIATE_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GENFILES_DIRECTORY}
    COMMAND ${BLENDER_PATH} ${INPUT_FILE} --background
            --python ${BLEND_EXPORT_SCRIPT} -- ${INTERMEDIATE_FILE})
  proto_data(${INTERMEDIATE_FILE} ${OUTPUT_FILE} world)
endforeach()

add_executable(mobius
  ${MOBIUS_SOURCE_FILES} ${MOBIUS_TOOL_OUTPUTS} ${MOBIUS_PROTO_OUTPUTS}
  ${MOBIUS_SHADER_OUTPUTS} ${MOBIUS_DATA_OUTPUTS})
target_compile_definitions(mobius PRIVATE -DSFML_STATIC -DGLEW_STATIC)
target_link_libraries(
  mobius PRIVATE libprotobuf libglew_static
  sfml-audio sfml-graphics sfml-window sfml-system)
target_include_directories(
  mobius SYSTEM PRIVATE ${GENFILES_DIRECTORY}
  dependencies/glm dependencies/protobuf/src
  dependencies/glew-cmake/include dependencies/sfml/include)
