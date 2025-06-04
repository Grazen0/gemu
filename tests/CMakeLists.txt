find_package(unity REQUIRED CONFIG REQUIRED)

file(GLOB_RECURSE test_sources CONFIGURE_DEPENDS *.c)

# Generate test runners for each test file
foreach(test_file ${test_sources})
  get_filename_component(test_name ${test_file} NAME_WE)
  set(runner_file "${CMAKE_CURRENT_BINARY_DIR}/${test_name}_runner.c")
  set(test_exec "gemu_${test_name}")

  add_custom_command(
    OUTPUT ${runner_file}
    COMMAND
      ${CMAKE_COMMAND} -E env ruby
      ${PROJECT_SOURCE_DIR}/scripts/generate_test_runner.rb ${test_file}
      ${runner_file}
    DEPENDS ${test_file}
    COMMENT "Generating Unity test runner for ${test_file}")

  add_executable(${test_exec} ${test_file} ${runner_file})
  target_link_libraries(${test_exec} PRIVATE gemu_core)
  target_link_libraries(${test_exec} PRIVATE unity::framework)
  target_include_directories(${test_exec} PRIVATE ${PROJECT_SOURCE_DIR}/src
                                                  unity)

  add_test(NAME ${test_name} COMMAND ${test_exec})
endforeach()
