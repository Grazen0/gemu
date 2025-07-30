cmake_minimum_required(VERSION 3.16)
project(gemu C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)

# Set default build type to Debug
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      Debug
      CACHE STRING "Build type" FORCE)
endif()

file(GLOB_RECURSE sources_common CONFIGURE_DEPENDS src/common/*.c)
add_library(gemu_common STATIC ${sources_common})
target_include_directories(gemu_common PUBLIC src)

file(GLOB_RECURSE sources_core CONFIGURE_DEPENDS src/core/*.c)
add_library(gemu_core STATIC ${sources_core})
target_include_directories(gemu_core PUBLIC src)
target_link_libraries(gemu_core PRIVATE gemu_common)

set(boot_rom
    "${PROJECT_SOURCE_DIR}/data/bootix_dmg.bin"
    CACHE FILEPATH "Path to boot ROM file")
add_custom_command(
  OUTPUT boot_rom.c
  COMMAND ${CMAKE_COMMAND} -E env xxd -i -n BOOT_ROM ${boot_rom} > boot_rom.c
  DEPENDS ${boot_rom}
  COMMENT "Dumping ${boot_rom}")
add_library(boot_rom STATIC boot_rom.c)

file(GLOB_RECURSE sources_frontend CONFIGURE_DEPENDS src/frontend/*.c)
add_executable(gemu ${sources_frontend})
target_link_libraries(gemu PRIVATE SDL3::SDL3)
target_link_libraries(gemu PRIVATE gemu_common)
target_link_libraries(gemu PRIVATE gemu_core)
target_link_libraries(gemu PRIVATE boot_rom)
target_link_libraries(gemu PRIVATE m)

option(BUILD_TESTING "Build unit tests" OFF)
include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
