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

MODULE_DIR = $(libdir)/$(PACKAGE)-$(libinput_pad_VERSION)/modules
MODULE_KBDUI_DIR = $(MODULE_DIR)/kbdui
MODULE_XKEYSEND_DIR = $(MODULE_DIR)/xkeysend

INCLUDES = \
	-I$(top_srcdir)                                                    \
	-DINPUT_PAD_PAD_SYSTEM_DIR=\""$(pkgdatadir)/pad"\"                 \
	-DINPUT_PAD_UI_GTK_DIR=\""$(pkgdatadir)/ui/gtk"\"                  \
	-DMODULE_KBDUI_DIR=\""$(MODULE_KBDUI_DIR)"\"                       \
	-DDATAROOTDIR=\""$(datarootdir)"\"                                 \
	$(NULL)

BUILT_SOURCES = \
	input-pad-marshal.c                                     \
	input-pad-marshal.h                                     \
	$(NULL)

libinput_pad_publicdir = $(includedir)/$(PACKAGE)-$(libinput_pad_VERSION)
libinput_pad_public_HEADERS = \
	input-pad-group.h                                       \
	input-pad-kbdui-gtk.h                                   \
	input-pad-window-gtk.h                                  \
	input-pad.h                                             \
	$(NULL)

libinput_pad_LTLIBRARIES = libinput-pad.la
libinput_paddir = $(libdir)

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
	kbdui-gtk.c                                             \
	parse-pad.c                                             \
	unicode_block.h                                         \
	window-gtk.c                                            \
	$(NULL)

libinput_pad_la_CFLAGS = \
	$(LIBXML2_CFLAGS)                                       \
	$(X11_CFLAGS)                                           \
	$(XKB_CFLAGS)                                           \
	$(LIBXKLAVIER_CFLAGS)                                   \
	$(NULL)

libinput_pad_la_LIBADD = \
	$(LIBXML2_LIBS)                                         \
	$(X11_LIBS)                                             \
	$(XKB_LIBS)                                             \
	$(LIBXKLAVIER_LIBS)                                     \
	$(NULL)

libinput_pad_la_LDFLAGS = \
	-export-dynamic                                         \
	-version-info $(LT_VERSION_INFO)                        \
	$(no_undefined)                                         \
	$(NULL)

if HAVE_GTK3
    libinput_pad_la_CFLAGS += $(GTK3_CFLAGS)
    libinput_pad_la_LIBADD += $(GTK3_LIBS)
else
    libinput_pad_la_CFLAGS += $(GTK2_CFLAGS)
    libinput_pad_la_LIBADD += $(GTK2_LIBS)
endif

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
	-DMODULE_XTEST_GDK_DIR=\""$(MODULE_XKEYSEND_DIR)"\"     \
	$(NULL)

libinput_pad_xtest_gdk_LTLIBRARIES = libinput-pad-xtest-gdk.la
libinput_pad_xtest_gdkdir = $(MODULE_XKEYSEND_DIR)

libinput_pad_xtest_gdk_la_SOURCES = \
	xtest-gdk.c                                             \
	$(NULL)

libinput_pad_xtest_gdk_la_CFLAGS = \
	$(X11_CFLAGS)                                           \
	$(NULL)

libinput_pad_xtest_gdk_la_LIBADD = \
	$(builddir)/libinput-pad.la                             \
	$(X11_LIBS)                                             \
	$(XTEST_LIBS)                                           \
	$(NULL)

libinput_pad_xtest_gdk_la_LDFLAGS = \
	-export-dynamic                                         \
	-avoid-version                                          \
	$(no_undefined)                                         \
	$(NULL)

if HAVE_GTK3
    libinput_pad_xtest_gdk_la_CFLAGS += $(GTK3_CFLAGS)
    libinput_pad_xtest_gdk_la_LIBADD += $(GTK3_LIBS)
else
    libinput_pad_xtest_gdk_la_CFLAGS += $(GTK2_CFLAGS)
    libinput_pad_xtest_gdk_la_LIBADD += $(GTK2_LIBS)
endif

endif

if HAVE_EEK
MODULE_EEK_GTK_BASE = eek-gtk

INCLUDES += \
	-DMODULE_EEK_GTK_BASE=\""$(MODULE_EEK_GTK_BASE)"\"      \
	$(NULL)

libinput_pad_eek_gtk_LTLIBRARIES = libinput-pad-eek-gtk.la
libinput_pad_eek_gtkdir = $(MODULE_KBDUI_DIR)

libinput_pad_eek_gtk_la_SOURCES = \
	eek-gtk.c                                               \
	eek-gtk.h                                               \
	$(NULL)

libinput_pad_eek_gtk_la_CFLAGS = \
	$(EEK_CFLAGS)                                           \
	$(NULL)

libinput_pad_eek_gtk_la_LIBADD = \
	$(builddir)/libinput-pad.la                             \
	$(EEK_LIBS)                                             \
	$(NULL)

libinput_pad_eek_gtk_la_LDFLAGS = \
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
