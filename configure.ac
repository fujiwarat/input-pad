# vim:set et ts=4:
#
# input-pad - The input pad 
#
# Copyright (c) 2010-2011 Takao Fujiwara <takao.fujiwara1@gmail.com>
# Copyright (c) 2010-2011 Red Hat, Inc.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
# MA  02110-1301  USA

dnl - if not 1, append datestamp to the version number.
m4_define([pad_released], [1])
m4_define([pad_major_version], [1])
m4_define([pad_minor_version], [0])
m4_define([pad_micro_version], [0])
m4_define(pad_maybe_datestamp,
    m4_esyscmd([if test x]pad_released[ != x1; then date +.%Y%m%d | tr -d '\n\r'; fi]))

m4_define([pad_version],
    pad_major_version.pad_minor_version.pad_micro_version[]pad_maybe_datestamp)

AC_INIT([input-pad], [pad_version], [http://code.google.com/p/input-pad/issues/entry],[input-pad])
AM_INIT_AUTOMAKE([1.10])
AC_GNU_SOURCE

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

dnl - VERSION
libinput_pad_VERSION="pad_major_version.pad_minor_version"
AC_SUBST(libinput_pad_VERSION)
AC_MSG_NOTICE([$PACKAGE sub version $libinput_pad_VERSION])

dnl If only source code changed, lt_revision + 1
dnl If any interface added, lt_age + 1
dnl If any interfaces changed or removed, lt_current + 1, lt_revision = 0, lt_age = 0
dnl libtool versionstring == [lt_current - lt_age].[lt_age].[lt_revision]
m4_define([lt_current], [pad_major_version])
m4_define([lt_revision], [pad_minor_version])
m4_define([lt_age], [0])
LT_VERSION_INFO="lt_current:lt_revision:lt_age"
AC_SUBST(LT_VERSION_INFO)
AC_MSG_NOTICE([library version $LT_VERSION_INFO])

dnl - define PACKAGE_VERSION_* variables
AM_SANITY_CHECK
AM_MAINTAINER_MODE
AM_DISABLE_STATIC
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_ISC_POSIX
AC_HEADER_STDC
AM_PROG_LIBTOOL
IT_PROG_INTLTOOL([0.35.0])

dnl - For dislpay Date
m4_define(pad_datedisplay,
    m4_esyscmd(date '+%a %b %d %Y' | tr -d '\n\r'))
DATE_DISPLAY="pad_datedisplay"
AC_SUBST(DATE_DISPLAY)

dnl - pkgconfig
AM_PATH_GLIB_2_0
PKG_CHECK_MODULES(GLIB2, [
    glib-2.0 >= 2.8
])

dnl - check gtk
AC_ARG_WITH([gtk],
    [AS_HELP_STRING([--with-gtk=2.0|3.0],
                    [Select GTK+ version. The default is 2.0])],
    [case "$with_gtk" in
        2.0|3.0) ;;
        *) AC_MSG_ERROR([invalid gtk version specified]) ;;
     esac],
    [with_gtk=2.0]
)

case "$with_gtk" in
    2.0)
        PKG_CHECK_MODULES(GTK2, [
            gtk+-2.0
        ])
        AM_CONDITIONAL(HAVE_GTK3, false)
        AM_CONDITIONAL(HAVE_GTK2, true)
        GTK_API_VERSION=2.0
        ;;
    3.0)
        PKG_CHECK_MODULES(GTK3, [
            gtk+-3.0
        ])
        AM_CONDITIONAL(HAVE_GTK3, true)
        AM_CONDITIONAL(HAVE_GTK2, false)
        GTK_API_VERSION=3.0
    ;;
esac
AC_SUBST(GTK_API_VERSION)

PKG_CHECK_MODULES(LIBXML2, [
    libxml-2.0 >= 2.0
])

PKG_CHECK_MODULES(X11, [
    x11
])

PKG_CHECK_MODULES(XKB, [
    xkbfile
])

PKG_CHECK_MODULES(LIBXKLAVIER,
    libxklavier >= 4.0,
    have_libxklavier=yes,
    have_libxklavier=no
)
if test "x$have_libxklavier" = "xyes" ; then
    AC_DEFINE(HAVE_LIBXKLAVIER, [1], [Define if we have libxklavier])
fi

dnl - check eek
AC_MSG_CHECKING([whether you enable libeek])
AC_ARG_ENABLE(eek,
              AS_HELP_STRING([--enable-eek=no/yes],
                             [Draw keyboard using libeek default=no]),
              enable_eek=yes,
              enable_eek=no)
