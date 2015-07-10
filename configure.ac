#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([dbmanager], [1.0.1], [alexandre.poirot@legrand.fr])
AM_INIT_AUTOMAKE
LT_INIT([win32-dll])
m4_include([m4/ax_cxx_compile_stdcxx_11.m4])
m4_include([m4/ax_cxx_check_lib.m4])
m4_include([m4/pkg.m4])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_LANG_CPLUSPLUS
AC_PROG_CXX
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL
AX_CXX_COMPILE_STDCXX_11([noext],[mandatory])
CXX11FLAGS="-std=c++11"
AC_SUBST(CXX11FLAGS)

# This flag accepts an argument of the form current[:revision[:age]]. So,
# passing -version-info 3:12:1 sets current to 3, revision to 12, and age to
# 1.
#
# See https://www.sourceware.org/autobook/autobook/autobook_91.html
# for a description on how to use the version information numbering
#
# If either revision or age are omitted, they default to 0. Also note that age
# must be less than or equal to the current interface number.
## Here are a set of rules to help you update your library version information: 
#
# 1.Start with version information of 0:0:0 for each libtool library.
#
# 2.Update the version information only immediately before a public release of
# your software. More frequent updates are unnecessary, and only guarantee
# that the current interface number gets larger faster.
#
# 3.If the library source code has changed at all since the last update, then
# increment revision (c:r:a becomes c:r+1:a).
#
# 4.If any interfaces have been added, removed, or changed since the last
# update, increment current, and set revision to 0.
#
# 5.If any interfaces have been added since the last public release, then
# increment age.
#
# 6.If any interfaces have been removed since the last public release, then
# set age to 0.
#
LT_VERSION_INFO="-version-info 0:1:0"
AC_SUBST(LT_VERSION_INFO)



#Doxygen
DX_HTML_FEATURE(ON)
DX_CHM_FEATURE(OFF)
DX_CHI_FEATURE(OFF)
DX_MAN_FEATURE(OFF)
DX_RTF_FEATURE(OFF)
DX_XML_FEATURE(OFF)
DX_PDF_FEATURE(OFF)
DX_PS_FEATURE(OFF)

AC_ARG_ENABLE([extended-documentation],
	[  --enable-extended-documentation
                          Enable extended doxygen documentation],
	[case "${enableval}" in
		yes) extendeddocumentation=true ;;
		no)  extendeddocumentation=false ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-extended-documentation]) ;;
	esac], [extendeddocumentation=false]
	)
AM_CONDITIONAL([EXTDOC], [test x$extendeddocumentation = xtrue])
AM_COND_IF([EXTDOC],
[doxyfile=Doxyfile.dev],
[doxyfile=Doxyfile.user]
)

DX_INIT_DOXYGEN($PACKAGE_NAME, $doxyfile, doc)

# Checks for libraries.
PKG_CHECK_MODULES(SQLITECPP, [sqlitecpp])
AC_SUBST(SQLITECPP_CFLAGS)
AC_SUBST(SQLITECPP_LIBS)
AX_CXX_CHECK_LIB([tinyxml], [main])
TINYXML_LIBS="-ltinyxml"
AC_SUBST(TINYXML_LIBS)

case $host in
  *-*-cygwin | *-*-mingw* | *-*-pw32*)
    need_no_undefined=yes
    ;;
  *)
    need_no_undefined=no
    ;;
esac
AM_CONDITIONAL(NO_UNDEFINED, test x$need_no_undefined = xyes)
AM_COND_IF([NO_UNDEFINED], [
LT_NO_UNDEFINED="-no-undefined"
])
AC_SUBST(LT_NO_UNDEFINED)

# Checks for header files.
AC_HEADER_STDC
AX_CXX_HAVE_MUTEX
AC_CHECK_HEADERS([cassert],
		 [],
		 [AC_MSG_ERROR(cassert headers not found. Please install them.)]
		)
AC_CHECK_HEADERS([string],
		 [],
		 [AC_MSG_ERROR(C++ STL string header not found. Please install it.)]
		)
AC_CHECK_HEADERS([tinyxml.h],
		 [],
		 [AC_MSG_ERROR([tinyxml library headers not found. Please install them.])]
		)

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE

# Checks for library functions.
AC_ARG_WITH(pkgconfigdir,
AS_HELP_STRING([[[--with-pkgconfigdir]]],
	[Use the specified pkgconfig dir (default is libdir/pkgconfig)]),
	[pkgconfigdir=${withval}],
	[pkgconfigdir='${libdir}/pkgconfig'])
AC_SUBST([pkgconfigdir])
AC_MSG_NOTICE([[pkgconfig directory is ${pkgconfigdir}]])

#Handles --enable-debug flag
AC_ARG_ENABLE([debug],
	AS_HELP_STRING([--enable-debug], [Enable debug build (adds debugging output to resulting binary)]))
	
AS_IF([test "x$enable_debug" = "xyes"], [
CXX11FLAGS+=" -DDEBUG"
])

#Handles --enable-unittests flag
AC_ARG_ENABLE([unittests],
	AS_HELP_STRING([--enable-unittests], [Enable cpputest unit tests compilation]))

AM_CONDITIONAL([UNITTESTS], [test "x$enable_unittests" = "xyes"])
AM_COND_IF([UNITTESTS], [
PKG_CHECK_MODULES(CPPUTEST, [cpputest])
AC_SUBST(CPPUTEST_CFLAGS)
AC_SUBST(CPPUTEST_LIBS)
])

# Supporting BUILDING_LIBDBMANAGER compilation directive
AC_MSG_CHECKING([if we need BUILDING_LIBDBMANAGER])
case $host in
  *-*-mingw*)
    CXXFLAGS="$CXXFLAGS -DBUILDING_LIBDBMANAGER"
    AC_MSG_RESULT(yes)
    AC_MSG_CHECKING([if we need LIBDBMANAGER_STATICLIB])
    if test "X$enable_shared" = "Xno"
    then
      CXXFLAGS="$CXXFLAGS -DLIBDBMANAGER_STATICLIB"
      AC_MSG_RESULT(yes)
    else
      AC_MSG_RESULT(no)
    fi
    ;;
  *)
    AC_MSG_RESULT(no)
    ;;
esac


AC_CONFIG_FILES([
		src/dbmanager.pc:dbmanager.pc.in
		Makefile
		src/Makefile
		tests/Makefile
		])


AC_OUTPUT
