if host_machine.system() == 'windows'
  linkargs = ['Ws2_32.lib', '-lcomctl32']
else
  linkargs = []
endif

zl = static_library('parzcore',
  'zipfile.cpp',
  'compress.cpp',
  'decompress.cpp',
  'fileutils.cpp',
  'utils.cpp',
  'file.cpp',
  'mmapper.cpp',
  'zipcreator.cpp',
  'taskcontrol.cpp',
  dependencies : compr_deps + [threaddep]
)

executable('parunzip',
  'parunzip.cpp',
  link_with : zl,
  install : true,
  link_args : linkargs,
  dependencies : threaddep,
)

executable('parzip',
  'parzip.cpp',
  link_with : zl,
  install : true,
  link_args : linkargs,
  dependencies : threaddep
)
