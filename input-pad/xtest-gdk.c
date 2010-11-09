/* vim:set et sts=4: */
/* input-pad - The input pad
 * Copyright (C) 2010 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2010 Red Hat, Inc.
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

static int
send_key_event (GdkWindow      *gdkwindow,
                guint           keysym,
                guint           keycode,
                guint           state)
{
    Display    *display;
    KeyCode     keycode_real;

    display = GDK_WINDOW_XDISPLAY (gdkwindow);
    if (keycode != 0) {
        keycode_real = (KeyCode) keycode;
    } else {
        keycode_real = XKeysymToKeycode (display, (KeySym) keysym);
    }
    XTestFakeKeyEvent (display, keycode_real, True, CurrentTime);
    XSync (display, False);
    XTestFakeKeyEvent (display, keycode_real, False, CurrentTime);
    XSync (display, False);

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
