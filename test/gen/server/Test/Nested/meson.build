# Generated file; do not modify.
subdir('Nested2')

sdbusplus_current_path = 'server/Test/Nested'

all_yaml_files += files(
    '../../../../yaml/server/Test/Nested.interface.yaml',
)

generated_sources += custom_target(
    'server/Test/Nested__cpp'.underscorify(),
    input: [
        '../../../../yaml/server/Test/Nested.interface.yaml',
    ],
    output: [
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
        'server/Test/Nested',
    ],
    install: should_generate_cpp,
    install_dir: [
        get_option('includedir') / sdbusplus_current_path,
        get_option('includedir') / sdbusplus_current_path,
        false,
        get_option('includedir') / sdbusplus_current_path,
        get_option('includedir') / sdbusplus_current_path,
    ],
    build_by_default: should_generate_cpp,
)

generated_markdown += custom_target(
    'server/Test/Nested/Nested2__markdown'.underscorify(),
    input: [
        '../../../../yaml/server/Test/Nested/Nested2.interface.yaml',
    ],
    output: ['Nested2.md'],
    depend_files: sdbusplusplus_depfiles,
    command: [
        sdbuspp_gen_meson_prog,
        '--command',
        'markdown',
        '--output',
        meson.current_build_dir(),
        '--tool',
        sdbusplusplus_prog,
        '--directory',
        meson.current_source_dir() / '../../../../yaml',
        'server/Test/Nested/Nested2',
    ],
    install: should_generate_markdown,
    install_dir: [inst_markdown_dir / sdbusplus_current_path],
    build_by_default: should_generate_markdown,
)

