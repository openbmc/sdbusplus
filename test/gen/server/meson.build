# Generated file; do not modify.
subdir('Test')

sdbusplus_current_path = 'server'

generated_others += custom_target(
    'server/Test__markdown'.underscorify(),
    input: [ '../../yaml/server/Test.interface.yaml',  ],
    output: [ 'Test.md' ],
    depend_files: sdbusplusplus_depfiles,
    command: [
        sdbuspp_gen_meson_prog, '--command', 'markdown',
        '--output', meson.current_build_dir(),
        '--tool', sdbusplusplus_prog,
        '--directory', meson.current_source_dir() / '../../yaml',
        'server/Test',
    ],
    install: should_generate_markdown,
    install_dir: [inst_markdown_dir / sdbusplus_current_path],
    build_by_default: should_generate_markdown,
)

