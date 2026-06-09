gemu_core_sources = [
  'cart.c',
  'cpu.c',
  'game_boy.c',
  'log.c',
  'macros.c',
  'mapper.c',
  'scheduler.c',
  'util.c',
]

gemu_core_inc = include_directories('.')

gemu_core = library(
  'gemu_core',
  gemu_core_sources,
  include_directories : gemu_core_inc,
)

gemu_core_dep = declare_dependency(
  link_with : gemu_core,
  include_directories : gemu_core_inc,
)
