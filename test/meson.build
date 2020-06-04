gtest_dep = dependency('gtest')
gtest_main_dep = dependency('gtest_main')
gmock_dep = dependency('gmock')
gmock_main_dep = dependency('gmock_main')

tests = [
    'bus/list_names',
    'bus/match',
    'exception/sdbus_error',
    'message/append',
    'message/read',
    'message/native_types',
    'message/types',
    'timer',
    'utility/tuple_to_array',
    'utility/type_traits',
]

foreach t : tests
    test(
        'test_' + t.underscorify(),
        executable(
            'test-' + t.underscorify(),
            t + '.cpp',
            dependencies: [
                gtest_dep,
                gmock_dep,
                gmock_main_dep,
                sdbusplus_dep,
            ],
        )
    )
endforeach

test(
    'test-vtable',
    executable(
        'test-vtable',
        'vtable/vtable.cpp',
        'vtable/vtable_c.c',
        dependencies: [ gtest_dep, gtest_main_dep, sdbusplus_dep ],
    ),
)

server_buildroot = meson.current_build_dir()
server_files = files(
    run_command(
        sdbusgen_prog,
        '--tool', sdbuspp_prog,
        '--output', server_buildroot,
        'server',
        check: true
    ).stdout().strip().split('\n')
)

test(
    'test-server',
    executable(
        'test-server',
        'server/object.cpp',
        server_files,
        include_directories: include_directories('server'),
        dependencies: [
            gmock_dep,
            gmock_main_dep,
            gtest_dep,
            pthread_dep,
            sdbusplus_dep,
        ],
    ),
)
