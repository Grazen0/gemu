find_package(unity REQUIRED CONFIG REQUIRED)
find_package(cJSON REQUIRED CONFIG REQUIRED)

file(GLOB_RECURSE test_sources CONFIGURE_DEPENDS ./test_*.c)

file(COPY data DESTINATION .)

# Generate test runners for each test file
foreach(test_source ${test_sources})
  get_filename_component(test_name ${test_source} NAME_WE)
  set(runner_source "${CMAKE_CURRENT_BINARY_DIR}/${test_name}_runner.c")
  set(test_exec "gemu_${test_name}")

  add_custom_command(
    OUTPUT ${runner_source}
    COMMAND
      ${CMAKE_COMMAND} -E env ruby
      ${PROJECT_SOURCE_DIR}/tools/generate_test_runner.rb ${test_source}
      ${runner_source}
    DEPENDS ${test_source}
    COMMENT "Generating Unity test runner for ${test_source}")

  add_executable(${test_exec} ${test_source} ${runner_source})
  target_link_libraries(${test_exec} PRIVATE gemu_lib)
  target_link_libraries(${test_exec} PRIVATE unity::framework)
  target_link_libraries(${test_exec} PRIVATE cjson)

  add_test(NAME ${test_name} COMMAND ${test_exec})
endforeach()
