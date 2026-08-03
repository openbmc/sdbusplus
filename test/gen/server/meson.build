# Generated file; do not modify.
subdir('Test')
subdir('Test2')
subdir('Test3')
subdir('TestWithMethod')

sdbusplus_current_path = 'server'

generated_markdown += custom_target(
    'server/Test__markdown'.underscorify(),
    input: [
        '../../yaml/server/Test.events.yaml',
        '../../yaml/server/Test.interface.yaml',
    ],
    output: ['Test.md'],
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
        meson.current_source_dir() / '../../yaml',
        'server/Test',
    ],
    install: should_generate_markdown,
    install_dir: [inst_markdown_dir / sdbusplus_current_path],
    build_by_default: should_generate_markdown,
)

generated_registry += custom_target(
    'server/Test__registry'.underscorify(),
    input: [
        '../../yaml/server/Test.events.yaml',
    ],
    output: ['Test.json'],
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
        meson.current_source_dir() / '../../yaml',
        'server/Test',
    ],
    install: should_generate_registry,
    install_dir: [inst_registry_dir / sdbusplus_current_path],
    build_by_default: should_generate_registry,
)

generated_markdown += custom_target(
    'server/Test2__markdown'.underscorify(),
    input: [
        '../../yaml/server/Test2.interface.yaml',
    ],
    output: ['Test2.md'],
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
        meson.current_source_dir() / '../../yaml',
        'server/Test2',
    ],
    install: should_generate_markdown,
    install_dir: [inst_markdown_dir / sdbusplus_current_path],
    build_by_default: should_generate_markdown,
)

generated_markdown += custom_target(
    'server/Test3__markdown'.underscorify(),
    input: [
        '../../yaml/server/Test3.interface.yaml',
    ],
    output: ['Test3.md'],
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
        meson.current_source_dir() / '../../yaml',
        'server/Test3',
    ],
    install: should_generate_markdown,
    install_dir: [inst_markdown_dir / sdbusplus_current_path],
    build_by_default: should_generate_markdown,
)

generated_markdown += custom_target(
    'server/TestWithMethod__markdown'.underscorify(),
    input: [
        '../../yaml/server/TestWithMethod.interface.yaml',
    ],
    output: ['TestWithMethod.md'],
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
        meson.current_source_dir() / '../../yaml',
        'server/TestWithMethod',
    ],
    install: should_generate_markdown,
    install_dir: [inst_markdown_dir / sdbusplus_current_path],
    build_by_default: should_generate_markdown,
)

