project('parzip', 'cpp',
  version : '1.1',
  license : 'GPLv3+',
  default_options : ['cpp_std=c++14', 'warning_level=3'])

zdep = dependency('zlib', fallback : ['zlib', 'zlib_dep'])
lzmadep = dependency('liblzma', fallback : ['liblzma', 'lzma_dep'       ])
threaddep = dependency('threads')

subdir('man')
subdir('src')
