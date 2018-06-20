# Initialization
AC_PREREQ([2.69])
AC_INIT([sdbusplus], [1.0], [https://github.com/openbmc/sdbusplus/issues])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign dist-xz])
AM_SILENT_RULES([yes])

# Make sure the default CFLAGS of `-O2 -g` don't override CODE_COVERAGE_CFLAGS
# It is important that this comes before AC_PROG_C{C,XX}, as we are attempting
# to stop them from populating default CFLAGS and CXXFLAGS.
AS_IF([test "x$enable_code_coverage" != "xno"], [
    AS_IF([test "x${CXXFLAGS+set}" != "xset"], [
        AC_SUBST(CXXFLAGS, [""])
    ])
    AS_IF([test "x${CFLAGS+set}" != "xset"], [
        AC_SUBST(CFLAGS, [""])
    ])
])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AM_PROG_AR
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# setup sdbus++
AM_PATH_PYTHON([2.7], [AC_SUBST([PYTHON], [echo "$PYTHON"])])
AC_SUBST([PYTHONDIR], ${pythondir})
AS_IF([test "$PYTHON" != :], [have_python=yes], [have_python=no])

AC_ARG_ENABLE([sdbuspp],
    AS_HELP_STRING([--disable-sdbuspp], [Disable sdbus++]))

AS_IF([test "x$enable_sdbuspp" != "xno"],
    [AS_IF([test "x$have_python" = "xno"], [AC_MSG_ERROR([sdbus++ requires python])])])

AM_CONDITIONAL([WANT_SDBUSPP], [test "x$enable_sdbuspp" != "xno"])

# setup libsdbusplus
AX_PKG_CHECK_MODULES(
    [SYSTEMD],
    [libsystemd >= 221],
    [],
    [have_systemd=yes],
    [have_systemd=no])
AC_ARG_ENABLE([libsdbusplus],
    AS_HELP_STRING([--disable-libsdbusplus], [Disable libsdbusplus]))

AS_IF([test "x$enable_libsdbusplus" != "xno"],
      [AS_IF([test "x$have_systemd" = "xno"], [AC_MSG_ERROR([libsdbusplus requires systemd])])])

AM_CONDITIONAL([WANT_LIBSDBUSPLUS], [test "x$enable_libsdbusplus" != "xno"])

# setup transaction
AC_ARG_ENABLE([transaction],
    AS_HELP_STRING([--enable-transaction], [Enable transaction id support.]))
AC_ARG_VAR(WANT_TRANSACTION, [Enable transaction id support.])
AS_IF([test "x$enable_transaction" = "xyes"], \
    [WANT_TRANSACTION="1"], \
    [WANT_TRANSACTION="0"])
AM_CONDITIONAL([WANT_TRANSACTION], [test "x$enable_transaction" = "xyes"])

# Checks for header files.
AS_IF([test "x$enable_libsdbusplus" != "xno"],
      [AC_CHECK_HEADER(systemd/sd-bus.h, ,[AC_MSG_ERROR([Could not find systemd/sd-bus.h...systemd development package required])])])

# Checks for typedefs, structures, and compiler characteristics.
AS_IF([test "x$enable_libsdbusplus" != "xno"],
      [AX_CXX_COMPILE_STDCXX_14([noext])])
AX_APPEND_COMPILE_FLAGS([-Wall -Werror], [CFLAGS])
AX_APPEND_COMPILE_FLAGS([-Wall -Werror], [CXXFLAGS])

# Checks for library functions.
LT_INIT # Removes 'unrecognized options: --with-libtool-sysroot'

# Legacy tests require pthread support
AS_IF([test "x$enable_tests" != "xno"], [
    AX_PTHREAD([], [
        AS_IF([test "x$enable_tests" != "xyes"], [
            AC_MSG_ERROR([Testing enabled but could not find pthreads])
        ])
    ])
])

# Check/set gtest specific functions.
AC_ARG_ENABLE([tests], [Whether we should build test cases])
AS_IF([test "x$enable_tests" != "xno"], [
    PKG_CHECK_MODULES([GTEST], [gtest])

    AX_SAVE_FLAGS_WITH_PREFIX(OLD, [CPPFLAGS])
    AX_APPEND_COMPILE_FLAGS([$GTEST_CFLAGS], [CPPFLAGS])
    AC_LANG_PUSH([C++])
    AC_CHECK_HEADERS([gtest/gtest.h], [
        AS_IF([test "x$GTEST_CFLAGS" = "x"], [
            AS_IF([test "x$PTHREAD_CFLAGS" = "x"], [
                AX_APPEND_COMPILE_FLAGS(["-DGTEST_HAS_PTHREAD=0"], [GTEST_CFLAGS])
            ], [
                AX_APPEND_COMPILE_FLAGS(["-DGTEST_HAS_PTHREAD=1"], [GTEST_CFLAGS])
                AX_APPEND_COMPILE_FLAGS([$PTHREAD_CFLAGS], [GTEST_CFLAGS])
            ])
        ])
    ], [
        AS_IF([test "x$enable_tests" = "xyes"], [
            AC_MSG_ERROR([Testing enabled but could not find gtest/gtest.h])
        ])
    ])
    AC_LANG_POP([C++])
    AX_RESTORE_FLAGS_WITH_PREFIX(OLD, [CPPFLAGS])

    AX_SAVE_FLAGS_WITH_PREFIX(OLD, [LDFLAGS])
    AX_APPEND_COMPILE_FLAGS([$GTEST_LIBS], [LDFLAGS])
    AC_CHECK_LIB([gtest], [main], [
        AS_IF([test "x$GTEST_LIBS" = "x"], [
            AX_APPEND_COMPILE_FLAGS([-lgtest], [GTEST_LIBS])
        ])
    ], [
        AS_IF([test "x$enable_tests" = "xyes"], [
            AC_MSG_ERROR([Testing enabled but couldn't find gtest libs])
        ])
    ])
    AX_RESTORE_FLAGS_WITH_PREFIX(OLD, [LDFLAGS])
])

# Code coverage
AX_CODE_COVERAGE
AM_EXTRA_RECURSIVE_TARGETS([check-code-coverage])

AC_ARG_ENABLE([oe-sdk],
    AS_HELP_STRING([--enable-oe-sdk], [Link testcases absolutely against OE SDK so they can be ran within it.])
)
AC_ARG_VAR(OECORE_TARGET_SYSROOT,
    [Path to the OE SDK SYSROOT])
AS_IF([test "x$enable_oe_sdk" == "xyes"],
    AS_IF([test "x$OECORE_TARGET_SYSROOT" == "x"],
          AC_MSG_ERROR([OECORE_TARGET_SYSROOT must be set with --enable-oe-sdk])
    )
    AC_MSG_NOTICE([Enabling OE-SDK at $OECORE_TARGET_SYSROOT])
    [
        testcase_flags="-Wl,-rpath,\${OECORE_TARGET_SYSROOT}/lib"
        testcase_flags="${testcase_flags} -Wl,-rpath,\${OECORE_TARGET_SYSROOT}/usr/lib"
        testcase_flags="${testcase_flags} -Wl,-dynamic-linker,`find \${OECORE_TARGET_SYSROOT}/lib/ld-*.so | sort -r -n | head -n1`"
    ]
    AC_SUBST([OESDK_TESTCASE_FLAGS], [$testcase_flags])
)

# Create configured output
AC_CONFIG_FILES([Makefile test/Makefile tools/Makefile tools/setup.py])
AC_CONFIG_FILES([example/Makefile])
AC_CONFIG_FILES([sdbusplus.pc])
AC_CONFIG_FILES([sdbusplus/bus.hpp])
AC_CONFIG_FILES([sdbusplus/server.hpp])
AC_CONFIG_FILES([tools/sdbusplus/templates/interface.mako.server.cpp])
AC_CONFIG_FILES([tools/sdbusplus/templates/method.mako.prototype.hpp])
AC_CONFIG_FILES([tools/sdbusplus/templates/property.mako.prototype.hpp])
AC_OUTPUT
