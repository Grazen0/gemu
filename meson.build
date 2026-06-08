project(
  'gemu',
  'c',
  meson_version : '>= 1.3.0',
  version : '0.1.0',
  default_options : [
    'warning_level=3',
    'c_std=c23',
    'default_library=static',
  ],
)

add_project_arguments(
  '-D_GNU_SOURCE',
  language : 'c'
)

subdir('core')
subdir('src')

tests = get_option('tests')

if tests
  subdir('tests')
endif

executable(
  'gemu',
  gemu_sources,
  include_directories : gemu_inc,
  dependencies : gemu_dependencies,
  install : true,
)
