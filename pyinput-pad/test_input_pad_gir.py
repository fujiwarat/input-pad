#!/usr/bin/python

#from input_pad import InputPadWindow, get_version, get_kbdui_name_list
from gi.repository import Gio
from gi.repository import Gtk
from gi.repository import InputPad
import os, sys

def button_pressed_cb(window, str, type, keysym, keycode, state):
    print("str = %s" % str)
    print("type = %u" % type)
    print("keysym = %u" % keysym)
    print("keycode = %u" % keycode)
    print("state = %u" % state)

def on_activated(app):
    window = app.get_window()
    # InputPad.Window() calls setlocale()
    length = InputPad.window_get_kbdui_name_list_length()
    print("available kbdui modules length: %d" % length)
    i = 0
    while i < length:
        print("available kbdui modules %s" %\
              InputPad.window_get_kbdui_name_get_name_with_index(i))
        i += 1
    if len(sys.argv) > 1:
        if os.path.isdir(sys.argv[1]):
            print("paddir %s" % sys.argv[1])
            window.set_paddir(sys.argv[1])
        elif os.path.isfile(sys.argv[1]):
            print("padfile %s" % sys.argv[1])
            window.append_padfile(sys.argv[1])
    #window.set_kbdui_name("eek-gtk")
    window.show()
    #window.set_show_table (0)
    window.set_show_layout (0)
    window.set_char_button_sensitive(True)
    window.connect("button-pressed", button_pressed_cb)
    window.reorder_button_pressed()

print("input-pad version: %s" % InputPad.get_version())
sys.argv = InputPad.window_init(sys.argv, InputPad.WindowType.WINDOW_TYPE_GTK)
app = InputPad.GtkApplication.new()
app.connect('activated', on_activated)
Gio.Application.run(app, sys.argv)
