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

add_executable(resonance_shot Source/main.cpp)
target_link_libraries(resonance_shot PRIVATE SDL3::SDL3)
target_compile_definitions(resonance_shot PRIVATE RESONANCE_VERSION="${PROJECT_VERSION}")
target_compile_options(resonance_shot PRIVATE -Wall -Wextra -Wpedantic -Werror)

include(CTest)
if(BUILD_TESTING)
  add_test(NAME resonance_shot_prints_greeting COMMAND resonance_shot)
  set_tests_properties(resonance_shot_prints_greeting PROPERTIES PASS_REGULAR_EXPRESSION
                                                                 "Hello World" TIMEOUT 5)
endif()
