cmake_minimum_required(VERSION 3.24)
project(c12_port LANGUAGES C CXX)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(PSXPORT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/psxport" CACHE PATH "psxport framework")
if(NOT EXISTS "${PSXPORT_DIR}/cmake/psxport.cmake")
  message(FATAL_ERROR "PSXPORT_DIR is unresolved; run ./run.sh with a user-supplied disc")
endif()
set(PSXPORT_BUILD_SMOKE ON CACHE BOOL "Build the framework agnosticism smoke" FORCE)
add_subdirectory("${PSXPORT_DIR}" psxport_build)
add_custom_target(c12_scaffold DEPENDS psxport_smoke)

execute_process(
  COMMAND git -C "${PSXPORT_DIR}" rev-parse HEAD
  RESULT_VARIABLE PSXPORT_REVISION_RESULT
  OUTPUT_VARIABLE PSXPORT_REVISION
  OUTPUT_STRIP_TRAILING_WHITESPACE)
string(LENGTH "${PSXPORT_REVISION}" PSXPORT_REVISION_LENGTH)
if(NOT PSXPORT_REVISION_RESULT EQUAL 0 OR NOT PSXPORT_REVISION MATCHES "^[0-9a-f]+$" OR NOT PSXPORT_REVISION_LENGTH EQUAL 40)
  message(FATAL_ERROR "cannot record psxport build provenance from ${PSXPORT_DIR}")
endif()
file(WRITE "${CMAKE_BINARY_DIR}/psxport_resolved.txt" "commit = ${PSXPORT_REVISION}\n")

# The emitter owns the source list. A cold configure intentionally exposes only provisioning
# targets; bootstrap reconfigures after emitting the user-supplied executable, then builds player.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/generated/rec_sources.cmake")
  include("${CMAKE_CURRENT_SOURCE_DIR}/generated/rec_sources.cmake")
  if(NOT GEN_REC_SRCS)
    message(FATAL_ERROR "generated/rec_sources.cmake did not declare GEN_REC_SRCS")
  endif()
  list(TRANSFORM GEN_REC_SRCS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/generated/")
  set_source_files_properties(
    ${GEN_REC_SRCS}
    PROPERTIES
      LANGUAGE CXX
      COMPILE_OPTIONS "-O1;-foptimize-sibling-calls;-fno-strict-aliasing;-fwrapv")
  add_executable(c12_port game/app/main.cpp ${GEN_REC_SRCS})
  add_dependencies(c12_port gen_gpu_shaders)
  set_target_properties(c12_port PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
  target_include_directories(c12_port PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/generated")
  target_link_libraries(c12_port PRIVATE psxport)
endif()
