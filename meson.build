project('lalr_parsergen', 'cpp', default_options : ['cpp_std=c++11'])


add_global_arguments('-std=c++11', language : 'cpp')


inc_dirs = [
    include_directories('../src'),
    include_directories('../external'),
    include_directories('../external/paw_print'),
]

srcs = run_command('python3', 'find_src.py', 'src').stdout().strip().split('\n')
lalr_parsergen_lib = static_library('lalr_parsergen', srcs, include_directories : inc_dirs)

srcs = run_command('python3', 'find_src.py', 'external').stdout().strip().split('\n')
srcs += ['main.cpp']

executable('test_lalr_parsergen', srcs, link_with : lalr_parsergen_lib, include_directories : inc_dirs)

