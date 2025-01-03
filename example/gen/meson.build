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
        sdbuspp_gen_meson_ver
    )
endif

inst_markdown_dir = get_option('datadir') / 'doc' / meson.project_name()
inst_registry_dir = get_option('datadir') / 'redfish-registry' / meson.project_name()

