sources = [
  'cpu.c',
  'data.c',
  'frontend.c',
  'game_boy.c',
  'log.c',
  'macros.c',
  'num.c',
  'scheduler.c',
]

sdl3_dep = dependency('sdl3')

dependencies = [
  sdl3_dep,
]


gemu_inc = include_directories('.')

gemu_lib = static_library(
  'gemu_lib',
  sources,
  include_directories: gemu_inc,
  dependencies : dependencies,
)

gemu_lib_dep = declare_dependency(
  link_with: gemu_lib,
  include_directories: gemu_inc,
)