AC_MSG_RESULT($enable_eek)

if test x"$enable_eek" = xyes; then
    dnl - check eek is available
    PKG_CHECK_MODULES(EEK, [
        eek-gtk-0.90 eek-xkl-0.90
	],, AC_MSG_ERROR([libeek not found]))
    AM_CONDITIONAL(HAVE_EEK, true)
else
    AM_CONDITIONAL(HAVE_EEK, false)
fi

dnl - check file path of glib-genmarshal
AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)

dnl - check python
AC_MSG_CHECKING([where you enable python])
AC_ARG_ENABLE(python,
              AS_HELP_STRING([--enable-python=no/yes],
                             [Enable Python wrapper default=yes]),,
              enable_python=yes)
AC_MSG_RESULT($enable_python)

if test x"$enable_python" = xyes; then
    AM_CONDITIONAL(HAVE_PYTHON, true)
    AM_PATH_PYTHON([2.5], [], [])
    AC_PATH_PROG(PYTHON_CONFIG, python$PYTHON_VERSION-config)
    if test x"$PYTHON_CONFIG" = x""; then
        AC_PATH_PROG(PYTHON_CONFIG, python-config-$PYTHON_VERSION)
    fi
    if test x"$PYTHON_CONFIG" != x""; then
        PYTHON_CFLAGS=`$PYTHON_CONFIG --includes`
        PYTHON_LIBS=`$PYTHON_CONFIG --libs`
    fi
    AC_MSG_NOTICE([PYTHON_CFLAGS=\"$PYTHON_CFLAGS\"])
    AC_MSG_NOTICE([PYTHON_LIBS=\"$PYTHON_LIBS\"])
    if test x"$PYTHON_CFLAGS" = x -o x"$PYTHON_LIBS" = x ; then
        AC_MSG_ERROR([Could not find PYTHON_CFLAGS or PYTHON_LIBS])
    fi

    AC_PATH_PROG(SWIG, swig)
    if test x"$SWIG" = x""; then
        AC_MSG_ERROR([Could not find swig])
    fi
else
    AM_CONDITIONAL(HAVE_PYTHON, false)
fi
AC_SUBST(PYTHON_CFLAGS)
AC_SUBST(PYTHON_LIBS)
AC_SUBST(SWIG)

dnl - check XTest module
AC_MSG_CHECKING([whether you enable xtest module])
AC_ARG_ENABLE(xtest,
              AS_HELP_STRING([--enable-xtest=no/yes],
                             [Send keyboard event with xtest. default=no]),
              enable_xtest=yes,
              enable_xtest=no)
AC_MSG_RESULT($enable_xtest)
AM_CONDITIONAL(HAVE_XTEST, false)

if test x"$enable_xtest" = xyes; then
    backup_LIBS=$LIBS
    if test "x$XTEST_LIBS" = "x"  ; then
        XTEST_LIBS="-lXtst"
    fi
    LIBS="$LIBS $XTEST_LIBS"
    AC_CHECK_LIB(Xtst, XTestFakeKeyEvent,
                 dnl - Do not put NULL string here so that LIBS is not overrided.
                [AC_MSG_NOTICE([use $XTEST_LIBS for XTEST module])
                 AM_CONDITIONAL(HAVE_XTEST, true)],
                [AC_MSG_NOTICE([if you want to enable X11 XTEST module, install libXtst-devel package])])
    LIBS=$backup_LIBS
fi
AC_SUBST(XTEST_LIBS)

dnl - define GETTEXT_* variables
GETTEXT_PACKAGE=input-pad
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package])

AM_GLIB_GNU_GETTEXT
AM_GLIB_DEFINE_LOCALEDIR(INPUT_PAD_LOCALEDIR)

dnl - OUTPUT files
AC_CONFIG_FILES([ po/Makefile.in
Makefile
input-pad.pc
input-pad.spec
data/Makefile
input-pad/Makefile
pyinput-pad/Makefile
pyinput-pad/pyinput_pad.pth
])

AC_OUTPUT
echo "
                input-pad version pad_version


libinput_pad version:    $libinput_pad_VERSION
GTK+ version:            $GTK_API_VERSION
Have libxklavier:        $have_libxklavier
Enable EEK               $enable_eek
Enable Python            $enable_python
Enable XTEST             $enable_xtest
lt version info          $LT_VERSION_INFO
"
