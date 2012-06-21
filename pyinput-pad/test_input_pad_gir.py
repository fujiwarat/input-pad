#!/usr/bin/python

#from input_pad import InputPadWindow, get_version, get_kbdui_name_list
from gi.repository import Gtk
from gi.repository import InputPad
import os, sys

def button_pressed_cb(window, str, type, keysym, keycode, state):
    print "window =", window
    print "str =", str
    print "type =", type
    print "keysym =", keysym
    print "keycode =", keycode
    print "state =", state

print "input-pad version:", InputPad.get_version()
window = InputPad.GtkWindow.new(Gtk.WindowType.TOPLEVEL, 0)
# InputPad.Window() calls setlocale()
length = InputPad.window_get_kbdui_name_list_length()
print "available kbdui modules length:", length
i = 0
while i < length:
    print "available kbdui modules", \
        InputPad.window_get_kbdui_name_get_name_with_index(i)
    i += 1
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
InputPad.window_main(window)
