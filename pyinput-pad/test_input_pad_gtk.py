#!/usr/bin/python

import sys
import gtk
from input_pad_window_gtk import InputPadGtkWindow

def button_pressed_cb(window, str, type, keysym, keycode, state, data):
    print "str =", str
    print "type =", type
    print "keysym =", keysym
    print "keycode =", keycode
    print "state =", state
    print "data =", data

if len(sys.argv) > 1:
    print "paddir", sys.argv[1]
    window = InputPadGtkWindow (gtk.WINDOW_TOPLEVEL, 0, sys.argv[1])
else:
    window = InputPadGtkWindow (gtk.WINDOW_TOPLEVEL)
window.show()
window.connect("button-pressed", button_pressed_cb)
print "input-pad visible?", window.get_visible()
gtk.main()
