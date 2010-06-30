#!/usr/bin/python

from input_pad import InputPadWindow, get_version, get_kbdui_name_list
import os, sys

def button_pressed_cb(window, str, type, keysym, keycode, state, data):
    print "str =", str
    print "type =", type
    print "keysym =", keysym
    print "keycode =", keycode
    print "state =", state
    print "data =", data

print "input-pad version:", get_version()
window = InputPadWindow()
# InputPadWindow() calls setlocale()
print "available kbdui modules:", get_kbdui_name_list()
window.new()
if len(sys.argv) > 1:
    if os.path.isdir(sys.argv[1]):
        print "paddir", sys.argv[1]
        window.set_paddir(sys.argv[1])
    elif os.path.isfile(sys.argv[1]):
        print "padfile", sys.argv[1]
        window.append_padfile(sys.argv[1])
#window.set_kbdui_name("eek-gtk")
window.show()
#window.set_show_table (0)
#window.set_show_layout (0)
window.set_char_button_sensitive(True)
window.connect("button-pressed", button_pressed_cb)
window.reorder_button_pressed()
print "input-pad visible?", window.get_visible()
window.main()
