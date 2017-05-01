if WANT_LIBSDBUSPLUS

nobase_include_HEADERS = \
	mapbox/recursive_wrapper.hpp \
	mapbox/variant.hpp \
	sdbusplus/bus.hpp \
	sdbusplus/bus/match.hpp \
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
	sdbusplus/server/object.hpp \
	sdbusplus/server/transaction.hpp \
	sdbusplus/slot.hpp \
	sdbusplus/utility/tuple_to_array.hpp \
	sdbusplus/utility/type_traits.hpp \
	sdbusplus/vtable.hpp

LIBSDBUSPLUS_FILES = \
	sdbusplus/exception.cpp

if WANT_TRANSACTION
LIBSDBUPLUS_FILES_TRANSACTION = \
	sdbusplus/server/transaction.cpp
endif

libsdbusplusdir = ${libdir}
libsdbusplus_LTLIBRARIES = libsdbusplus.la
libsdbusplus_la_SOURCES = \
	$(LIBSDBUSPLUS_FILES) \
	$(LIBSDBUPLUS_FILES_TRANSACTION)
libsdbusplus_la_LIBADD = $(SYSTEMD_LIBS)
libsdbusplus_la_LDFLAGS = -version-info 1:0:0 -shared
libsdbusplus_la_CXXFLAGS = $(SYSTEMD_CFLAGS)

pkgconfiglibdir = ${libdir}/pkgconfig
pkgconfiglib_DATA = sdbusplus.pc

endif
SUBDIRS = test tools
