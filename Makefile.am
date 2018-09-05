ACLOCAL_AMFLAGS = -Im4

if WANT_LIBSDBUSPLUS

nobase_include_HEADERS = \
	mapbox/recursive_wrapper.hpp \
	mapbox/variant.hpp \
	sdbusplus/asio/connection.hpp \
	sdbusplus/asio/object_server.hpp \
	sdbusplus/asio/detail/async_send_handler.hpp \
	sdbusplus/bus.hpp \
	sdbusplus/bus/match.hpp \
	sdbusplus/sdbus.hpp \
	sdbusplus/test/sdbus_mock.hpp \
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
	sdbusplus/slot.hpp \
	sdbusplus/timer.hpp \
	sdbusplus/utility/container_traits.hpp \
	sdbusplus/utility/read_into_tuple.hpp \
	sdbusplus/utility/tuple_to_array.hpp \
	sdbusplus/utility/type_traits.hpp \
	sdbusplus/vtable.hpp

LIBSDBUSPLUS_FILES = \
	sdbusplus/exception.cpp \
	sdbusplus/sdbus.cpp

if WANT_TRANSACTION
nobase_include_HEADERS += \
	sdbusplus/server/transaction.hpp
LIBSDBUPLUS_FILES_TRANSACTION = \
	sdbusplus/server/transaction.cpp
endif

libsdbusplusdir = ${libdir}
libsdbusplus_LTLIBRARIES = libsdbusplus.la
libsdbusplus_la_SOURCES = \
	$(LIBSDBUSPLUS_FILES) \
	$(LIBSDBUPLUS_FILES_TRANSACTION)
libsdbusplus_la_LIBADD = $(SYSTEMD_LIBS) $(CODE_COVERAGE_LIBS)
libsdbusplus_la_LDFLAGS = -version-info 1:0:0 -shared
libsdbusplus_la_CPPFLAGS = $(CODE_COVERAGE_CPPFLAGS)
libsdbusplus_la_CXXFLAGS = $(CODE_COVERAGE_CXXFLAGS) $(SYSTEMD_CFLAGS)

pkgconfiglibdir = ${libdir}/pkgconfig
pkgconfiglib_DATA = sdbusplus.pc

SUBDIRS = . tools example test
else
SUBDIRS = . tools
endif # WANT_LIBSDBUSPLUS
