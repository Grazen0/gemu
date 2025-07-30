find_package(unity REQUIRED CONFIG REQUIRED)
find_package(cJSON REQUIRED CONFIG REQUIRED)

file(GLOB_RECURSE test_sources CONFIGURE_DEPENDS */test_*.c)
file(GLOB_RECURSE test_helpers_sources CONFIGURE_DEPENDS helpers/*.c)

file(COPY data DESTINATION .)

# Generate test runners for each test file
foreach(test_file ${test_sources})
  get_filename_component(test_name ${test_file} NAME_WE)
  set(runner_file "${CMAKE_CURRENT_BINARY_DIR}/${test_name}_runner.c")
  set(test_exec "gemu_${test_name}")

  add_custom_command(
    OUTPUT ${runner_file}
    COMMAND
      ${CMAKE_COMMAND} -E env ruby
      ${PROJECT_SOURCE_DIR}/tools/generate_test_runner.rb ${test_file}
      ${runner_file}
    DEPENDS ${test_file}
    COMMENT "Generating Unity test runner for ${test_file}")

  add_executable(${test_exec} ${test_file} ${runner_file}
                              ${test_helpers_sources})
  target_link_libraries(${test_exec} PRIVATE gemu_core)
  target_link_libraries(${test_exec} PRIVATE unity::framework)
  target_link_libraries(${test_exec} PRIVATE cjson)

  add_test(NAME ${test_name} COMMAND ${test_exec})
endforeach()
