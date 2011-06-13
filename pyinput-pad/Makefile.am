# vim:set noet ts=4:
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

noinst_DIST = \
	$(NULL)

INCLUDES = \
	-I$(top_srcdir)                                                    \
	-I$(top_srcdir)/input-pad                                          \
	$(NULL)

pyinput_pad_swig_files = \
	input_pad_group.i                                       \
	input_pad_window_gtk.i                                  \
	input_pad.i                                             \
	$(NULL)

pyinput_pad_PYTHON = \
	$(pyinput_pad_swig_files:.i=.py)                        \
	$(NULL)

pyinput_pad_LTLIBRARIES = \
	_input_pad_group.la                                     \
	_input_pad_window_gtk.la                                \
	_input_pad.la                                           \
	$(NULL)

pyinput_paddir = $(pyexecdir)/$(PACKAGE)-$(libinput_pad_VERSION)

_input_pad_la_SOURCES = \
	$(NULL)

nodist__input_pad_la_SOURCES = \
	input_pad_wrap.c                                        \
	$(NULL)

_input_pad_la_CFLAGS = \
	$(PYTHON_CFLAGS)                                        \
	$(NULL)

_input_pad_la_LDFLAGS = \
	$(PYTHON_LIBS)                                          \
	-avoid-version                                          \
	-module                                                 \
	$(NULL)

_input_pad_la_LIBADD = \
	$(top_builddir)/input-pad/libinput-pad.la               \
	$(NULL)

if HAVE_GTK3
    _input_pad_la_CFLAGS += $(GTK3_CFLAGS)
    _input_pad_la_LDFLAGS += $(GTK3_LIBS)
else
    _input_pad_la_CFLAGS += $(GTK2_CFLAGS)
    _input_pad_la_LDFLAGS += $(GTK2_LIBS)
endif

_input_pad_group_la_SOURCES = \
	$(NULL)

nodist__input_pad_group_la_SOURCES = \
	input_pad_group_wrap.c                                  \
	$(NULL)

_input_pad_group_la_CFLAGS = \
	$(PYTHON_CFLAGS)                                        \
	$(NULL)

_input_pad_group_la_LDFLAGS = \
	$(PYTHON_LIBS)                                          \
	-avoid-version                                          \
	-module                                                 \
	$(NULL)

_input_pad_group_la_LIBADD = \
	$(top_builddir)/input-pad/libinput-pad.la               \
	$(NULL)

_input_pad_window_gtk_la_SOURCES = \
	$(NULL)

nodist__input_pad_window_gtk_la_SOURCES = \
	input_pad_window_gtk_wrap.c                             \
	$(NULL)

_input_pad_window_gtk_la_CFLAGS = \
	$(PYTHON_CFLAGS)                                        \
	$(NULL)

_input_pad_window_gtk_la_LDFLAGS = \
	$(PYTHON_LIBS)                                          \
	-avoid-version                                          \
	-module                                                 \
	$(NULL)

_input_pad_window_gtk_la_LIBADD = \
	$(top_builddir)/input-pad/libinput-pad.la               \
	$(NULL)

if HAVE_GTK3
    _input_pad_window_gtk_la_CFLAGS += $(GTK3_CFLAGS)
    _input_pad_window_gtk_la_LDFLAGS += $(GTK3_LIBS)
else
    _input_pad_window_gtk_la_CFLAGS += $(GTK2_CFLAGS)
    _input_pad_window_gtk_la_LDFLAGS += $(GTK2_LIBS)
endif

%.py %_wrap.c: %.i
	@f=`basename "$<"`;                                     \
	if [ x"$$f" = xinput_pad_window_gtk.i ] ; then          \
	  sed -e "/G_BEGIN_DECLS/d" -e "/G_END_DECLS/d"         \
	      -e "/G_MODULE_EXPORT/d"                           \
            $(top_srcdir)/input-pad/input-pad-window-gtk.h      \
          > $(builddir)/input-pad-window-gtk-swig.h;            \
	fi;                                                     \
	$(SWIG) -python -I$(top_srcdir)/input-pad $<            \
	$(NULL)

pyinput_pad_pth_in_files = \
	pyinput_pad.pth.in                                      \
	$(NULL)

pyinput_pad_pth_DATA = $(pyinput_pad_pth_in_files:.pth.in=.pth)
pyinput_pad_pthdir = $(pyexecdir)

CLEANFILES = \
	$(pyinput_pad_PYTHON)                                   \
	$(pyinput_pad_pth_DATA)                                 \
	input-pad-window-gtk-swig.h                             \
	input_pad_wrap.c                                        \
	input_pad_group_wrap.c                                  \
	input_pad_window_gtk_wrap.c                             \
	$(NULL)

EXTRA_DIST = \
	$(pyinput_pad_swig_files)                               \
	$(pyinput_pad_pth_in_files)                             \
	test_input_pad.py                                       \
	test_input_pad_group.py                                 \
	test_input_pad_gtk.py                                   \
	$(NULL)
