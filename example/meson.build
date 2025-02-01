executable('list-users', 'list-users.cpp', dependencies: sdbusplus_dep)

has_asio = meson.get_compiler('cpp').has_header_symbol(
    'boost/asio.hpp',
    'boost::asio::io_context',
    required: false,
)
asio_dep = sdbusplus_dep
if not has_asio
    asio_dep = disabler()
endif

assert(
    not get_option('examples').allowed() or has_asio,
    'Boost is required when examples are enabled',
)

executable(
    'asio-example',
    'asio-example.cpp',
    dependencies: [
        asio_dep,
        dependency(
            'boost',
            modules: ['coroutine', 'context'],
            disabler: true,
            required: false,
        ),
    ],
)

executable(
    'coroutine-example',
    'coroutine-example.cpp',
    dependencies: [sdbusplus_dep],
)

executable('register-property', 'register-property.cpp', dependencies: asio_dep)

executable(
    'get-all-properties',
    'get-all-properties.cpp',
    dependencies: asio_dep,
)

yaml_selected_subdirs = ['net']
subdir('gen')

executable(
    'calculator-server',
    'calculator-server.cpp',
    generated_sources,
    implicit_include_directories: false,
    include_directories: include_directories('gen'),
    dependencies: sdbusplus_dep,
)

executable(
    'calculator-aserver',
    'calculator-aserver.cpp',
    generated_sources,
    implicit_include_directories: false,
    include_directories: include_directories('gen'),
    dependencies: sdbusplus_dep,
)

executable(
    'calculator-client',
    'calculator-client.cpp',
    generated_sources,
    implicit_include_directories: false,
    include_directories: include_directories('gen'),
    dependencies: sdbusplus_dep,
)
