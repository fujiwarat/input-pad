#!/usr/bin/python

import os, sys
import gtk
import input_pad
from input_pad_window_gtk import InputPadGtkWindow

def button_pressed_cb(window, str, type, keysym, keycode, state, data):
    print "str =", str
    print "type =", type
    print "keysym =", keysym
    print "keycode =", keycode
    print "state =", state
    print "data =", data

input_pad.init()
window = InputPadGtkWindow (gtk.WINDOW_TOPLEVEL)
if len(sys.argv) > 1:
    if os.path.isdir(sys.argv[1]):
        print "paddir", sys.argv[1]
        window.set_paddir(sys.argv[1])
    elif os.path.isfile(sys.argv[1]):
        print "padfile", sys.argv[1]
        window.append_padfile(sys.argv[1])
window.show()
window.set_char_button_sensitive(True)
window.connect("button-pressed", button_pressed_cb)
window.reorder_button_pressed()
print "input-pad visible?", window.get_visible()
print "keyboard_state?", window.get_keyboard_state()
gtk.main()
