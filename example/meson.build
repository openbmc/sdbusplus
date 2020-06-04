executable(
    'list-users',
    'list-users.cpp',
    dependencies: sdbusplus_dep,
)

boost_dep = dependency(
    'boost',
    disabler: true,
    required: false,
    modules: [ 'coroutine' ],
)

executable(
    'asio-example',
    'asio-example.cpp',
    cpp_args: [
        '-DBOOST_ASIO_DISABLE_THREADS',
        '-DBOOST_ALL_NO_LIB',
        '-DBOOST_SYSTEM_NO_DEPRECATED',
        '-DBOOST_ERROR_CODE_HEADER_ONLY',
        '-DBOOST_COROUTINES_NO_DEPRECATION_WARNING',
    ],
    dependencies: [ boost_dep, sdbusplus_dep ],
)

calc_buildroot = meson.current_build_dir()
calc_files = files(
    run_command(
        sdbusgen_prog,
        '--tool', sdbuspp_prog,
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
