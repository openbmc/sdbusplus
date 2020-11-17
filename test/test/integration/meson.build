executable(
    'private_bus_runner',
    'private_bus_runner.cpp',
    dependencies: [
        sdbusplus_dep,
    ]
)

mockServiceExample_buildroot = meson.current_build_dir()
mockServiceExample_files = files(
    run_command(
        sdbusgen_prog,
        '--tool', sdbusplusplus_prog,
        '--output', mockServiceExample_buildroot,
        '--gen-mocks',
        'net',
        check: true
    ).stdout().strip().split('\n')
)

test(
    'itest-daemon_manager',
    executable(
        'itest-daemon_manager',
        'daemon_manager.cpp',
        dependencies: [
            gmock_dep,
            gtest_dep,
            sdbusplus_dep
        ],
    )
)

test(
    'itest-private_bus',
    executable(
        'itest-private_bus',
        'private_bus.cpp',
        dependencies: [
            gmock_dep,
            gtest_dep,
            sdbusplus_dep
        ],
    )
)

test(
    'itest-mock_service',
    executable(
        'itest-mock_service',
        'mock_service.cpp',
        mockServiceExample_files,
        dependencies: [
            gmock_dep,
            gtest_dep,
            sdbusplus_dep
        ],
    )
)
