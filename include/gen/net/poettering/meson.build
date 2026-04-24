# Generated file; do not modify.
subdir('Calculator')

sdbusplus_current_path = 'net/poettering'

generated_markdown += custom_target(
    'net/poettering/Calculator__markdown'.underscorify(),
    input: [
        '../../../yaml/net/poettering/Calculator.events.yaml',
        '../../../yaml/net/poettering/Calculator.interface.yaml',
    ],
    output: ['Calculator.md'],
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
        meson.current_source_dir() / '../../../yaml',
        'net/poettering/Calculator',
    ],
    install: should_generate_markdown,
    install_dir: [inst_markdown_dir / sdbusplus_current_path],
    build_by_default: should_generate_markdown,
)

generated_registry += custom_target(
    'net/poettering/Calculator__registry'.underscorify(),
    input: [
        '../../../yaml/net/poettering/Calculator.events.yaml',
    ],
    output: ['Calculator.json'],
    depend_files: sdbusplusplus_depfiles,
    command: [
        sdbuspp_gen_meson_prog,
        '--command',
        'registry',
        '--output',
        meson.current_build_dir(),
        '--tool',
        sdbusplusplus_prog,
        '--directory',
        meson.current_source_dir() / '../../../yaml',
        'net/poettering/Calculator',
    ],
    install: should_generate_registry,
    install_dir: [inst_registry_dir / sdbusplus_current_path],
    build_by_default: should_generate_registry,
)

