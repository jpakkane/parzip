executable('fuzz', 'fuzz.cpp',
  include_directories : libinc,
  link_with : zl,
  link_args : ['Ws2_32.lib', '-lcomctl32'],
  dependencies : threaddep,
)

