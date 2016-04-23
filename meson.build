project('jzip', 'cpp', default_options : ['cpp_std=c++14', 'warnlevel=3'])

zdep = dependency('zlib')

executable('junzip',
  'junzip.cpp', 'zipfile.cpp', 'zimp.cpp',
  dependencies : zdep)

