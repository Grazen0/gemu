cmake_minimum_required(VERSION 3.11)
project(gemu C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(SDL3 REQUIRED CONFIG REQUIRED)

# Set default build type to Debug
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      Debug
      CACHE STRING "Build type" FORCE)
endif()

set(gemu_sources
    src/control.c
    src/cpu.c
    src/frontend.c
    src/game_boy.c
    src/instructions.c
    src/log.c
    src/num.c
    src/sdl.c)

add_library(gemu_lib ${gemu_sources})
target_include_directories(gemu_lib PUBLIC src)
target_link_libraries(gemu_lib PRIVATE argparse_static)

if(TARGET SDL3::SDL3-static)
  message(STATUS "Using static SDL3")
  target_link_libraries(gemu_lib PRIVATE SDL3::SDL3-static)
elseif(TARGET SDL3::SDL3-shared)
  message(STATUS "Using shared SDL3")
  target_link_libraries(gemu_lib PRIVATE SDL3::SDL3-shared)
else()
  message(FATAL_ERROR "Neither SDL3::SDL3-static nor SDL3::SDL3 found")
endif()

add_executable(gemu src/main.c)
target_link_libraries(gemu PRIVATE gemu_lib)

install(TARGETS gemu RUNTIME DESTINATION bin)

include(CTest)
if(GEMU_BUILD_TESTING AND BUILD_TESTING)
  add_subdirectory(tests)
endif()
