project('lalr_parsergen', 'cpp', default_options : ['cpp_std=c++11'])


add_global_arguments('-std=c++11', language : 'cpp')


inc_dirs = []

srcs = run_command('python3', 'find_src.py', 'src').stdout().strip().split('\n')
srcs += ['main.cpp']

executable('lalr_parsergen', srcs, include_directories : inc_dirs)

