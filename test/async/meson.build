
async_tests = ['context', 'fdio', 'mutex', 'task', 'timer', 'watchdog']

foreach t : async_tests
    test(
        'test_' + t.underscorify(),
        executable(
            'test-' + t.underscorify(),
            t + '.cpp',
            dependencies: [gtest_dep, gmock_dep, sdbusplus_dep],
        ),
    )
endforeach

subdir('fdio_timed')
