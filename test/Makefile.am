if WANT_LIBSDBUSPLUS

AM_CPPFLAGS = -I$(top_srcdir) $(GTEST_CPPFLAGS)
gtest_ldflags = -lgtest -lgtest_main
check_PROGRAMS =

TESTS = $(check_PROGRAMS)

check_PROGRAMS += bus_list_names
bus_list_names_SOURCES = bus/list_names.cpp
bus_list_names_LDFLAGS = $(gtest_ldflags) $(SYSTEMD_LIBS)

check_PROGRAMS += message_append
message_append_CXXFLAGS = $(SYSTEMD_CFLAGS) $(PTHREAD_CFLAGS)
message_append_LDFLAGS = $(SYSTEMD_LIBS) $(PTHREAD_LIBS)
message_append_SOURCES = message/append.cpp

check_PROGRAMS += message_read
message_read_CXXFLAGS = $(SYSTEMD_CFLAGS) $(PTHREAD_CFLAGS)
message_read_LDFLAGS = $(SYSTEMD_LIBS) $(PTHREAD_LIBS)
message_read_SOURCES = message/read.cpp

check_PROGRAMS += message_native_types
message_native_types_SOURCES = message/native_types.cpp

check_PROGRAMS += message_types
message_types_SOURCES = message/types.cpp
message_types_LDFLAGS = $(gtest_ldflags)

check_PROGRAMS += utility_tuple_to_array
utility_tuple_to_array_SOURCES = utility/tuple_to_array.cpp
utility_tuple_to_array_LDFLAGS = $(gtest_ldflags)

check_PROGRAMS += utility_type_traits
utility_type_traits_SOURCES = utility/type_traits.cpp

check_PROGRAMS += vtable_vtable
vtable_vtable_SOURCES = vtable/vtable.cpp vtable/vtable_c.c
vtable_vtable_LDFLAGS = $(gtest_ldflags)

endif
