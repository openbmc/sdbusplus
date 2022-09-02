sdbusplusplus_depfiles = files(
  'sdbus++',
  'sdbusplus/enum.py',
  'sdbusplus/error.py',
  'sdbusplus/__init__.py',
  'sdbusplus/interface.py',
  'sdbusplus/main.py',
  'sdbusplus/method.py',
  'sdbusplus/namedelement.py',
  'sdbusplus/property.py',
  'sdbusplus/renderer.py',
  'sdbusplus/signal.py',
  'sdbusplus/templates/error.cpp.mako',
  'sdbusplus/templates/error.hpp.mako',
  'sdbusplus/templates/interface.client.hpp.mako',
  'sdbusplus/templates/interface.server.cpp.mako',
  'sdbusplus/templates/interface.server.hpp.mako',
  'sdbusplus/templates/method.prototype.hpp.mako',
  'sdbusplus/templates/property.prototype.hpp.mako',
  'sdbusplus/templates/signal.prototype.hpp.mako',
)
meson.override_find_program('sdbus++', files('sdbus++'))
sdbusplusplus_prog = find_program('sdbus++', native: true)

meson.override_find_program('sdbus++-gendir', files('sdbus++-gendir'))
sdbusgen_prog = find_program('sdbus++-gendir', native: true)

meson.override_find_program('sdbus++-gen-meson', files('sdbus++-gen-meson'))
sdbuspp_gen_meson_prog = find_program('sdbus++-gen-meson', native: true)
