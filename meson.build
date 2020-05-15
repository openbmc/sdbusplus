project('sdbusplus', 'cpp',
    default_options: [
      'cpp_std=c++17',
      'optimization=2',
      'werror=true',
      ],
    version: '1.0.0',
)

libsystemd_pkg = dependency('libsystemd')
libsdb_src = []

subdir('sdbusplus')

libsdb = shared_library(
    'sdbusplus',
    libsdb_src,
    dependencies: [ libsystemd_pkg ],
    version: meson.project_version(),
    install: true,
)

import('pkgconfig').generate(
    libraries: [ libsdb ],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'C++ bindings for sdbus',
)
