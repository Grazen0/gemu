cmake_minimum_required(VERSION 3.16)
project(gemu C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(-Wall -Wextra -Wpedantic)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g3 -Og -ggdb3")

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)

# Set default build type to Debug
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      Debug
      CACHE STRING "Build type" FORCE)
endif()

file(GLOB_RECURSE COMMON_FILES CONFIGURE_DEPENDS src/common/*.c)
file(GLOB_RECURSE CORE_FILES CONFIGURE_DEPENDS src/core/*.c)
file(GLOB_RECURSE FRONTEND_FILES CONFIGURE_DEPENDS src/frontend/*.c)

add_library(gemu_common STATIC ${COMMON_FILES})
target_include_directories(gemu_common PUBLIC src)

add_library(gemu_core STATIC ${CORE_FILES})
target_include_directories(gemu_core PUBLIC src)
target_link_libraries(gemu_core PRIVATE gemu_common)

add_executable(gemu ${FRONTEND_FILES})
target_link_libraries(gemu PRIVATE SDL3::SDL3)
target_link_libraries(gemu PRIVATE gemu_common)
target_link_libraries(gemu PRIVATE gemu_core)

add_subdirectory(tests)
