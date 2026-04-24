# Generated file; do not modify.

sdbusplus_current_path = 'org/freedesktop/DBus/Error'

generated_sources += custom_target(
    'org/freedesktop/DBus/Error__cpp'.underscorify(),
    input: [
        '../../../../../yaml/org/freedesktop/DBus/Error.events.yaml',
    ],
    output: [
        'event.cpp',
        'event.hpp',
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
        meson.current_source_dir() / '../../../../../yaml',
        'org/freedesktop/DBus/Error',
    ],
    install: should_generate_cpp,
    install_dir: [
        false,
        get_option('includedir') / sdbusplus_current_path,
    ],
    build_by_default: should_generate_cpp,
)

