# Generated file; do not modify.
subdir('Error')

sdbusplus_current_path = 'org/freedesktop/DBus'

generated_markdown += custom_target(
    'org/freedesktop/DBus/Error__markdown'.underscorify(),
    input: [ '../../../../org/freedesktop/DBus/Error.events.yaml',  ],
    output: [ 'Error.md' ],
    depend_files: sdbusplusplus_depfiles,
    command: [
        sdbuspp_gen_meson_prog, '--command', 'markdown',
        '--output', meson.current_build_dir(),
        '--tool', sdbusplusplus_prog,
        '--directory', meson.current_source_dir() / '../../../..',
        'org/freedesktop/DBus/Error',
    ],
    install: should_generate_markdown,
    install_dir: [inst_markdown_dir / sdbusplus_current_path],
    build_by_default: should_generate_markdown,
)

generated_registry += custom_target(
    'org/freedesktop/DBus/Error__registry'.underscorify(),
    input: [ '../../../../org/freedesktop/DBus/Error.events.yaml',  ],
    output: [ 'Error.json' ],
    depend_files: sdbusplusplus_depfiles,
    command: [
        sdbuspp_gen_meson_prog, '--command', 'registry',
        '--output', meson.current_build_dir(),
        '--tool', sdbusplusplus_prog,
        '--directory', meson.current_source_dir() / '../../../..',
        'org/freedesktop/DBus/Error',
    ],
    install: should_generate_registry,
    install_dir: [inst_registry_dir / sdbusplus_current_path],
    build_by_default: should_generate_registry,
)

