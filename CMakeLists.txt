cmake_minimum_required(VERSION 3.25)

project(
  ResonanceShot
  VERSION 0.1.0
  LANGUAGES CXX)

if(NOT APPLE)
  message(FATAL_ERROR "Resonance Shot currently supports Apple Silicon macOS only")
endif()
if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$")
  message(FATAL_ERROR "Resonance Shot requires an arm64 build")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_OSX_ARCHITECTURES
    arm64
    CACHE STRING "Product architecture" FORCE)
set(CMAKE_OSX_DEPLOYMENT_TARGET
    "26.0"
    CACHE STRING "Minimum macOS version")

include(cmake/Dependencies.cmake)

set(FLOOR_SHADER_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Floor.metal")
set_property(
  DIRECTORY
  APPEND
  PROPERTY CMAKE_CONFIGURE_DEPENDS "${FLOOR_SHADER_FILE}")
file(READ "${FLOOR_SHADER_FILE}" FLOOR_SHADER_SOURCE)
configure_file(cmake/FloorShader.hpp.in generated/FloorShader.hpp @ONLY)

add_library(resonance_math STATIC Source/Math.cpp)
target_include_directories(resonance_math PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Source")
target_compile_options(resonance_math PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_executable(resonance_shot Source/GpuWindow.cpp Source/SdlRuntime.cpp Source/main.cpp)
target_include_directories(resonance_shot PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_link_libraries(resonance_shot PRIVATE resonance_math SDL3::SDL3)
target_compile_definitions(resonance_shot PRIVATE RESONANCE_VERSION="${PROJECT_VERSION}")
target_compile_options(resonance_shot PRIVATE -Wall -Wextra -Wpedantic -Werror)

include(CTest)
if(BUILD_TESTING)
  FetchContent_MakeAvailable(Catch2)
  list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
  include(Catch)

  add_executable(resonance_math_tests Tests/MathTests.cpp)
  target_link_libraries(resonance_math_tests PRIVATE resonance_math Catch2::Catch2WithMain)
  target_compile_options(resonance_math_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
  catch_discover_tests(resonance_math_tests)

  find_program(XCRUN_EXECUTABLE xcrun REQUIRED)
  add_test(
    NAME resonance_shot_compiles_floor_shader
    COMMAND
      "${XCRUN_EXECUTABLE}" metal
      "-fmodules-cache-path=${CMAKE_CURRENT_BINARY_DIR}/MetalModuleCache" -c "${FLOOR_SHADER_FILE}"
      -o "${CMAKE_CURRENT_BINARY_DIR}/Floor.air")
endif()
