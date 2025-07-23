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
            ],
        )
        gmock_dep = gtest_proj.dependency('gmock')
    else
        assert(
            not get_option('tests').enabled(),
            'Googletest is required if tests are enabled',
        )
    endif
endif

tests = [
    'async/context',
    'async/fdio',
    'async/fdio_timed',
    'async/task',
    'async/timer',
    'async/watchdog',
    'bus/exception',
    'bus/list_names',
    'bus/match',
    'event/event',
    'exception/sdbus_error',
    'message/append',
    'message/call',
    'message/native_types',
    'message/read',
    'message/types',
    'timer',
    'unpack_properties',
    'utility/make_dbus_args_tuple',
    'utility/tuple_to_array',
    'utility/type_traits',
]

foreach t : tests
    test(
        'test_' + t.underscorify(),
        executable(
            'test-' + t.underscorify(),
            t + '.cpp',
            dependencies: [gtest_dep, gmock_dep, sdbusplus_dep],
        ),
    )
endforeach

assert(
    not get_option('tests').allowed() or boost_dep.found(),
    'Boost is required when tests are enabled',
)

test(
    'test-bus_aio',
    executable(
        'test-bus_aio',
        'bus/aio.cpp',
        dependencies: [boost_dep, gmock_dep, gtest_dep, sdbusplus_dep],
    ),
)

test(
    'test-vtable',
    executable(
        'test-vtable',
        'vtable/vtable.cpp',
        'vtable/vtable_c.c',
        dependencies: [gtest_dep, sdbusplus_dep],
    ),
)

yaml_selected_subdirs = ['server']
subdir('gen')

server_test_pre = declare_dependency(
    include_directories: include_directories('gen'),
    dependencies: sdbusplus_dep,
)

server_test_lib = static_library(
    'server-test',
    generated_sources,
    implicit_include_directories: false,
    dependencies: server_test_pre,
)

server_test_dep = declare_dependency(
    sources: generated_headers,
    link_with: server_test_lib,
    dependencies: server_test_pre,
)

test(
    'test-server',
    executable(
        'test-server',
        'server/object.cpp',
        dependencies: [gmock_dep, gtest_dep, server_test_dep],
    ),
)

test(
    'test-server-message-variant',
    executable(
        'test-server-message-variant',
        'server/message_variant.cpp',
        dependencies: [gmock_dep, gtest_dep, server_test_dep],
    ),
)

uninit_tests = [
    'test_server_no_uninitialized_value_constructor',
    'test_aserver_no_uninitialized_value_constructor',
    'test_aserver_emit_interfaces_added_signal',
    'test_aserver_multiple_interfaces',
]

foreach t : uninit_tests
    test(
        t,
        executable(
            t,
            f'gen/@t@.cpp',
            generated_sources,
            include_directories: [root_inc],
            dependencies: [sdbusplus_dep, server_test_dep],
        ),
    )
endforeach
