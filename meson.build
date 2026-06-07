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

sources = [
  'src/main.c',
  'src/frontend/common.c',
]

dependencies = [
  gemu_lib_dep
]

frontend = get_option('frontend')

if frontend == 'sdl3'
  sources += ['src/frontend/sdl3.c']
  dependencies += dependency('sdl3')
elif frontend == 'raylib'
  sources += ['src/frontend/raylib.c']
  dependencies += dependency('raylib')
endif

exe = executable(
  'gemu',
  sources,
  dependencies : dependencies,
  install : true,
)
