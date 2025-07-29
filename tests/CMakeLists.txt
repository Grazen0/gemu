find_package(unity REQUIRED CONFIG REQUIRED)
find_package(cJSON REQUIRED CONFIG REQUIRED)

file(GLOB_RECURSE test_sources CONFIGURE_DEPENDS */test_*.c)

file(COPY data DESTINATION .)

# Generate test runners for each test file
foreach(test_file ${test_sources})
  get_filename_component(test_name ${test_file} NAME_WE)
  set(test_exec "gemu_${test_name}")

  add_executable(${test_exec} ${test_file})
  target_link_libraries(${test_exec} PRIVATE gemu_core)
  target_link_libraries(${test_exec} PRIVATE unity::framework)
  target_link_libraries(${test_exec} PRIVATE cjson)

  add_test(NAME ${test_name} COMMAND ${test_exec})
endforeach()
