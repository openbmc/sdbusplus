gtest_dep = dependency('gtest', main: true, disabler: true, required: false)
gmock_dep = dependency('gmock', disabler: true, required: false)
if not gtest_dep.found() or not gmock_dep.found()
    gtest_proj = import('cmake').subproject('googletest', required: false)
    if gtest_proj.found()
        gtest_dep = declare_dependency(
            dependencies: [
                dependency('threads'),
                gtest_proj.dependency('gtest'),
                gtest_proj.dependency('gtest_main'),
            ]
        )
        gmock_dep = gtest_proj.dependency('gmock')
    else
        assert(
            not get_option('tests').enabled(),
            'Googletest is required if tests are enabled'
        )
    endif
endif

tests = [
    'bus/list_names',
    'bus/match',
    'exception/sdbus_error',
    'message/append',
    'message/call',
    'message/read',
    'message/native_types',
    'message/types',
    'timer',
    'unpack_properties',
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
                sdbusplus_dep,
            ],
        )
    )
endforeach

assert(
    not get_option('tests').enabled() or boost_dep.found(),
    'Boost is required when tests are enabled'
)

test(
    'test-bus_aio',
    executable(
        'test-bus_aio',
        'bus/aio.cpp',
        dependencies: [
            boost_dep,
            gmock_dep,
            gtest_dep,
            sdbusplus_dep,
        ],
    )
)

test(
    'test-vtable',
    executable(
        'test-vtable',
        'vtable/vtable.cpp',
        'vtable/vtable_c.c',
        dependencies: [
            gtest_dep,
            sdbusplus_dep,
        ],
    ),
)

subdir('server/Test')
server_test_lib = static_library(
    'server-test',
    server_test_sources,
    dependencies: sdbusplus_dep,
)

test(
    'test-server',
    executable(
        'test-server',
        'server/object.cpp',
        link_with: server_test_lib,
        dependencies: [
            gmock_dep,
            gtest_dep,
            sdbusplus_dep,
        ],
    ),
)

test(
    'test-server-message-variant',
    executable(
        'test-server-message-variant',
        'server/message_variant.cpp',
        link_with: server_test_lib,
        dependencies: [
            gmock_dep,
            gtest_dep,
            sdbusplus_dep,
        ],
    ),
)
