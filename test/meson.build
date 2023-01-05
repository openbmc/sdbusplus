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
    'utility/tuple_to_array',
    'utility/type_traits',
]

if (cxx.get_id() != 'clang')
  tests += [
    'async/context',
    'async/task',
    'async/timer',
  ]
endif

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

generated_sources = []
generated_others = []
subdir('gen')
subdir('gen/server')

server_test_pre = declare_dependency(
    include_directories: include_directories('gen'),
    dependencies: sdbusplus_dep)

server_test_lib = static_library(
    'server-test',
    generated_sources,
    implicit_include_directories: false,
    dependencies: server_test_pre)

generated_sources_header = []
foreach s : generated_sources
    foreach f : s.to_list()
        if f.full_path().endswith('.hpp')
            generated_sources_header += f
        endif
    endforeach
endforeach

server_test_dep = declare_dependency(
  sources: generated_sources_header,
  link_with: server_test_lib,
  dependencies: server_test_pre)

test(
    'test-server',
    executable(
        'test-server',
        'server/object.cpp',
        dependencies: [
            gmock_dep,
            gtest_dep,
            server_test_dep,
        ],
    ),
)

test(
    'test-server-message-variant',
    executable(
        'test-server-message-variant',
        'server/message_variant.cpp',
        dependencies: [
            gmock_dep,
            gtest_dep,
            server_test_dep,
        ],
    ),
)
