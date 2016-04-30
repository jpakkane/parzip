executable('junzip',
  'junzip.cpp', 'zipfile.cpp', 'zimp.cpp', 'fileutils.cpp',
  'utils.cpp', 'file.cpp',
  dependencies : [zdep, lzmadep, threaddep]
)

test_exe = find_program('tester.py')
test('unzip test', test_exe, args : [meson.source_root(), meson.current_build_dir()])

