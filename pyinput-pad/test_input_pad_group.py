#!/usr/bin/python

from input_pad_group import input_pad_group_parse_all_files

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
    group = input_pad_group_parse_all_files()
    dump_group(group)

def main():
    test()

main()
