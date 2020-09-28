project('sdbusplus', 'cpp', 'c',
    default_options: [
      'buildtype=debugoptimized',
      'cpp_std=c++17',
      'werror=true',
      'tests=' + (meson.is_subproject() ? 'disabled' : 'auto'),
      'examples=' + (meson.is_subproject() ? 'disabled' : 'auto'),
    ],
    version: '1.0.0',
    meson_version: '>=0.53.2',
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
    dependencies: [ libsystemd_pkg ],
    version: meson.project_version(),
    install: true,
)

sdbusplus_dep = declare_dependency(
    include_directories: root_inc,
    link_with: libsdbusplus,
    dependencies: libsystemd_pkg
)

libsdbusplusasio_src = files(
    'src/sdbusplus/asio/scoped_dbus_interface.cpp',
)

libsdbusplusasio = library(
    'sdbusplusasio',
    libsdbusplusasio_src,
    include_directories: root_inc,
    dependencies: [ libsystemd_pkg, dependency('threads') ],
    version: meson.project_version(),
    install: true,
)

sdbusplusasio_dep = declare_dependency(
    include_directories: root_inc,
    link_with: [libsdbusplus, libsdbusplusasio],
    dependencies: libsystemd_pkg
)

subdir('tools')

boost_dep = dependency(
    'boost',
    disabler: true,
    required: false,
    modules: [ 'coroutine', 'context' ],
)

if not get_option('examples').disabled()
  subdir('example')
endif
if not get_option('tests').disabled()
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

import('pkgconfig').generate(
    libraries: libsdbusplusasio,
    name: meson.project_name() + 'asio',
    version: meson.project_version(),
    requires: [libsystemd_pkg, dependency('sdbusplus')],
    description: 'C++ asio bindings for sdbus',
)
