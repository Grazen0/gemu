gemu_inc = include_directories('.')

frontend = get_option('frontend')

gemu_sources = files(
  'main.c',
  'frontend/common.c',
)

gemu_dependencies = [
  gemu_core_dep
]

if frontend == 'sdl3'
  gemu_sources += files('frontend/sdl3.c')
  gemu_dependencies += dependency('sdl3')
elif frontend == 'raylib'
  gemu_sources += files('frontend/raylib.c')
  gemu_dependencies += dependency('raylib')
elif frontend == 'opengl'
  gemu_sources += files('frontend/opengl.c')
  gemu_dependencies += [dependency('gl'), dependency('glut')]
elif frontend == 'sfml'
  gemu_sources += files('frontend/sfml.c')
  gemu_dependencies += dependency('csfml-graphics')
endif
