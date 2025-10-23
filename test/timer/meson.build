timer_tests = [
    'timer_callback_done',
    'timer_callback_not_done',
    'timer_expire',
    'timer_not_expire',
    'timer_update_expire',
    'timer_update_not_expire',
]

foreach t : timer_tests
    test(
        'test_' + t.underscorify(),
        executable(
            'test-' + t.underscorify(),
            t + '.cpp',
            dependencies: [gtest_dep, gmock_dep, sdbusplus_dep],
        ),
    )
endforeach
