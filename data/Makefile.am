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

ui_gtk_DATA = input-pad.ui
ui_gtkdir = $(pkgdatadir)/ui/gtk

icon_DATA = input-pad.png
icondir = $(datarootdir)/pixmaps

paddir = $(pkgdatadir)/pad
pad_in_files = \
	base.xml.in                                             \
	latin.xml.in                                            \
	runr.xml.in                                             \
	goth.xml.in                                             \
	ja.xml.in                                               \
	zh.xml.in                                               \
	str.xml.in                                              \
	w3c.xml.in                                              \
	cmd.xml.in                                              \
	$(NULL)

pad_in_in_files = \
	alphabet.xml.in.in                                      \
	$(NULL)

pad_no_sorted = $(pad_in_files:.xml.in=.xml)
pad_no_sorted += $(pad_in_in_files:.xml.in.in=.xml.in)
pad_no_sorted += $(pad_in_in_files:.xml.in.in=.xml)
pad_DATA = \
	group01-base.xml                                        \
	group02-alphabet.xml                                    \
	group02-ja.xml                                          \
	group02-zh.xml                                          \
	group50-str.xml                                         \
	group60-w3c.xml                                         \
	group70-cmd.xml                                         \
	$(NULL)

group01-base.xml: base.xml
	@cp $< $@
group02-alphabet.xml: alphabet.xml
	@cp $< $@
group02-ja.xml: ja.xml
	@cp $< $@
group02-zh.xml: zh.xml
	@cp $< $@
group50-str.xml: str.xml
	@cp $< $@
group60-w3c.xml: w3c.xml
	@cp $< $@
group70-cmd.xml: cmd.xml
	@cp $< $@


alphabet.xml.in : alphabet.xml.in.in
	@sed -e "s/_name/name/g"                                \
	     -e "s/_comment/comment/g"                          \
	     -e "s/_label/label/g"                              \
	$< > $@

ALPHABET_TABLE_FILES = \
	latin.xml                                               \
	runr.xml                                                \
	goth.xml                                                \
	$(NULL)

alphabet.xml : alphabet.xml.in $(ALPHABET_TABLE_FILES)
	@sed -e "/@latin@/r latin.xml"                          \
	     -e "/@runr@/r runr.xml"                            \
	     -e "/@goth@/r goth.xml"                            \
	alphabet.xml.in > $@;                                   \
	sed -e "/@latin@/d"                                     \
	    -e "/@runr@/d"                                      \
	    -e "/@goth@/d"                                      \
	$@ > $@.$$$$;                                           \
	mv $@.$$$$ $@

# The <dummy> tag is needed for intltoo-extract which is called by
# intltool-update.
# % intltool-extract --update --type=gettext/xml ../data/latin.xml.in
# It seems intltool-extract expects the number of the parent tag is one.
%.xml : %.xml.in
	@sed -e "s/_name/name/g"                                \
	     -e "s/_comment/comment/g"                          \
	     -e "s/_label/label/g"                              \
	     -e "/<dummy>/d"                                    \
	     -e "/<\/dummy>/d"                                  \
	$< > $@

CLEANFILES= \
	$(pad_DATA)                                             \
	$(pad_no_sorted)                                        \
	$(NULL)

#desktopdir = $(datarootdir)/applications
desktop_in_files = input-pad.desktop.in
#desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
#@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	$(desktop_in_files)                                     \
	$(icon_DATA)                                            \
	$(pad_in_in_files)                                      \
	$(pad_in_files)                                         \
	$(ui_gtk_DATA)                                          \
	char2hexstr.py                                          \
	keyboard.xml.in                                         \
	w3ctostr.py                                             \
	$(NULL)

