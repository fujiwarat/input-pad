# vim:set et ts=4:
#
# input-pad - The input pad
#
# Copyright (c) 2010-2015 Takao Fujiwara <takao.fujiwara1@gmail.com>
# Copyright (c) 2010-2015 Red Hat, Inc.
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
m4_define([pad_released], [0])
m4_define([pad_major_version], [1])
m4_define([pad_minor_version], [0])
m4_define([pad_micro_version], [99])
m4_define([pad_abi_current], [1])
m4_define([pad_abi_revision],
          [m4_eval(100 * pad_minor_version + pad_micro_version)])
m4_define([pad_abi_age], [0])
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
libinput_pad_VERSION="pad_major_version.pad_abi_current"
AC_SUBST(libinput_pad_VERSION)
AC_MSG_NOTICE([$PACKAGE sub version $libinput_pad_VERSION])
libinput_pad_API_VERSION="pad_major_version.0"
AC_SUBST(libinput_pad_API_VERSION)

dnl libtool versioning
dnl
dnl If LT_VERSION_INFO="lt_current:lt_revision:lt_age", libinputpad is
dnl libinputpad-API_VERSION.so.(lt_current - lt_age).lt_age.lt_revision
dnl If the abi is changed, but it is compatible with last version,
dnl pad_abi_current++, pad_abi_age++;
dnl If the abi is not compatible with last version,
dnl pad_abi_current++, pad_abi_age = 0;
m4_define([lt_current], [pad_abi_current])
m4_define([lt_revision], [pad_abi_revision])
m4_define([lt_age], [pad_abi_age])
LT_VERSION_INFO="lt_current:lt_revision:lt_age"
AC_SUBST(LT_VERSION_INFO)
AC_MSG_NOTICE([library version $LT_VERSION_INFO])

dnl - define PACKAGE_VERSION_* variables
AM_SANITY_CHECK
AM_MAINTAINER_MODE(enable)
AC_DISABLE_STATIC
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_HEADER_STDC
LT_INIT
IT_PROG_INTLTOOL([0.35.0])

dnl - For dislpay Date
m4_define(pad_datedisplay,
    m4_esyscmd(date '+%a %b %d %Y' | tr -d '\n\r'))
DATE_DISPLAY="pad_datedisplay"
AC_SUBST(DATE_DISPLAY)

dnl - pkgconfig
AM_PATH_GLIB_2_0(2.37.0)
PKG_CHECK_MODULES(GLIB2, [
    glib-2.0 >= 2.37
])

PKG_CHECK_MODULES(GMODULE2,
    [gmodule-2.0],
    [],
    [AC_MSG_NOTICE([gmodule-2.0 is required by glib-2.0 >= 2.30])]
)

PKG_CHECK_MODULES(GTK3, [
    gtk+-3.0 >= 3.12
])
GTK_API_VERSION=3.0
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
              [],
              enable_eek=no)
AC_MSG_RESULT($enable_eek)

if test x"$enable_eek" = xyes; then
    dnl - check eek is available
    PKG_CHECK_MODULES(EEK, [
        eek-gtk-0.90 >= 1.0.6, eek-xkl-0.90 >= 1.0.6
	],, AC_MSG_ERROR([libeek not found]))
    AM_CONDITIONAL(HAVE_EEK, true)
else
    AM_CONDITIONAL(HAVE_EEK, false)
fi

dnl - check file path of glib-genmarshal
AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)

dnl - GObject introspection
GOBJECT_INTROSPECTION_CHECK([0.6.8])

INPUT_PAD_GIR_SCANNERFLAGS=
if test x"$found_introspection" = x"yes" ; then
    INPUT_PAD_GIR_SCANNERFLAGS="--identifier-prefix=InputPad --symbol-prefix=input_pad"
    PKG_CHECK_EXISTS([gobject-introspection-1.0 >= 0.9.6],
                     [gir_symbol_prefix=yes],
                     [gir_symbol_prefix=no])
    if test x"$gir_symbol_prefix" = x"no" ; then
        INPUT_PAD_GIR_SCANNERFLAGS="--strip-prefix=InputPad"
    fi
fi
AC_SUBST(IBUS_GIR_SCANNERFLAGS)

dnl - check XTest module
AC_MSG_CHECKING([whether you enable xtest module])
AC_ARG_ENABLE(xtest,
              AS_HELP_STRING([--enable-xtest=no/yes],
                             [Send keyboard event with xtest. default=yes]),
              [],
              enable_xtest=yes)
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
])

AC_OUTPUT
echo "
                input-pad version pad_version


libinput_pad version:    $libinput_pad_VERSION
GTK+ version:            $GTK_API_VERSION
Have libxklavier:        $have_libxklavier
Enable EEK               $enable_eek
Enable PyGObject3        $found_introspection
GIR scannerflags         "$INPUT_PAD_GIR_SCANNERFLAGS"
Enable XTEST             $enable_xtest
lt version info          $LT_VERSION_INFO
"
