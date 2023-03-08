project(
    'billchecker',
    'cpp',
    default_options :
    [
        'cpp_std=c++17'
    ]
)

if get_option('debug_prints')
    add_project_arguments('-DDEBUG_PRINTING', language : 'cpp')
endif

boost_dep = dependency(
    'boost',
    modules : ['program_options']
)

lib_hdrs_dir = include_directories(
    '/usr/local/lib/',
    '/path/to/the/csv-parser/single_include',
    '/path/to/the/rapidjson/include'
)

sources = [
    'billchecker.cpp',
    'main.cpp'
]

executable(
    'billchecker',
    sources,
    dependencies: boost_dep,
    include_directories: lib_hdrs_dir
)
