#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

#Version management
DBMANAGERLIB_VERSION=0.1.0
DBMANAGERLIB_MAJOR=0
DBMANAGERLIB_MINOR=1
DBMANAGERLIB_RELEASE=0


AC_PREREQ([2.69])
AC_INIT([dbmanager], [0.1.0], [alexandre.poirot@legrand.fr])
AM_INIT_AUTOMAKE
LT_INIT
m4_include([m4/ax_cxx_compile_stdcxx_11.m4])
m4_include([m4/pkg.m4])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_LANG_CPLUSPLUS
AC_PROG_CXX
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL
AX_CXX_COMPILE_STDCXX_11([noext],[mandatory])


# Checks for libraries.
PKG_CHECK_MODULES(SQLITECPP, [libsqlitecpp])
AC_SUBST(SQLITECPP_CFLAGS)
AC_SUBST(SQLITECPP_LIBS)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([cassert])
AC_CHECK_HEADERS([string])
AC_CHECK_HEADERS([sqlite3.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE

#Pass version varaibles to .in files
AC_SUBST(DBMANAGERLIB_VERSION)
AC_SUBST(DBMANAGERLIB_MAJOR)
AC_SUBST(DBMANAGERLIB_MINOR)
AC_SUBST(DBMANAGERLIB_RELEASE)

# Checks for library functions.
AC_ARG_WITH(pkgconfigdir,
AS_HELP_STRING([[[--with-pkgconfigdir]]],
	[Use the specified pkgconfig dir (default is libdir/pkgconfig)]),
	[pkgconfigdir=${withval}],
	[pkgconfigdir='${libdir}/pkgconfig'])
AC_SUBST([pkgconfigdir])
AC_MSG_NOTICE([[pkgconfig directory is ${pkgconfigdir}]])

AC_CONFIG_FILES([
		src/libdbmanager.pc:libdbmanager.pc.in
		Makefile
		src/Makefile
		])

AC_OUTPUT
