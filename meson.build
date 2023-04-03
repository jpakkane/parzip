project('parzip', 'cpp',
  version : '1.4.0',
  license : 'GPLv3+',
  default_options : ['cpp_std=c++17', 'warning_level=3'])

#add_global_arguments('-DWIN32_LEAN_AND_MEAN', language: 'cpp')

zdep = dependency('zlib')
if host_machine.system() != 'windows'
  lzmadep = dependency('liblzma')
  compr_deps = [zdep, lzmadep]
else
  compr_deps = [zdep]
endif
threaddep = dependency('threads')

gtkdep = dependency('gtk+-3.0', required : false)

subdir('man')
if not meson.is_cross_build() or host_machine.system() != 'windows'
  subdir('src') # Threads don't work on mingw but want to develop on Ubuntu for now.
endif

libinc = include_directories('src')

if gtkdep.found()
  subdir('gtk')
endif

if host_machine.system() == 'windows'
  subdir('windows')
endif

subdir('fuzz')
subdir('test')

