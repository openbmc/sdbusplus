project(
    'sdbusplus',
    'cpp',
    'c',
    default_options: [
        'buildtype=debugoptimized',
        'cpp_std=c++23',
        'warning_level=3',
        'werror=true',
        'tests=' + (meson.is_subproject() ? 'disabled' : 'auto'),
        'examples=' + (meson.is_subproject() ? 'disabled' : 'auto'),
    ],
    version: '1.0.0',
    meson_version: '>=1.1.1',
)
cxx = meson.get_compiler('cpp')
if (cxx.get_id() == 'clang')
    add_project_arguments(['-Wno-c++26-extensions'], language: 'cpp')
endif

libsystemd_pkg = dependency('libsystemd')
nlohmann_json_dep = dependency('nlohmann_json', include_type: 'system')

python = import('python')
python_bin = python.find_installation(
    'python3',
    modules: ['inflection', 'yaml', 'mako'],
)

if not python_bin.found()
    error('No valid python3 installation found')
endif

root_inc = include_directories('include')

libsdbusplus_src = files(
    'src/async/context.cpp',
    'src/async/fdio.cpp',
    'src/async/match.cpp',
    'src/async/mutex.cpp',
    'src/bus.cpp',
    'src/bus/match.cpp',
    'src/event.cpp',
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
    dependencies: [libsystemd_pkg, nlohmann_json_dep],
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
    compile_args: boost_compile_args,
)

sdbusplus_dep = declare_dependency(
    include_directories: root_inc,
    link_with: libsdbusplus,
    dependencies: [boost_dep, libsystemd_pkg, nlohmann_json_dep],
)

if meson.is_subproject()
    sdbusplus_dep = sdbusplus_dep.as_system()
endif

should_generate_cpp = true
should_generate_markdown = false
should_generate_registry = false

subdir('tools')

if get_option('examples').allowed()
    subdir('example')
endif
if get_option('tests').allowed()
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
