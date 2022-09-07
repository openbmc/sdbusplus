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

# Function2 might not have a pkg-config. It is header only so just make
# sure we can access the needed symbols from the header.
function2_dep = dependency('function2', required: false)
has_function2 = meson.get_compiler('cpp').has_header_symbol(
    'function2/function2.hpp',
    'fu2::unique_function',
    dependencies: function2_dep,
    required: false
)
if not has_function2
    function2_opts = import('cmake').subproject_options()
    function2_opts.add_cmake_defines({'BUILD_TESTING': 'OFF'})
    function2_proj = import('cmake').subproject(
        'function2',
        options: function2_opts,
        required: false
    )
    assert(function2_proj.found(), 'function2 is required')
    if function2_proj.found()
        function2_dep = function2_proj.dependency('function2')
    endif
endif

root_inc = include_directories('include')

boost_dep = dependency('boost', required: false)

libsdbusplus_deps = [
    boost_dep,
    function2_dep,
    libsystemd_pkg,
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
    'src/exception.cpp',
    'src/bus.cpp',
    'src/bus/match.cpp',
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
