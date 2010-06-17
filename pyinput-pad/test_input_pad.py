#!/usr/bin/python

from input_pad import InputPadWindow, get_version
import sys

def button_pressed_cb(window, str, type, keysym, keycode, state, data):
    print "str =", str
    print "type =", type
    print "keysym =", keysym
    print "keycode =", keycode
    print "state =", state
    print "data =", data

print "input-pad version", get_version()
window = InputPadWindow()
window.new()
if len(sys.argv) > 1:
    print "paddir", sys.argv[1]
    window.set_paddir(sys.argv[1])
window.show()
window.set_char_button_sensitive(True)
window.connect("button-pressed", button_pressed_cb)
window.reorder_button_pressed()
print "input-pad visible?", window.get_visible()
window.main()