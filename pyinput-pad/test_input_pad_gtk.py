#!/usr/bin/python

import sys
import gtk
from input_pad_window_gtk import _InputPadGtkWindow

def button_pressed_cb(window, str, type, keysym, keycode, state, data):
    print "str =", str
    print "type =", type
    print "keysym =", keysym
    print "keycode =", keycode
    print "state =", state
    print "data =", data

window = _InputPadGtkWindow (gtk.WINDOW_TOPLEVEL, 0)
window.show()
window.connect("button-pressed", button_pressed_cb)
print "input-pad visible?", window.get_visible()
gtk.main()
