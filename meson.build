project('jzip', 'cpp',
  default_options : ['cpp_std=c++14', 'warning_level=3'])

zdep = dependency('zlib')
threaddep = dependency('threads')

executable('junzip',
  'junzip.cpp', 'zipfile.cpp', 'zimp.cpp',
  dependencies : [zdep, threaddep]
)
