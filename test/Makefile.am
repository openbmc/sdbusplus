@VALGRIND_CHECK_RULES@
VALGRIND_SUPPRESSIONS_FILES = valgrind.supp


if WANT_LIBSDBUSPLUS

AM_CPPFLAGS = -I$(top_builddir) -I$(top_srcdir) \
              $(GTEST_CFLAGS) $(GMOCK_CFLAGS) $(SYSTEMD_CFLAGS) \
              $(CODE_COVERAGE_CPPFLAGS)
AM_CFLAGS = $(CODE_COVERAGE_CFLAGS)
AM_CXXFLAGS = $(CODE_COVERAGE_CXXFLAGS)
test_ldadd = $(top_builddir)/libsdbusplus.la $(SYSTEMD_LIBS) \
             $(OESDK_TESTCASE_FLAGS) $(CODE_COVERAGE_LIBS)
gtest_ldadd = $(test_ldadd) $(GTEST_LIBS) $(GMOCK_LIBS) -lgmock_main -lpthread

check_PROGRAMS =
TESTS = $(check_PROGRAMS)

check_PROGRAMS += bus_list_names
bus_list_names_SOURCES = bus/list_names.cpp
bus_list_names_LDADD = $(gtest_ldadd)

check_PROGRAMS += bus_match
bus_match_SOURCES = bus/match.cpp
bus_match_LDADD = $(gtest_ldadd)

check_PROGRAMS += exception_sdbus_error
exception_sdbus_error_SOURCES = exception/sdbus_error.cpp
exception_sdbus_error_LDADD = $(gtest_ldadd)

check_PROGRAMS += message_append
message_append_SOURCES = message/append.cpp
message_append_LDADD = $(gtest_ldadd)

check_PROGRAMS += message_read
message_read_SOURCES = message/read.cpp
message_read_LDADD = $(gtest_ldadd)

check_PROGRAMS += message_native_types
message_native_types_SOURCES = message/native_types.cpp
message_native_types_LDADD = $(gtest_ldadd)

check_PROGRAMS += message_types
message_types_SOURCES = message/types.cpp
message_types_LDADD = $(gtest_ldadd)

check_PROGRAMS += utility_tuple_to_array
utility_tuple_to_array_SOURCES = utility/tuple_to_array.cpp
utility_tuple_to_array_LDADD = $(gtest_ldadd)

check_PROGRAMS += utility_type_traits
utility_type_traits_SOURCES = utility/type_traits.cpp
utility_type_traits_LDADD = $(gtest_ldadd)

check_PROGRAMS += vtable_vtable
vtable_vtable_SOURCES = vtable/vtable.cpp vtable/vtable_c.c
vtable_vtable_LDADD = $(gtest_ldadd)

check_PROGRAMS += timer
timer_SOURCES = timer.cpp
timer_LDADD = $(gtest_ldadd)

server/Test/server.hpp:
	@mkdir -p $(@D)
	@top_srcdir@/tools/sdbus++ \
	    -r $(srcdir) -t $(top_builddir)/tools/sdbusplus/templates \
	    interface server-header server.Test > $@

server/Test/server.cpp:
	@mkdir -p $(@D)
	@top_srcdir@/tools/sdbus++ \
	    -r $(srcdir) -t $(top_builddir)/tools/sdbusplus/templates \
	    interface server-cpp server.Test > $@

server_test_generated_sources = server/Test/server.cpp server/Test/server.hpp

BUILT_SOURCES = $(server_test_generated_sources)
CLEANFILES = $(server_test_generated_sources)

check_PROGRAMS += object
object_SOURCES = server/object.cpp $(server_test_generated_sources)
object_LDADD = $(gtest_ldadd)

endif
