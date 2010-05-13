# vim:set noet ts=4:
#
# input-pad - The input pad 
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

ui_gtk_DATA = input-pad.ui
ui_gtkdir = $(pkgdatadir)/ui/gtk

icon_DATA = input-pad.png
icondir = $(datarootdir)/pixmaps

paddir = $(pkgdatadir)/pad
pad_in_files = \
	group01-base.xml.in                                     \
	group02-ja.xml.in                                       \
	group02-zh.xml.in                                       \
	group90-keyboard.xml.in                                 \
	$(NULL)

pad_DATA = $(pad_in_files:.xml.in=.xml)

%.xml : %.xml.in
	@sed -e "s/_name/name/g" $< > $@

CLEANFILES= \
	$(pad_DATA)                                             \
	$(NULL)

#desktopdir = $(datarootdir)/applications
desktop_in_files = input-pad.desktop.in
#desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
#@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	$(desktop_in_files)                                     \
	$(icon_DATA)                                            \
	$(pad_in_files)                                         \
	$(ui_gtk_DATA)                                          \
	char2hexstr.py                                          \
	$(NULL)

