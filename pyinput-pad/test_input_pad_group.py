#!/usr/bin/python

import input_pad_group
import sys

def dump_group(group):
    while group != None:
        print "group name =", group.name
        table = group.table
        while table != None:
            print "  table name =", table.name
            print "  table column =", table.column
            print "  table type =", table.type
            if table.type == 1:
                print "  table chars", table.data.chars
            elif table.type == 2:
                print "  table keysyms", table.data.keysyms
            table = table.next
        group = group.next

def test():
    if len(sys.argv) > 1:
        print "paddir", sys.argv[1]
        group = input_pad_group.parse_all_files(sys.argv[1])
    else:
        group = input_pad_group.parse_all_files()
    dump_group(group)

def main():
    test()

main()
