noinst_PROGRAMS = calculator-server

calculator_server_generated_files = \
	net/poettering/Calculator/server.hpp \
	net/poettering/Calculator/server.cpp \
	net/poettering/Calculator/error.hpp \
	net/poettering/Calculator/error.cpp

calculator_markdown_generated_files = \
        calculator.md

calculator_server_SOURCES = \
    calculator-server.cpp $(calculator_server_generated_files)
calculator_server_CXXFLAGS = $(SYSTEMD_CFLAGS)
calculator_server_LDADD = $(SYSTEMD_LIBS) ../libsdbusplus.la

BUILT_SOURCES = \
	$(calculator_server_generated_files) \
	$(calculator_markdown_generated_files)
CLEANFILES = \
	$(calculator_server_generated_files) \
	$(calculator_markdown_generated_files)

net/poettering/Calculator/server.hpp:
	@top_srcdir@/tools/sdbus++ \
	    interface server-header net.poettering.Calculator > $@

net/poettering/Calculator/server.cpp:
	@top_srcdir@/tools/sdbus++ \
	    interface server-cpp net.poettering.Calculator > $@

net/poettering/Calculator/error.hpp:
	@top_srcdir@/tools/sdbus++ \
	    error exception-header net.poettering.Calculator > $@

net/poettering/Calculator/error.cpp:
	@top_srcdir@/tools/sdbus++ \
	    error exception-cpp net.poettering.Calculator > $@

calculator.md:
	@top_srcdir@/tools/sdbus++ \
	    interface markdown net.poettering.Calculator > $@
	@top_srcdir@/tools/sdbus++ \
	    error markdown net.poettering.Calculator >> $@
