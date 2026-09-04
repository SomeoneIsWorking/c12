cmake_minimum_required(VERSION 3.24)
project(c12_port LANGUAGES C CXX)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
include(CTest)

# Break-first migration: the offline emitter, generated corpus, and generated dispatcher are gone.
# Keep one explicit failure until psxport connects Lightrec and the title wires its typed-exit runner.
add_custom_target(
  c12_port
  COMMAND
    "${CMAKE_COMMAND}" -E echo
    "C-12 gameplay target unavailable: Lightrec backend and title typed-exit runner are not integrated"
  COMMAND "${CMAKE_COMMAND}" -E false
  VERBATIM
)

if(BUILD_TESTING)
  add_test(
    NAME c12_source_policy
    COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_source_policy.py"
  )
endif()
