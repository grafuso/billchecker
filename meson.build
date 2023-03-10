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

rapidjson = subproject('rapidjson')
cmake = import('cmake')
csv_sub = cmake.subproject('csv')

rapidjson_dep = rapidjson.get_variable('rapidjson_dep')
csv_dep = csv_sub.dependency('csv')

sources = [
    'billchecker.cpp',
    'main.cpp'
]

executable(
    'billchecker',
    sources,
    dependencies:
    [
        boost_dep,
        rapidjson_dep,
        csv_dep
    ]
)
