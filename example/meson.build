executable(
    'list-users',
    'list-users.cpp',
    dependencies: sdbusplus_dep,
)

if boost_dep.found()
    executable(
        'asio-example',
        'asio-example.cpp',
        cpp_args: [
            '-DBOOST_ALL_NO_LIB',
            '-DBOOST_SYSTEM_NO_DEPRECATED',
            '-DBOOST_ERROR_CODE_HEADER_ONLY',
            '-DBOOST_COROUTINES_NO_DEPRECATION_WARNING',
        ],
        dependencies: [ boost_dep, pthread_dep, sdbusplus_dep ],
    )
endif

calc_buildroot = meson.current_build_dir()
calc_files = files(
    run_command(
        sdbusgen_prog,
        '--tool', sdbusplusplus_prog,
        '--output', calc_buildroot,
        'net',
        check: true
    ).stdout().strip().split('\n')
)

executable(
    'calculator-server',
    'calculator-server.cpp',
    calc_files,
    dependencies: sdbusplus_dep,
)
