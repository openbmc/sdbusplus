executable(
    'list-users',
    'list-users.cpp',
    dependencies: sdbusplus_dep,
)

  asio_dep = disabler()

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
    dependencies: [ sdbusplus_dep ],
)

executable(
    'register-property',
    'register-property.cpp',
    dependencies: asio_dep,
)

executable(
    'get-all-properties',
    'get-all-properties.cpp',
    dependencies: asio_dep,
)

generated_sources = []
generated_others = []
subdir('gen')
subdir('gen/net')

executable(
    'calculator-server',
    'calculator-server.cpp',
    generated_sources,
    implicit_include_directories: false,
    include_directories: include_directories('gen'),
    dependencies: sdbusplus_dep,
)
