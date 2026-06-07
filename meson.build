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


frontend = get_option('frontend')

frontend_sources = {
  'sdl': ['src/frontend/sdl.c', 'src/frontend/common.c'],
  'raylib': ['src/frontend/raylib.c', 'src/frontend/common.c'],
}

frontend_deps = {
  'sdl': [dependency('sdl3')],
  'raylib': [dependency('raylib')],
}

sources = ['src/main.c'] + frontend_sources[frontend]
dependencies = [gemu_lib_dep] + frontend_deps[frontend]

exe = executable(
  'gemu',
  sources,
  dependencies : dependencies,
  install : true,
)
