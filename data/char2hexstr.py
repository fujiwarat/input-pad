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

import os, sys

reverse = 0
stdread = 0
disable_orig_output = 0

def usage ():
    print "usage: " + os.path.basename(sys.argv[0]) + " FILENAME"
    print "FILENAME             FILENAME includes UTF-8 chars"

if len (sys.argv) > 1:
    if sys.argv[1] == '--disable-verbose' or sys.argv[1] == '-s':
        disable_orig_output = 1
        del sys.argv[1]
if len (sys.argv) > 1:
    if sys.argv[1] == '--stdin' or sys.argv[1] == '-i':
        stdread = 1
        del sys.argv[1]
if len (sys.argv) > 1:
    if sys.argv[1] == '--reverse' or sys.argv[1] == '-r':
        reverse = 1
        del sys.argv[1]

def to_hex_line (line):
    retval = ""
    for ch in unicode (line, "utf-8"):
        if ch == ' ':
            continue
        retval = retval + " " + hex (ord (ch))
    return retval

def to_char_line (line):
    retval = ""
    for v in line.split ():
        if not v.startswith ("0x") and not v.startswith ("0X"):
            continue
        retval = retval + " " + unichr (int (v, 16))
    return retval

def convert_line (line):
    line = line.rstrip ()
    if disable_orig_output == 0:
        print line
    if reverse == 0:
        print to_hex_line (line)
    else:
        print to_char_line (line)

if stdread != 0:
    for line in sys.stdin:
        convert_line (line)
    exit (0)

if len(sys.argv) == 1:
    usage ()
    exit (-1)

for filename in sys.argv[1:]:
    print filename + " :"
    for line in file (filename):
        convert_line (line)

