cmake_minimum_required(VERSION 3.24)
project(c12_port LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
include(CTest)
option(C12_BUILD_RUNTIME "Build the authenticated native/dynarec startup probe" ON)

if(C12_BUILD_RUNTIME)
  set(PSXPORT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/psxport" CACHE PATH "Shared psxport checkout")
  if(NOT EXISTS "${PSXPORT_DIR}/runtime/psx/psx_exe_image.h")
    message(FATAL_ERROR "C-12 requires psxport with the bounded PS-X EXE image API at ${PSXPORT_DIR}")
  endif()
  add_subdirectory("${PSXPORT_DIR}" framework EXCLUDE_FROM_ALL)

  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/title.json" C12_IDENTITY_JSON)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/title.json")
  string(JSON C12_EXECUTABLE GET "${C12_IDENTITY_JSON}" executable)
  string(JSON C12_SIZE GET "${C12_IDENTITY_JSON}" size)
  string(JSON C12_SHA256 GET "${C12_IDENTITY_JSON}" sha256)
  configure_file(game/image/title_identity.h.in metadata/title_identity.h @ONLY)

  add_library(c12_runtime STATIC
    game/image/authenticated_image.cpp
    game/runtime/c12_platform_facts.cpp
    game/runtime/c12_runtime.cpp
  )
  target_link_libraries(c12_runtime PUBLIC psxport)
  target_include_directories(c12_runtime PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/game/image"
    "${CMAKE_CURRENT_SOURCE_DIR}/game/runtime"
    "${CMAKE_CURRENT_BINARY_DIR}/metadata")
  add_executable(c12_boot_probe game/app/boot_probe.cpp)
  target_link_libraries(c12_boot_probe PRIVATE c12_runtime)
endif()

# Break-first migration: the offline emitter, generated corpus, and generated dispatcher are gone.
# The startup probe is diagnostic. Player startup remains unavailable until the title
# owns a verified presentation/field lifecycle rather than presenting guest-picture fallback.
add_custom_target(
  c12_port
  COMMAND
    "${CMAKE_COMMAND}" -E echo
    "C-12 gameplay target unavailable: title field lifecycle and native presentation are not integrated"
  COMMAND "${CMAKE_COMMAND}" -E false
  VERBATIM
)

if(BUILD_TESTING)
  add_test(
    NAME c12_source_policy
    COMMAND "${Python3_EXECUTABLE}" -B -m unittest discover -s tests -p "test_*.py"
  )
  set_tests_properties(c12_source_policy PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  if(C12_BUILD_RUNTIME)
    add_executable(c12_image_contract tests/test_authenticated_image.cpp)
    target_link_libraries(c12_image_contract PRIVATE c12_runtime)
    add_executable(c12_runtime_services tests/test_runtime_services.cpp)
    target_link_libraries(c12_runtime_services PRIVATE c12_runtime)
    add_test(NAME c12_image_contract COMMAND c12_image_contract
      "${CMAKE_CURRENT_SOURCE_DIR}/scratch/tests/c12-image-contract")
    add_test(NAME c12_runtime_services COMMAND c12_runtime_services)
    add_test(NAME c12_cpp_style COMMAND "${Python3_EXECUTABLE}"
      "${PSXPORT_DIR}/tools/check_cpp_style.py" --root "${CMAKE_CURRENT_SOURCE_DIR}"
      --compile-commands "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json")
  endif()
endif()
