sources = [
  'cpu.c',
  'data.c',
  'frontend/common.c',
  'game_boy.c',
  'log.c',
  'macros.c',
  'num.c',
  'scheduler.c',
]

gemu_inc = include_directories('.')

gemu_lib = static_library(
  'gemu_lib',
  sources,
  include_directories: gemu_inc,
)

gemu_lib_dep = declare_dependency(
  link_with: gemu_lib,
  include_directories: gemu_inc,
)
