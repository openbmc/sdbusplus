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

subdir('async')
subdir('timer')

tests = [
    'bus/exception',
    'bus/list_names',
    'bus/match',
    'event/event',
    'exception/sdbus_error',
    'message/append',
    'message/call',
    'message/flat_map_test',
    'message/native_types',
    'message/read',
    'message/types',
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

sdbusplus_current_path = 'server/Test'

generated_headers += custom_target(
    'common/full_tree'.underscorify(),
    input: [
        'yaml/server/Test.interface.yaml',
        'yaml/server/Test/Nested.interface.yaml',
        'yaml/server/Test/Nested/Nested2.interface.yaml',
        'yaml/server/Test2.interface.yaml',
        'yaml/server/Test3.interface.yaml',
        'yaml/server/TestWithMethod.interface.yaml',
    ],
    output: [
        'full_tree.hpp',
    ],
    depend_files: sdbusplusplus_depfiles,
    command: [
        sdbuspp_gen_meson_prog,
        '--command',
        'tree',
        '--output',
        meson.current_build_dir(),
        '--tool',
        sdbusplusplus_prog,
        '--directory',
        meson.current_source_dir() / 'yaml',
    ],
    install: should_generate_cpp,
    install_dir: [
        get_option('includedir') / sdbusplus_current_path,
    ],
    build_by_default: should_generate_cpp,
)

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
    'test_property_names',
    'test_method_names',
    'test_signal_names',
    'test_full_tree',
]

foreach t : uninit_tests
    test(
        t,
        executable(
            t,
            f'gen/@t@.cpp',
            generated_sources,
            include_directories: [root_inc],
            dependencies: [sdbusplus_dep, server_test_dep, gtest_dep],
        ),
    )
endforeach
