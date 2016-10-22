if host_machine.system() == 'windows'
  linkargs = 'Ws2_32.lib'
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

utest_exe = find_program('unziptest.py')
test('unzip test', utest_exe, args : [meson.source_root(), meson.current_build_dir()])

ctest_exe = find_program('ziptest.py')
test('zip test', ctest_exe, args : [meson.source_root(), meson.current_build_dir()])
