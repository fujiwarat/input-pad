# vim:set noet ts=4:
#
# input-pad - The input pad
#
# Copyright (c) 2010-2014 Takao Fujiwara <takao.fujiwara1@gmail.com>
# Copyright (c) 2010-2014 Red Hat, Inc.
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

# gobject introspection
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_SCANNER_ARGS =
INTROSPECTION_COMPILER_ARGS = \
    --includedir=$(srcdir)    \
    --includedir=.            \
    $(NULL)
INTROSPECTION_GIRS =
CLEANFILES =

MODULE_DIR = $(libdir)/$(PACKAGE)-$(libinput_pad_VERSION)/modules
MODULE_KBDUI_DIR = $(MODULE_DIR)/kbdui
MODULE_XKEYSEND_DIR = $(MODULE_DIR)/xkeysend

AM_CPPFLAGS = \
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

MAINTAINERCLEANFILES = resources.c

libinput_pad_publicdir = $(includedir)/$(PACKAGE)-$(libinput_pad_VERSION)
libinput_pad_public_HEADERS = \
	input-pad-group.h                                       \
	input-pad-kbdui-gtk.h                                   \
	input-pad-window-gtk.h                                  \
	input-pad.h                                             \
	$(NULL)

#libinput_pad_LTLIBRARIES = libinput-pad-$(libinput_pad_API_VERSION).la
libinput_pad_LTLIBRARIES = libinput-pad-1.0.la
libinput_paddir = $(libdir)

libinput_pad_1_0_la_SOURCES = \
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
	resources.c                                             \
	unicode_block.h                                         \
	window-gtk.c                                            \
	$(NULL)

libinput_pad_1_0_la_CFLAGS = \
	$(GTK3_CFLAGS)                                          \
	$(GMODULE2_CFLAGS)                                      \
	$(LIBXML2_CFLAGS)                                       \
	$(X11_CFLAGS)                                           \
	$(XKB_CFLAGS)                                           \
	$(LIBXKLAVIER_CFLAGS)                                   \
	$(NULL)

libinput_pad_1_0_la_LIBADD = \
	$(GTK3_LIBS)                                            \
	$(GMODULE2_LIBS)                                        \
	$(LIBXML2_LIBS)                                         \
	$(X11_LIBS)                                             \
	$(XKB_LIBS)                                             \
	$(LIBXKLAVIER_LIBS)                                     \
	$(NULL)

libinput_pad_1_0_la_LDFLAGS = \
	-export-dynamic                                         \
	-version-info $(LT_VERSION_INFO)                        \
	$(no_undefined)                                         \
	$(NULL)

resources.c: input-pad.gresource.xml
	$(GLIB_COMPILE_RESOURCES) $< --target=$@ --generate-source

input-pad-marshal.h: input-pad-marshal.list
	@$(GLIB_GENMARSHAL) $< --prefix=INPUT_PAD --header > $@ \
	$(NULL)

input-pad-marshal.c: input-pad-marshal.list
	@echo "#include \"input-pad-marshal.h\"" > $@ &&        \
	$(GLIB_GENMARSHAL) $< --prefix=INPUT_PAD --body >> $@   \
	$(NULL)

if HAVE_XTEST
MODULE_XTEST_GDK_BASE = input-pad-xtest-gdk

AM_CPPFLAGS += \
	-DMODULE_XTEST_GDK_BASE=\""$(MODULE_XTEST_GDK_BASE)"\"  \
	-DMODULE_XTEST_GDK_DIR=\""$(MODULE_XKEYSEND_DIR)"\"     \
	$(NULL)

libinput_pad_xtest_gdk_LTLIBRARIES = libinput-pad-xtest-gdk.la
libinput_pad_xtest_gdkdir = $(MODULE_XKEYSEND_DIR)

libinput_pad_xtest_gdk_la_SOURCES = \
	xtest-gdk.c                                             \
	$(NULL)

libinput_pad_xtest_gdk_la_CFLAGS = \
	$(GTK3_CFLAGS)                                          \
	$(X11_CFLAGS)                                           \
	$(NULL)

libinput_pad_xtest_gdk_la_LIBADD = \
	$(builddir)/libinput-pad-$(libinput_pad_API_VERSION).la \
	$(GTK3_LIBS)                                            \
	$(X11_LIBS)                                             \
	$(XTEST_LIBS)                                           \
	$(NULL)

libinput_pad_xtest_gdk_la_LDFLAGS = \
	-export-dynamic                                         \
	-avoid-version                                          \
	$(no_undefined)                                         \
	$(NULL)
endif

if HAVE_EEK
MODULE_EEK_GTK_BASE = eek-gtk

AM_CPPFLAGS += \
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
	$(builddir)/libinput-pad-$(libinput_pad_API_VERSION).la \
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
	$(builddir)/libinput-pad-$(libinput_pad_API_VERSION).la \
	$(NULL)

if HAVE_INTROSPECTION
introspection_files = \
    $(libinput_pad_1_0_la_SOURCES)                                  \
    $(NULL)
InputPad-$(libinput_pad_VERSION).gir: $(libinput_pad_LTLIBRARIES) Makefile
InputPad_1_1_gir_SCANNERFLAGS = \
    --pkg-export=input-pad                                      \
    --warn-all                                                  \
    $(INPUT_PAD_GIR_SCANNERFLAGS)
InputPad_1_1_gir_INCLUDES = GLib-2.0 GObject-2.0 Gio-2.0 Gtk-3.0
InputPad_1_1_gir_LIBS = $(libinput_pad_LTLIBRARIES)
InputPad_1_1_gir_FILES = $(addprefix $(srcdir)/,$(introspection_files))
InputPad_1_1_gir_CFLAGS = -DIBUS_COMPILATION
INTROSPECTION_GIRS += InputPad-$(libinput_pad_VERSION).gir

girdir = $(datadir)/gir-1.0
dist_gir_DATA = $(INTROSPECTION_GIRS)

typelibsdir = $(libdir)/girepository-1.0
typelibs_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(dist_gir_DATA) $(typelibs_DATA)
endif

man_one_in_files = input-pad.1.in
man_one_files = $(man_one_in_files:.1.in=.1)
man_one_DATA =$(man_one_files:.1=.1.gz)
man_onedir = $(mandir)/man1
%.1: %.1.in
	$(AM_V_GEN) sed \
	    -e 's|@VERSION[@]|$(VERSION)|g' $< > $@.tmp && \
	    mv $@.tmp $@
%.1.gz: %.1
	$(AM_V_GEN) gzip -c $< > $@.tmp && mv $@.tmp $@

CLEANFILES += \
	$(BUILT_SOURCES)                                        \
	$(man_one_files)                                        \
	$(man_one_DATA)                                         \
	$(NULL)

EXTRA_DIST = \
	$(man_one_in_files)                                     \
	app-menu.ui                                             \
	dialog.ui                                               \
	input-pad.gresource.xml                                 \
	input-pad-marshal.list                                  \
	unicode_block.sh                                        \
	window-gtk.ui                                           \
	$(NULL)
