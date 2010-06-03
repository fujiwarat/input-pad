#!/usr/bin/python

from input_pad import InputPadWindow, get_version
import gtk

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
window.show()
window.connect("button-pressed", button_pressed_cb)
window.main()