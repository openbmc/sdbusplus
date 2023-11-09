# Generated file; do not modify.
generated_sources += custom_target(
    'net/poettering/Dummy__cpp'.underscorify(),
    input: [ '../../../../yaml/net/poettering/Dummy.interface.yaml',  ],
    output: [ 'common.hpp', 'server.cpp', 'server.hpp', 'aserver.hpp', 'client.hpp',  ],
    depend_files: sdbusplusplus_depfiles,
    command: [
        sdbuspp_gen_meson_prog, '--command', 'cpp',
        '--output', meson.current_build_dir(),
        '--tool', sdbusplusplus_prog,
        '--directory', meson.current_source_dir() / '../../../../yaml',
        'net/poettering/Dummy',
    ],
)

