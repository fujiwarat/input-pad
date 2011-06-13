#!/usr/bin/python
# vim:set et sts=4 sw=4:
# -*- coding: utf-8 -*-
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

# the latest is http://www.w3.org/TR/REC-html40/sgml/entities.html

import os, sys

reverse = 0
stdread = 0
disable_orig_output = 0

def usage ():
    print "usage: " + os.path.basename(sys.argv[0]) + " entities.html"

if len(sys.argv) == 1:
    usage ()
    exit (-1)

class PadHTMLLabelRawText:
    def __init__(self, line):
        line_array = line.split()
        self.__rawtext = line_array[1]
        label = line_array[3]
        label = label.split('"')[1]
        self.__label = label.replace('&amp;', '&')
    def __cmp__(self, pad_b):
        code_a = int (self.__label[2:len(self.__label) - 1], 10)
        code_b = int (pad_b.__label[2:len(pad_b.__label) - 1], 10)
        return code_a - code_b
    def __str__(self):
        retval = \
            "        <string>\n"
        retval = retval + \
            "          <label>%s</label>\n"           % self.__label
        retval = retval + \
            "          <rawtext>&amp;%s;</rawtext>\n" % self.__rawtext
        retval = retval + \
            "        </string>"
        return retval


pad_list = []
for filename in sys.argv[1:]:
    for line in file (filename):
        if not line.startswith("&lt;!ENTITY"):
            continue
        pad = PadHTMLLabelRawText(line)
        pad_list.append(pad)
pad_list.sort()
for pad in pad_list:
    print str(pad)
