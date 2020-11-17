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

thread_dep = dependency('threads',
                required: false)

libsdbusplus_src = files(
    'src/exception.cpp',
    'src/sdbus.cpp',
    'src/server/transaction.cpp',
    'src/sdbusplus/test/integration/daemon_manager.cpp',
    'src/sdbusplus/test/integration/dbusd_manager.cpp',
    'src/sdbusplus/test/integration/private_bus.cpp',
    'src/sdbusplus/test/integration/mock_object.cpp',
    'src/sdbusplus/test/integration/mock_service.cpp',
)

libsdbusplus = library(
    'sdbusplus',
    libsdbusplus_src,
    include_directories: root_inc,
    dependencies: [libsystemd_pkg, thread_dep],
    version: meson.project_version(),
    install: true,
)

sdbusplus_dep = declare_dependency(
    include_directories: root_inc,
    link_with: libsdbusplus,
    dependencies: [libsystemd_pkg, thread_dep]
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
