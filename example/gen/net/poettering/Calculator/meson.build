# Generated file; do not modify.

sdbusplus_current_path = 'net/poettering/Calculator'

all_yaml_files += files(
    '../../../../yaml/net/poettering/Calculator.events.yaml',
    '../../../../yaml/net/poettering/Calculator.interface.yaml',
)

generated_sources += custom_target(
    'net/poettering/Calculator__cpp'.underscorify(),
    input: [
        '../../../../yaml/net/poettering/Calculator.events.yaml',
        '../../../../yaml/net/poettering/Calculator.interface.yaml',
    ],
    output: [
        'event.cpp',
        'event.hpp',
        'common.hpp',
        'server.hpp',
        'server.cpp',
        'aserver.hpp',
        'client.hpp',
    ],
    depend_files: sdbusplusplus_depfiles,
    command: [
        sdbuspp_gen_meson_prog,
        '--command',
        'cpp',
        '--output',
        meson.current_build_dir(),
        '--tool',
        sdbusplusplus_prog,
        '--directory',
        meson.current_source_dir() / '../../../../yaml',
        'net/poettering/Calculator',
    ],
    install: should_generate_cpp,
    install_dir: [
        false,
        get_option('includedir') / sdbusplus_current_path,
        get_option('includedir') / sdbusplus_current_path,
        get_option('includedir') / sdbusplus_current_path,
        false,
        get_option('includedir') / sdbusplus_current_path,
        get_option('includedir') / sdbusplus_current_path,
    ],
    build_by_default: should_generate_cpp,
)

