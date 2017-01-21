if WANT_LIBSDBUSPLUS

nobase_include_HEADERS = \
	mapbox/recursive_wrapper.hpp \
	mapbox/variant.hpp \
	sdbusplus/bus.hpp \
	sdbusplus/exception.hpp \
	sdbusplus/message.hpp \
	sdbusplus/message/append.hpp \
	sdbusplus/message/native_types.hpp \
	sdbusplus/message/read.hpp \
	sdbusplus/message/types.hpp \
	sdbusplus/server.hpp \
	sdbusplus/server/bindings.hpp \
	sdbusplus/server/interface.hpp \
	sdbusplus/server/manager.hpp \
	sdbusplus/server/match.hpp \
	sdbusplus/server/object.hpp \
	sdbusplus/server/transaction.hpp \
	sdbusplus/slot.hpp \
	sdbusplus/utility/tuple_to_array.hpp \
	sdbusplus/utility/type_traits.hpp \
	sdbusplus/vtable.hpp

LIBSDBUSPLUS_FILES = \
	sdbusplus/server/transaction.cpp

libsdbusplusdir = ${libdir}
libsdbusplus_LTLIBRARIES = libsdbusplus.la
libsdbusplus_la_SOURCES = $(LIBSDBUSPLUS_FILES)
libsdbusplus_la_LDFLAGS = $(SYSTEMD_LIBS) -version-info 1:0:0 -shared -DENABLE_TRANSACTION
libsdbusplus_la_CXXFLAGS = $(SYSTEMD_CFLAGS)

pkgconfiglibdir = ${libdir}/pkgconfig
pkgconfiglib_DATA = sdbusplus.pc

endif
SUBDIRS = test tools
