project('sdbusplus', 'cpp', 'c',
    default_options: [
      'buildtype=debugoptimized',
      'cpp_std=c++17',
      'werror=true',
      'tests=' + (meson.is_subproject() ? 'disabled' : 'auto'),
      'examples=' + (meson.is_subproject() ? 'disabled' : 'auto'),
    ],
    version: '1.0.0',
)

libsystemd_pkg = dependency('libsystemd')
root_inc = include_directories('include')

libsdbusplus_src = files(
    'src/exception.cpp',
    'src/sdbus.cpp',
    'src/server/transaction.cpp',
)

libsdbusplus = library(
    'sdbusplus',
    libsdbusplus_src,
    include_directories: root_inc,
    dependencies: libsystemd_pkg,
    version: meson.project_version(),
    install: true,
)

sdbusplus_dep = declare_dependency(
    include_directories: root_inc,
    link_with: libsdbusplus,
    dependencies: libsystemd_pkg
)

# We first find these tools outside of the directory they are contained in
# to ensure we allow the system to provide them instead of using our local
# copy in cross compiled builds where the python environment will not work
# with ad-hoc source contained scripts that need a working PYTHONPATH.
sdbuspp_prog = find_program('sdbus++', native: true, required: false)
sdbusgen_prog = find_program('sdbus++-gendir', native: true, required: false)
subdir('tools')

examples_opt = get_option('examples')
if not examples_opt.disabled()
    subdir('example')
endif

tests_opt = get_option('tests')
if not tests_opt.disabled()
    subdir('test')
endif

install_subdir(
    'include/sdbusplus',
    install_dir: get_option('includedir'),
    strip_directory: false,
)

import('pkgconfig').generate(
    libraries: libsdbusplus,
    name: meson.project_name(),
    version: meson.project_version(),
    requires: libsystemd_pkg,
    description: 'C++ bindings for sdbus',
)
