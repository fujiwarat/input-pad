# vim:set noet ts=4:
#
# input-pad - The input pad 
#
# Copyright (c) 2010 Takao Fujiwara <takao.fujiwara1@gmail.com>
# Copyright (c) 2010 Red Hat, Inc.
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
	-DINPUT_PAD_PAD_SYSTEM_DIR=\""$(pkgdatadir)/pad"\"                 \
	-DINPUT_PAD_UI_GTK_DIR=\""$(pkgdatadir)/ui/gtk"\"                  \
	-DDATAROOTDIR=\""$(datarootdir)"\"                                 \
	$(NULL)

BUILT_SOURCES = \
	input-pad-marshal.c                                     \
	input-pad-marshal.h                                     \
	$(NULL)

libinput_pad_publicdir = $(includedir)/$(PACKAGE)-$(libinput_pad_VERSION)
libinput_pad_public_HEADERS = \
	input-pad-group.h                                       \
	input-pad-window-gtk.h                                  \
	input-pad.h                                             \
	$(NULL)

libinput_pad_LTLIBRARIES = libinput-pad.la
libinput_paddir = $(libdir)/$(PACKAGE)-$(libinput_pad_VERSION)

libinput_pad_la_SOURCES = \
	$(BUILT_SOURCES)                                        \
	$(libinput_pad_public_HEADERS)                          \
	button-gtk.c                                            \
	button-gtk.h                                            \
	combobox-gtk.c                                          \
	combobox-gtk.h                                          \
	geometry-gdk.c                                          \
	geometry-gdk.h                                          \
	geometry-xkb.h                                          \
	i18n.h                                                  \
	input-pad-private.h                                     \
	parse-pad.c                                             \
	unicode_block.h                                         \
	window-gtk.c                                            \
	$(NULL)

libinput_pad_la_CFLAGS = \
	$(GTK2_CFLAGS)                                          \
	$(LIBXML2_CFLAGS)                                       \
	$(X11_CFLAGS)                                           \
	$(XKB_CFLAGS)                                           \
	$(LIBXKLAVIER_CFLAGS)                                   \
	$(NULL)

libinput_pad_la_LIBADD = \
	$(GTK2_LIBS)                                            \
	$(LIBXML2_LIBS)                                         \
	$(X11_LIBS)                                             \
	$(XKB_LIBS)                                             \
	$(LIBXKLAVIER_LIBS)                                     \
	$(NULL)

libinput_pad_la_LDFLAGS = \
	-export-dynamic                                         \
	-avoid-version                                          \
	$(no_undefined)                                         \
	$(NULL)

input-pad-marshal.h: input-pad-marshal.list
	@$(GLIB_GENMARSHAL) $< --prefix=INPUT_PAD --header > $@ \
	$(NULL)

input-pad-marshal.c: input-pad-marshal.list
	@echo "#include \"input-pad-marshal.h\"" > $@ &&        \
	$(GLIB_GENMARSHAL) $< --prefix=INPUT_PAD --body >> $@   \
	$(NULL)

if HAVE_XTEST
MODULE_XTEST_GDK_BASE = input-pad-xtest-gdk

INCLUDES += \
	-DMODULE_XTEST_GDK_BASE=\""$(MODULE_XTEST_GDK_BASE)"\"  \
	-DMODULE_XTEST_GDK_DIR=\""$(libinput_paddir)"\"         \
	$(NULL)

libinput_pad_LTLIBRARIES += libinput-pad-xtest-gdk.la

libinput_pad_xtest_gdk_la_SOURCES = \
	xtest-gdk.c                                             \
	$(NULL)

libinput_pad_xtest_gdk_la_CFLAGS = \
	$(GTK2_CFLAGS)                                          \
	$(X11_CFLAGS)                                           \
	$(NULL)

libinput_pad_xtest_gdk_la_LIBADD = \
	$(builddir)/libinput-pad.la                             \
	$(GTK2_LIBS)                                            \
	$(X11_LIBS)                                             \
	$(XTEST_LIBS)                                           \
	$(NULL)

libinput_pad_xtest_gdk_la_LDFLAGS = \
	-export-dynamic                                         \
	-avoid-version                                          \
	$(no_undefined)                                         \
	$(NULL)
endif

bin_PROGRAMS = \
	input-pad                                               \
	$(NULL)

input_pad_SOURCES = \
	input-pad-main.c                                        \
	$(NULL)

input_pad_CFLAGS = \
	$(NULL)

input_pad_LDADD = \
	$(builddir)/libinput-pad.la                             \
	$(NULL)

CLEANFILES = \
	$(BUILT_SOURCES)                                        \
	$(NULL)

EXTRA_DIST = \
	input-pad-marshal.list                                  \
	unicode_block.sh                                        \
	$(NULL)
