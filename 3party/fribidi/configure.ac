##                                               -*- Autoconf -*-
## Process this file with autoconf to produce a configure script.
## Use autoupdate to update this file for newer versions of autoconf.
## Use autoscan to check if you need to add something to this file.
##############################################################################
## $Id: configure.ac,v 1.28 2009-03-26 22:37:29 behdad Exp $
## $Auther: behdad $
## $Date: 2009-03-26 22:37:29 $
## $Revision: 1.28 $
## $Source: /cvs/fribidi/fribidi2/configure.ac,v $
##############################################################################

AC_PREREQ(2.56)

dnl Define different version variables.
dnl
dnl Making releases:
dnl   FRIBIDI_MICRO_VERSION++;
dnl   FRIBIDI_INTERFACE_AGE++;
dnl
dnl If any functions have been added:
dnl   FRIBIDI_INTERFACE_AGE = 0;
dnl   FRIBIDI_INTERFACE_VERSION++;
dnl   FRIBIDI_BINARY_AGE++;
dnl
dnl If binary backwards compatibility has been broken:
dnl   FRIBIDI_BINARY_AGE = FRIBIDI_INTERFACE_AGE = 0;
dnl
m4_define(fribidi_major_version, 0)dnl
m4_define(fribidi_minor_version, 19)dnl
m4_define(fribidi_micro_version, 2)dnl
m4_define(fribidi_interface_version, 3)dnl
m4_define(fribidi_interface_age, 1)dnl
m4_define(fribidi_binary_age, 3)dnl
dnl
m4_define(fribidi_version,
	  m4_if(m4_eval(fribidi_micro_version()),0,
	    fribidi_major_version.fribidi_minor_version,
	    fribidi_major_version.fribidi_minor_version.fribidi_micro_version))dnl

AC_INIT([GNU FriBidi],fribidi_version(),[http://fribidi.org/bug])
AC_REVISION([$Id: configure.ac,v 1.28 2009-03-26 22:37:29 behdad Exp $])
AC_CONFIG_SRCDIR(lib/fribidi.h)
AC_CONFIG_HEADERS(config.h)
AM_INIT_AUTOMAKE([gnits])

FRIBIDI_MAJOR_VERSION=fribidi_major_version()
FRIBIDI_MINOR_VERSION=fribidi_minor_version()
FRIBIDI_MICRO_VERSION=fribidi_micro_version()
FRIBIDI_VERSION=fribidi_version()
FRIBIDI_INTERFACE_VERSION=fribidi_interface_version()
FRIBIDI_INTERFACE_AGE=fribidi_interface_age()
FRIBIDI_BINARY_AGE=fribidi_binary_age()

AC_SUBST(FRIBIDI_MAJOR_VERSION)
AC_SUBST(FRIBIDI_MINOR_VERSION)
AC_SUBST(FRIBIDI_MICRO_VERSION)
AC_SUBST(FRIBIDI_VERSION)
AC_SUBST(FRIBIDI_INTERFACE_VERSION)
AC_SUBST(FRIBIDI_INTERFACE_AGE)
AC_SUBST(FRIBIDI_BINARY_AGE)

dnl libtool versioning.
m4_define(lt_current, fribidi_interface_version())dnl
m4_define(lt_revision, fribidi_interface_age())dnl
m4_define(lt_age, fribidi_binary_age())dnl
LT_VERSION_INFO=lt_current():lt_revision():lt_age()
LT_CURRENT_MINUS_AGE=m4_eval(lt_current - lt_age)

AC_SUBST(LT_VERSION_INFO)
AC_SUBST(LT_CURRENT_MINUS_AGE)

dnl ==========================================================================

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL
AC_MSG_CHECKING([for native Win32])
  case $build_os in
    mingw*) os_win32=yes ;;
    *) os_win32=no ;;
  esac
AC_MSG_RESULT([$os_win32])
AM_CONDITIONAL(OS_WIN32, test "$os_win32" = "yes")
AC_MSG_CHECKING([for some Win32 platform])
  case $build_os in
    mingw*|cygwin*) platform_win32=yes ;;
    *) platform_win32=no ;;
  esac
AC_MSG_RESULT([$platform_win32])
AM_CONDITIONAL(PLATFORM_WIN32, test "$platform_win32" = "yes")


# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h strings.h inttypes.h wchar.h asm/page.h sys/times.h])

# Checks for typedefs and structures.
AC_C_CONST
AC_C_STRINGIZE
AC_CHECK_SIZEOF(int)
SIZEOF_INT=$ac_cv_sizeof_int
AC_SUBST(SIZEOF_INT)
AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(void *)
AC_CHECK_SIZEOF(wchar_t)

# Checks for compiler characteristics.
changequote(,)dnl
if test "x$GCC" = "xyes"; then
  CFLAGS="$CFLAGS -Wall -ansi "
fi
changequote([,])dnl

# Checks for library functions.
AC_CHECK_FUNCS([memmove memset strdup])
AC_TYPE_SIGNAL

# Checks for configure options

# --enable-debug
m4_define(debug_default,
          m4_if(m4_eval(fribidi_minor_version() % 2),1,yes,no))
AC_ARG_ENABLE(debug,
              AC_HELP_STRING([--enable-debug],
                             [turn on debugging @<:@default=debug_default()@:>@]),,
              enable_debug=debug_default())
if test x$enable_debug = xyes; then
	AC_DEFINE(DEBUG,1,
                  [Define to 1 if you want to include debug code in the library])
fi

# --disable-deprecated
AC_ARG_ENABLE(deprecated,
              AC_HELP_STRING([--disable-deprecated],
                             [do not build deprecated functionality]))
if test x$enable_deprecated = xno; then
	FRIBIDI_NO_DEPRECATED=1
else
	FRIBIDI_NO_DEPRECATED=0
fi
AC_SUBST(FRIBIDI_NO_DEPRECATED)

# --enable-malloc
AC_ARG_ENABLE(malloc,
              AC_HELP_STRING([--enable-malloc],
                             [use simple malloc instead of memory chunks]))
if test x$enable_malloc = xyes; then
	AC_DEFINE(USE_SIMPLE_MALLOC,1,
                  [Define to 1 if you want to use simple mallocs instead of memory chunks])
fi

# --disable-charsets
AC_ARG_ENABLE(charsets,
              AC_HELP_STRING([--disable-charsets],
                             [exclude charset conversion routines from library]))
if test x$enable_charsets = xno; then
	FRIBIDI_CHARSETS=0
else
	FRIBIDI_CHARSETS=1
fi
AC_SUBST(FRIBIDI_CHARSETS)
AM_CONDITIONAL(FRIBIDI_CHARSETS, test x$FRIBIDI_CHARSETS = x1)

# --with[out]-glib
AC_ARG_WITH(glib,
              AC_HELP_STRING([--with-glib=@<:@no/auto/yes@:>@],
                             [use Glib @<:@default=auto@:>@]))
GLIB_PACKAGE=glib-2.0
GLIB_MINVERSION=2.4
GLIB_LIBS=
GLIB_CFLAGS=
if test x$with_glib = xyes; then
	PKG_CHECK_MODULES(GLIB,$GLIB_PACKAGE >= $GLIB_MINVERSION)
	FRIBIDI_USE_GLIB=1
else
if test x$with_glib = xno; then
	FRIBIDI_USE_GLIB=0
else
	PKG_CHECK_MODULES(GLIB,$GLIB_PACKAGE >= $GLIB_MINVERSION,
		FRIBIDI_USE_GLIB=1,
		FRIBIDI_USE_GLIB=0)
fi
fi
if test x$FRIBIDI_USE_GLIB = x0; then
	GLIB_PACKAGE=
fi
AC_SUBST(FRIBIDI_USE_GLIB)
AM_CONDITIONAL(FRIBIDI_USE_GLIB, test x$FRIBIDI_USE_GLIB = x1)


MISC_CFLAGS="$GLIB_CFLAGS"
MISC_LIBS="$GLIB_LIBS"
MISC_PACKAGES="$GLIB_PACKAGE"
AC_SUBST(MISC_CFLAGS)
AC_SUBST(MISC_LIBS)
AC_SUBST(MISC_PACKAGES)

# Generate output
AC_CONFIG_FILES([fribidi.pc
		 lib/fribidi-config.h
		 Makefile
		 charset/Makefile
		 gen.tab/Makefile
		 lib/Makefile
		 bin/Makefile
		 doc/Makefile
		 test/Makefile])
AC_OUTPUT
