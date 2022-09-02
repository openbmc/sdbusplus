calculator_sources = []

calculator_sources += custom_target(
    'Calculator__cpp',
    input: '../Calculator.interface.yaml',
    output: [ 'server.cpp', 'server.hpp', 'client.hpp' ],
    depend_files: sdbusplusplus_depfiles,
    command: [
        sdbuspp_gen_meson_prog, '--command', 'cpp',
        '--output', meson.current_build_dir(),
        '--tool', sdbusplusplus_prog,
        '--directory', meson.current_source_dir() / '../../..',
        'net/poettering/Calculator',
    ],
)

calculator_sources += custom_target(
    'Calculator__errors',
    input: '../Calculator.errors.yaml',
    output: [ 'error.cpp', 'error.hpp' ],
    depend_files: sdbusplusplus_depfiles,
    command: [
        sdbuspp_gen_meson_prog, '--command', 'cpp',
        '--output', meson.current_build_dir(),
        '--tool', sdbusplusplus_prog,
        '--directory', meson.current_source_dir() / '../../..',
        'net/poettering/Calculator',
    ],
)
