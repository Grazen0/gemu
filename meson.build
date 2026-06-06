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

subdir('src')
subdir('tests')

sdl3_dep = dependency('sdl3')

dependencies = [
  sdl3_dep,
  gemu_lib_dep,
]

exe = executable(
  'gemu',
  'src/main.c',
  dependencies : dependencies,
  install : true,
)
