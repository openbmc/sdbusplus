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
root_inc = include_directories('include')
libsdbusplus_deppkgs = [ libsystemd_pkg ]

libunifex_dep = dependency('unifex', required: false)
if libunifex_dep.found()
    libsdbusplus_deppkgs += libunifex_dep
else
    libunifex_opts = import('cmake').subproject_options()
    libunifex_opts.add_cmake_defines({
        'BUILD_SHARED_LIBS': 'ON',
        'BUILD_TESTING': 'OFF',
        'BUILD_EXAMPLES': 'OFF',
        'CMAKE_CXX_STANDARD': '20',
    })

    libunifex_proj = import('cmake').subproject(
        'libunifex',
        options: libunifex_opts,
        required: true,
    )
    libunifex_dep = libunifex_proj.dependency('unifex')
endif

libsdbusplus_src = files(
    'src/exception.cpp',
    'src/bus.cpp',
    'src/message/native_types.cpp',
    'src/sdbus.cpp',
    'src/server/interface.cpp',
    'src/server/transaction.cpp',
)

libsdbusplus_deps = [ libsystemd_pkg, libunifex_dep ]

libsdbusplus = library(
    'sdbusplus',
    libsdbusplus_src,
    include_directories: root_inc,
    dependencies: libsdbusplus_deps,
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
    dependencies: [
        boost_dep,
        libsdbusplus_deps,
    ],
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
    requires: libsdbusplus_deppkgs,
    extra_cflags: boost_compile_args,
    description: 'C++ bindings for sdbus',
)
