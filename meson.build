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

boost_dep = dependency('boost', required: false)

libsdbusplus_deps = [
    boost_dep,
    libsystemd_pkg,
    dependency('stdplus'),
]

libsdbusplus_cflags = [
    '-DBOOST_ASIO_DISABLE_THREADS',
    '-DBOOST_ALL_NO_LIB',
    '-DBOOST_SYSTEM_NO_DEPRECATED',
    '-DBOOST_ERROR_CODE_HEADER_ONLY',
    '-DBOOST_COROUTINES_NO_DEPRECATION_WARNING',
]

libsdbusplus_pre = declare_dependency(
    include_directories: root_inc,
    compile_args: libsdbusplus_cflags,
    dependencies: libsdbusplus_deps,
)

libsdbusplus = library(
    'sdbusplus',
    'src/async/context.cpp',
    'src/async/match.cpp',
    'src/async/scope.cpp',
    'src/bus.cpp',
    'src/bus/match.cpp',
    'src/event.cpp',
    'src/exception.cpp',
    'src/message/native_types.cpp',
    'src/sdbus.cpp',
    'src/server/interface.cpp',
    'src/server/transaction.cpp',
    dependencies: libsdbusplus_pre,
    version: meson.project_version(),
    install: true,
)

sdbusplus_dep = declare_dependency(
    link_with: libsdbusplus,
    dependencies: libsdbusplus_pre,
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

libsdbusplus_reqs = []
foreach dep : libsdbusplus_deps
    if dep.type_name() == 'pkgconfig'
        libsdbusplus_reqs += dep
    endif
endforeach

import('pkgconfig').generate(
    libsdbusplus,
    name: meson.project_name(),
    version: meson.project_version(),
    requires: libsdbusplus_reqs,
    extra_cflags: libsdbusplus_cflags,
    description: 'C++ bindings for sdbus',
)
