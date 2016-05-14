zl = static_library('parzcore',
  'zipfile.cpp',
  'compress.cpp',
  'decompress.cpp',
  'fileutils.cpp',
  'utils.cpp',
  'file.cpp',
  'mmapper.cpp',
  'zipcreator.cpp',
  dependencies : [zdep, lzmadep, threaddep]
)

executable('parunzip',
  'parunzip.cpp',
  link_with : zl,
  install : true,
  dependencies : threaddep,
)

executable('parzip',
  'parzip.cpp',
  link_with : zl,
  install : true,
  dependencies : threaddep
)

utest_exe = find_program('unziptest.py')
test('unzip test', utest_exe, args : [meson.source_root(), meson.current_build_dir()])

ctest_exe = find_program('ziptest.py')
test('zip test', ctest_exe, args : [meson.source_root(), meson.current_build_dir()])
