project('jzip', 'cpp',
  default_options : ['cpp_std=c++14', 'warning_level=3'])

zdep = dependency('zlib')
lzmadep = dependency('liblzma')
threaddep = dependency('threads')

executable('junzip',
  'junzip.cpp', 'zipfile.cpp', 'zimp.cpp', 'fileutils.cpp',
  'utils.cpp', 'file.cpp',
  dependencies : [zdep, lzmadep, threaddep]
)

test_exe = find_program('tester.py')
test('unzip test', test_exe, args : [meson.source_root(), meson.build_root()])

