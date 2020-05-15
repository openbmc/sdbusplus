project('sdbusplus', 'cpp',
    default_options: [
      'cpp_std=c++17',
      'werror=true',
    ],
    version: '1.0.0',
)

libsystemd_pkg = dependency('libsystemd')

libsdbusplus_src = files(
    'sdbusplus/exception.cpp',
    'sdbusplus/sdbus.cpp',
    'sdbusplus/server/transaction.cpp',
)

libsdbusplus = shared_library(
    'sdbusplus',
    libsdbusplus_src,
    dependencies: libsystemd_pkg,
    version: meson.project_version(),
    install: true,
)

install_subdir(
    'sdbusplus',
    install_dir: get_option('includedir'),
    strip_directory: false,
    # TODO (stwcx): Once we remove autotools, we can move these files
    # out of the same directory as the headers and remove this.
    exclude_files: [ 'exception.cpp', 'sdbus.cpp', 'server/transaction.cpp' ],
)

import('pkgconfig').generate(
    libraries: libsdbusplus,
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'C++ bindings for sdbus',
)
