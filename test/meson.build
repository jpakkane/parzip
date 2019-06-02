bq_test = executable('bytequeue_test', 'bytequeue_test.cpp',
    include_directories: '../src',
    dependencies: threaddep)
    
test('bytequeue_test', bq_test)


utest_exe = find_program('unziptest.py')
test('unzip test', utest_exe, args : [meson.source_root(), meson.current_build_dir() / '../src'])

ctest_exe = find_program('ziptest.py')
test('zip test', ctest_exe, args : [meson.source_root(), meson.current_build_dir() / '../src'])
