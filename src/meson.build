zl = static_library('jzcore',
  'zipfile.cpp',
  'decompress.cpp',
  'fileutils.cpp',
  'utils.cpp',
  'file.cpp',
  'mmapper.cpp',
  'zipcreator.cpp',
  dependencies : [zdep, lzmadep, threaddep]
)

executable('junzip',
  'junzip.cpp',
  link_with : zl,
  install : true,
  dependencies : threaddep,
)

executable('jzip',
  'jzip.cpp',
  link_with : zl,
  install : true,
  dependencies : threaddep
)

test_exe = find_program('tester.py')
test('unzip test', test_exe, args : [meson.source_root(), meson.current_build_dir()])
