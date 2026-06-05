unity_dep = dependency('unity', modules: ['unity::framework'])
cjson_dep = dependency('libcjson')

test_sources = [
  'test_cpu.c',
  'test_cpu_opcodes.c',
  'test_num.c',
]

cc = meson.get_compiler('c')

# Copy test data into build dir
run_command(
  'cp',
  '-r',
  meson.current_source_dir() / 'data',
  meson.current_build_dir(),
  check: true
)

foreach test_source : test_sources
  test_name = test_source.split('.')[0]

  runner_source = custom_target(
    test_name + '_runner',
    input: test_source,
    output: test_name + '_runner.c',
    command: [
      'ruby',
      meson.project_source_root() / 'tools/generate_test_runner.rb',
      '@INPUT@',
      '@OUTPUT@'
    ]
  )

  test_exe = executable(
    'gemu_' + test_name,
    [
      test_source,
      runner_source
    ],
    dependencies: [
      unity_dep,
      cjson_dep,
      gemu_lib_dep,
    ],
  )

  test(test_name, test_exe)
endforeach
