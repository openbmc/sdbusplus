install_headers(
    'bus.hpp',
    'exception.hpp',
    'message.hpp',
    'sdbus.hpp',
    'server.hpp',
    'slot.hpp',
    'timer.hpp',
    'vtable.hpp',
    subdir: 'sdbusplus',
)

subdir('asio')
subdir('bus')
subdir('message')
subdir('server')
subdir('test')
subdir('utility')

libsdb_src += files(
    'exception.cpp',
    'sdbus.cpp',
)
