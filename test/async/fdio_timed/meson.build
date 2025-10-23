
async_tests = [
    'fdio_timed_write_async_with_timeout',
    'fdio_timed_write_async_with_timeout_iterative',
    'fdio_timed_write_skip_with_timeout',
    'fdio_timed_write_sync',
    'fdio_timed_write_sync_iterative',
]

foreach t : async_tests
    test(
        'test_' + t.underscorify(),
        executable(
            'test-' + t.underscorify(),
            t + '.cpp',
            'suite.cpp',
            dependencies: [gtest_dep, gmock_dep, sdbusplus_dep],
        ),
    )
endforeach
