server_test_sources = custom_target(
    'server_test__cpp',
    input: '../Test.interface.yaml',
    output: [ 'server.cpp', 'server.hpp', 'client.hpp' ],
    depend_files: sdbusplusplus_depfiles,
    command: [
        sdbuspp_gen_meson_prog, '--command', 'cpp',
        '--output', meson.current_build_dir(),
        '--tool', sdbusplusplus_prog,
        '--directory', meson.current_source_dir() / '../..',
        'server/Test',
    ],
)
