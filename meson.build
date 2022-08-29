project('sdbusplus', 'cpp', 'c',
    default_options: [
      'buildtype=debugoptimized',
      'cpp_std=c++20',
      'warning_level=3',
      'werror=true',
      'tests=' + (meson.is_subproject() ? 'disabled' : 'auto'),
      'examples=' + (meson.is_subproject() ? 'disabled' : 'auto'),
    ],
    version: '1.0.0',
    meson_version: '>=0.57.0',
)

libsystemd_pkg = dependency('libsystemd')

python = import('python')
python_bin = python.find_installation('python3', modules:['inflection', 'yaml', 'mako'])

if not python_bin.found()
  error('No valid python3 installation found')
endif


root_inc = include_directories('include')

libsdbusplus_src = files(
    'src/async/context.cpp',
    'src/async/match.cpp',
    'src/bus.cpp',
    'src/exception.cpp',
    'src/message/native_types.cpp',
    'src/sdbus.cpp',
    'src/server/interface.cpp',
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

boost_compile_args = [
    '-DBOOST_ASIO_DISABLE_THREADS',
    '-DBOOST_ALL_NO_LIB',
    '-DBOOST_SYSTEM_NO_DEPRECATED',
    '-DBOOST_ERROR_CODE_HEADER_ONLY',
    '-DBOOST_COROUTINES_NO_DEPRECATION_WARNING',
]

boost_dep = declare_dependency(
    dependencies: dependency('boost', required: false),
    compile_args: boost_compile_args)

sdbusplus_dep = declare_dependency(
    include_directories: root_inc,
    link_with: libsdbusplus,
    dependencies: [ libsystemd_pkg, boost_dep ],
)

subdir('tools')

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
    libsdbusplus,
    name: meson.project_name(),
    version: meson.project_version(),
    requires: libsystemd_pkg,
    extra_cflags: boost_compile_args,
    description: 'C++ bindings for sdbus',
)
