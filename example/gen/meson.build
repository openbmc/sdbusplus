# Generated file; do not modify.
sdbuspp_gen_meson_ver = run_command(
    sdbuspp_gen_meson_prog,
    '--version',
    check: true,
).stdout().strip().split('\n')[0]

if sdbuspp_gen_meson_ver != 'sdbus++-gen-meson version 10'
    warning('Generated meson files from wrong version of sdbus++-gen-meson.')
    warning(
        'Expected "sdbus++-gen-meson version 10", got:',
        sdbuspp_gen_meson_ver,
    )
endif

inst_markdown_dir = get_option('datadir') / 'doc' / meson.project_name()
inst_registry_dir = get_option('datadir') / 'redfish-registry' / meson.project_name()

all_yaml_files = []
generated_sources = []
generated_headers = []
generated_markdown = []
generated_registry = []

foreach d : yaml_selected_subdirs
    subdir(d)
endforeach

generated_headers += custom_target(
    'common/full_tree'.underscorify(),
    input: all_yaml_files,
    output: [
        'full_tree.hpp',
    ],
    depend_files: sdbusplusplus_depfiles,
    command: [
        sdbuspp_gen_meson_prog,
        '--command',
        'tree',
        '--output',
        meson.current_build_dir(),
        '--tool',
        sdbusplusplus_prog,
        '--directory',
        meson.current_source_dir() / '../yaml',
        'xyz',
    ],
    install: should_generate_cpp,
    install_dir: [
        get_option('includedir') / sdbusplus_current_path,
    ],
    build_by_default: should_generate_cpp,
)

foreach s : generated_sources
    foreach f : s.to_list()
        if f.full_path().endswith('.hpp')
            generated_headers += f
        endif
    endforeach
endforeach

