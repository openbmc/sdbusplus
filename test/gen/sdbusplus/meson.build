# Generated file; do not modify.
generated_sources += custom_target(
    'sdbusplus_property_variant',
    input: [
        '../../yaml/server/Aggregate.interface.yaml',
        '../../yaml/server/Test.interface.yaml',
        '../../yaml/server/Test2.interface.yaml',
        '../../yaml/server/Test3.interface.yaml',
        '../../yaml/server/TestWithMethod.interface.yaml',
    ],
    output: ['property_variant.hpp'],
    depend_files: sdbusplusplus_depfiles,
    command: [
        sdbuspp_gen_meson_prog,
        '--command',
        'property-variant',
        '--output',
        meson.current_build_dir(),
        '--tool',
        sdbusplusplus_prog,
        '--directory',
        meson.current_source_dir() / '../../yaml',
    ],
    install: should_generate_cpp,
    install_dir: get_option('includedir') / 'sdbusplus',
    build_by_default: should_generate_cpp,
)
