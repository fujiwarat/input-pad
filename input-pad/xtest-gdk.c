/* vim:set et sts=4: */
/* input-pad - The input pad
 * Copyright (C) 2010-2011 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2010-2011 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>

#include <input-pad-window-gtk.h>
#include <input-pad-group.h>

G_MODULE_EXPORT
gboolean     input_pad_module_init (InputPadGtkWindow *window);
G_MODULE_EXPORT
gboolean     input_pad_module_setup (InputPadGtkWindow *window);
G_MODULE_EXPORT
const gchar* g_module_check_init (GModule *module);

static Display *saved_display = NULL;

static int
xsend_key_state (Display       *display,
                 guint          state,
                 Bool           pressed)
{
    KeyCode     keycode_real;
    static struct {
        guint   state;
        KeySym  keysym;
    } state2keysym[] = {
        { ControlMask, XK_Control_L } ,
        { Mod1Mask,    XK_Alt_L },
        { Mod4Mask,    XK_Super_L },
        { ShiftMask,   XK_Shift_L },
        { LockMask,    XK_Caps_Lock },
        { 0,           0L }
    };
    int i;

    if (pressed) {
        saved_display = display;
    } else {
        saved_display = NULL;
    }

    for (i = 0; state2keysym[i].state != 0; i++) {
        if (state & state2keysym[i].state) {
            keycode_real = XKeysymToKeycode (display, state2keysym[i].keysym);
            XTestFakeKeyEvent (display, keycode_real, pressed, CurrentTime);
            XSync (display, False);
        }
    }
    return TRUE;
}

static void
signal_exit_cb (int signal_id)
{
    if (saved_display != NULL) {
        xsend_key_state (saved_display,
                         ControlMask | Mod1Mask | Mod4Mask | ShiftMask | LockMask,
                         FALSE);
        saved_display = NULL;
    }
    signal (signal_id, SIG_DFL);
    raise (signal_id);
}

static int
send_key_event (GdkWindow      *gdkwindow,
                guint           keysym,
                guint           keycode,
                guint           state)
{
    Display    *display;
    KeyCode     keycode_real;

    display = GDK_WINDOW_XDISPLAY (gdkwindow);
    if (state != 0) {
        xsend_key_state (display, state, True);
    }
    if (keycode != 0) {
        keycode_real = (KeyCode) keycode;
    } else {
        keycode_real = XKeysymToKeycode (display, (KeySym) keysym);
    }
    XTestFakeKeyEvent (display, keycode_real, True, CurrentTime);
    XSync (display, False);
    XTestFakeKeyEvent (display, keycode_real, False, CurrentTime);
    XSync (display, False);
    if (state != 0) {
        xsend_key_state (display, state, False);
    }

    return TRUE;
}

/*
 * Need to get GdkWindow after invoke gtk_widget_show() and gtk_main()
 */
static gboolean
have_extension (InputPadGtkWindow *window)
{
    int opcode = 0;
    int event  = 0;
    int error  = 0;

    g_return_val_if_fail (window != NULL &&
                          INPUT_PAD_IS_GTK_WINDOW (window), FALSE);

    if (!XQueryExtension (GDK_WINDOW_XDISPLAY (gtk_widget_get_window (GTK_WIDGET (window))),
                         "XTEST", &opcode, &event, &error)) {
        g_warning ("Could not find XTEST module. Maybe you did not install "
                   "libXtst library.\n"
                   "%% xdpyinfo | grep XTEST");
        return FALSE;
    }
    return TRUE;
}

static unsigned int
on_window_button_pressed (InputPadGtkWindow    *window,
                          gchar                *str,
                          guint                 type,
                          guint                 keysym,
                          guint                 keycode,
                          guint                 state,
                          gpointer              data)
{
    if (!have_extension (window)) {
        return FALSE;
    }
    if (type == INPUT_PAD_TABLE_TYPE_CHARS) {
        if (keysym > 0) {
            send_key_event (gtk_widget_get_window (GTK_WIDGET (window)), keysym, keycode, state);
            return TRUE;
        } else {
            return FALSE;
        }
    } else if (type == INPUT_PAD_TABLE_TYPE_KEYSYMS) {
        send_key_event (gtk_widget_get_window (GTK_WIDGET (window)), keysym, keycode, state);
        return TRUE;
    }
    return FALSE;
}

static void
on_window_reorder_button_pressed (InputPadGtkWindow    *window,
                                  gpointer              data)
{
    g_signal_handlers_disconnect_by_func (G_OBJECT (window),
                                          G_CALLBACK (on_window_button_pressed),
                                          NULL);
    g_signal_connect (G_OBJECT (window),
                      "button-pressed",
                      G_CALLBACK (on_window_button_pressed), NULL);
}

gboolean
input_pad_module_init (InputPadGtkWindow *window)
{
    signal (SIGINT, signal_exit_cb);
    return TRUE;
}

gboolean
input_pad_module_setup (InputPadGtkWindow *window)
{
    g_return_val_if_fail (window != NULL &&
                          INPUT_PAD_IS_GTK_WINDOW (window), FALSE);

    g_signal_connect (G_OBJECT (window),
                      "button-pressed",
                      G_CALLBACK (on_window_button_pressed), NULL);
    g_signal_connect (G_OBJECT (window),
                      "reorder-button-pressed",
                      G_CALLBACK (on_window_reorder_button_pressed), NULL);
    return TRUE;
}

const gchar*
g_module_check_init (GModule *module)
{
    return NULL;
}
