#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([FlaxPDF],[0.7.3],[])
AC_CONFIG_AUX_DIR([config])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([-Wall foreign dist-bzip2 no-dist-gzip])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER([src/autoconfig.h])
AC_PREFIX_DEFAULT([/usr])
AC_LANG_CPLUSPLUS


# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_USE_SYSTEM_EXTENSIONS
AM_GNU_GETTEXT([external])
dnl AM_GNU_GETTEXT_VERSION([0.17])

AX_PTHREAD
AX_OPENMP
LIBS=["$PTHREAD_LIBS $LIBS"]
CXXFLAGS=["$CXXFLAGS $OPENMP_CXXFLAGS $PTHREAD_CFLAGS"]
CFLAGS=["$CFLAGS $OPENMP_CFLAGS $PTHREAD_CFLAGS"]

# FLTK
unset fltkconfig
AC_CHECK_PROGS([fltkconfig], [fltk-config13 fltk-config])

if test "x$fltkconfig" = x; then
	AC_MSG_ERROR([fltk-config not found])
fi

VER=$($fltkconfig --version)
MINOR=$(echo $VER | cut -d. -f2)
PATCH=$(echo $VER | cut -d. -f3)

if test "$MINOR" -lt 3 || test "$PATCH" -lt 3; then
	AC_MSG_ERROR([FLTK >= 1.3.3 not found, got $VER])
fi

CXXFLAGS=["$CXXFLAGS $($fltkconfig --cxxflags | sed 's@-I@-isystem @g')"]
LIBS=["$LIBS $($fltkconfig --ldflags --use-images)"]

# Checks for libraries.
AC_CHECK_LIB([lzo2], [__lzo_init_v2], [], AC_MSG_ERROR([LZO not found]))
#AC_CHECK_LIB([dl], [dlopen], [], AC_MSG_ERROR([libdl not found]))
#AC_CHECK_LIB([rt], [sched_get_priority_min], [], AC_MSG_ERROR([librt not found]))
PKG_CHECK_MODULES([DEPS], [poppler >= 0.31.0 xrender])

DEPS_CFLAGS=$(echo $DEPS_CFLAGS | sed 's@-I@-isystem @g')

LIBS=["$LIBS $DEPS_LIBS"]
CXXFLAGS=["$CXXFLAGS $DEPS_CFLAGS"]

# Check for webkitfltk version
#AC_MSG_CHECKING([webkitfltk version is ok])
#AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <webkit.h>]],
#	[[
	#if WK_FLTK_MAJOR != 0 || WK_FLTK_MINOR < 2
	#error unsupported webkitfltk version
	#endif
#	]])], [AC_MSG_RESULT([yes])], [AC_MSG_FAILURE([Unsupported webkit version, 0.2 required])]
#	)

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Random other things
CFLAGS=["$CFLAGS -Wall -Wextra"]
CXXFLAGS=["$CXXFLAGS -Wall -Wextra"]
LDFLAGS=["$LDFLAGS -Wl,-O1"]

AX_CHECK_COMPILE_FLAG([-ffunction-sections],
	[CXXFLAGS="$CXXFLAGS -ffunction-sections -fdata-sections"
	LDFLAGS="$LDFLAGS -Wl,-gc-sections"
	ac_cv_sections_supported=yes],
	[ac_cv_sections_supported=no],
	[])

AX_CHECK_LINK_FLAG([-Wl,-as-needed], [LDFLAGS="$LDFLAGS -Wl,-as-needed"],
	[], [])

AC_CONFIG_COMMANDS([atag],[
echo -e "\n\n\t\t*********\n"
echo Configure finished
echo -e "\n\t\t*********\n\n"],
[distributable=$distributable])

AC_CONFIG_FILES([Makefile src/Makefile po/Makefile.in])
AC_OUTPUT
