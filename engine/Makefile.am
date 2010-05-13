# vim:set noet ts=4:
#
# ibus-input-pad - The input pad for IBus
#
# Copyright (c) 2010 Takao Fujiwara <takao.fujiwara1@gmail.com>
# Copyright (c) 2010 Red Hat, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details. 
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

noinst_DIST = \
	$(NULL)

INCLUDES = \
	-I$(top_srcdir)                                                    \
	-I$(top_srcdir)/input-pad                                          \
	-DDATAROOTDIR=\""$(datarootdir)"\"                                 \
	$(NULL)

libexec_PROGRAMS = ibus-engine-input-pad

ibus_engine_input_pad_SOURCES = \
	engine.c                                                \
	engine.h                                                \
	main.c                                                  \
	$(NULL)

ibus_engine_input_pad_CFLAGS = \
	$(GLIB2_CFLAGS)                                         \
	$(IBUS_CFLAGS)                                          \
	$(NULL)

ibus_engine_input_pad_LDFLAGS = \
	-rpath $(libdir)/input-pad-$(libinput_pad_VERSION)      \
	$(NULL)

ibus_engine_input_pad_LDADD = \
	$(top_builddir)/input-pad/libinput-pad.la               \
	$(GLIB2_LIBS)                                           \
	$(IBUS_LIBS)                                            \
	$(NULL)

component_in_files = \
	input-pad.xml.in                                        \
	$(NULL)

component_DATA = $(component_in_files:.xml.in=.xml)
componentdir = $(datarootdir)/ibus/component

%.xml : %.xml.in
	@sed -e "s|\@GETTEXT_PACKAGE\@|$(GETTEXT_PACKAGE)|g"    \
	     -e "s|\@VERSION\@|$(VERSION)|g"                    \
	     -e "s|\@datarootdir\@|$(datarootdir)|g"            \
	     -e "s|\@libexecdir\@|$(libexecdir)|g"              \
	$< > $@

EXTRA_DIST = \
	$(component_in_files)                                   \
	$(NULL)

CLEANFILES = \
	$(component_DATA)                                       \
	$(NULL)
