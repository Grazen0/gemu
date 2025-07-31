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

set(boot_rom_file
    "${PROJECT_SOURCE_DIR}/data/bootix_dmg.bin"
    CACHE FILEPATH "Path to boot ROM file")
set(boot_rom_source "${PROJECT_BINARY_DIR}/boot_rom.c")

add_custom_command(
  OUTPUT ${boot_rom_source}
  COMMAND ${CMAKE_COMMAND} -E env xxd -i -n BOOT_ROM ${boot_rom_file} >
          ${boot_rom_source}
  DEPENDS ${boot_rom_file}
  COMMENT "Dumping ${boot_rom_file}")

file(GLOB_RECURSE gemu_sources CONFIGURE_DEPENDS src/*.c)

add_library(gemu_lib ${gemu_sources} ${boot_rom_source})
target_include_directories(gemu_lib PUBLIC src)
target_link_libraries(gemu_lib PRIVATE SDL3::SDL3)
target_link_libraries(gemu_lib PRIVATE m) # math.h

add_executable(gemu src/main.c)
target_link_libraries(gemu PRIVATE gemu_lib)

include(CTest)
if(GEMU_BUILD_TESTING AND BUILD_TESTING)
  add_subdirectory(tests)
endif()
