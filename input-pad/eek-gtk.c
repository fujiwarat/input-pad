/* vim:set et sts=4: */
/* input-pad - The input pad
 * Copyright (C) 2010 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2010 Daiki Ueno <ueno@unixuser.org>
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

#include <X11/keysym.h>
#include <eek/eek-gtk.h>
#include <eek/eek-xkl.h>

#include "input-pad-group.h"
#include "input-pad-kbdui-gtk.h"
#include "input-pad-window-gtk.h"
#include "geometry-xkb.h"
#include "eek-gtk.h"

#define INPUT_PAD_GTK_KBDUI_EEK_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), INPUT_PAD_TYPE_GTK_KBDUI_EEK, InputPadGtkKbduiEekPrivate))

struct _InputPadGtkKbduiEekPrivate {
    EekKeyboard                *eek_keyboard;
    EekLayout                  *eek_layout;
};

static void             create_keyboard_layout_ui_real_eek
                                                (InputPadGtkKbdui  *kbdui,
                                                 GtkWidget         *vbox,
                                                 InputPadGtkWindow *window);
static void             destroy_prev_keyboard_layout_eek
                                                (InputPadGtkKbdui  *kbdui,
                                                 GtkWidget         *vbox,
                                                 InputPadGtkWindow *window);

G_DEFINE_TYPE (InputPadGtkKbduiEek, input_pad_gtk_kbdui_eek,
               INPUT_PAD_TYPE_GTK_KBDUI)

static void
on_window_keyboard_changed_eek (InputPadGtkWindow *window,
                                gint               group,
                                gpointer           data)
{
    gint prev_group, prev_level;
    InputPadGtkKbduiEek *kbdui;

    g_return_if_fail (INPUT_PAD_IS_GTK_KBDUI_EEK (data));

    kbdui = INPUT_PAD_GTK_KBDUI_EEK (data);
    eek_keyboard_get_keysym_index (kbdui->priv->eek_keyboard,
                                   &prev_group, &prev_level);
    eek_keyboard_set_keysym_index (kbdui->priv->eek_keyboard,
                                   group, prev_level);
}

static void
on_eek_keyboard_key_pressed (EekKeyboard *keyboard,
                             EekKey      *key,
                             gpointer     user_data)
{
    InputPadGtkWindow *window;
    char *str, *empty = "";
    guint keycode;
    guint keysym;
    guint *keysyms;
    gint num_groups, num_levels;
    gint group, level;
    guint state = 0;
    gboolean retval = FALSE;

    g_return_if_fail (user_data != NULL &&
                      INPUT_PAD_IS_GTK_WINDOW (user_data));

    window = INPUT_PAD_GTK_WINDOW (user_data);
    keycode = eek_key_get_keycode (key);
    keysym = eek_key_get_keysym (key);
    str = eek_keysym_to_string (keysym);
    if (str == NULL)
        str = empty;
    eek_key_get_keysyms (key, &keysyms, &num_groups, &num_levels);
    eek_key_get_keysym_index (key, &group, &level);
    state = input_pad_gtk_window_get_keyboard_state (window);
    if (keysyms && (keysym != keysyms[group * num_levels])) {
        state |= ShiftMask;
    }
    state = input_pad_xkb_build_core_state (state, group);

    g_signal_emit_by_name (window, "button-pressed",
                   str, INPUT_PAD_TABLE_TYPE_KEYSYMS, keysym, keycode, state,
                   &retval);
    if (str != empty)
        g_free (str);

    if (state & ShiftMask) {
        state ^= ShiftMask;
    }
    if ((state & ControlMask) && 
        (keysym != XK_Control_L) && (keysym != XK_Control_R)) {
        state ^= ControlMask;
    }
    if ((state & Mod1Mask) && 
        (keysym != XK_Alt_L) && (keysym != XK_Alt_R)) {
        state ^= Mod1Mask;
    }
    input_pad_gtk_window_set_keyboard_state (window, state);
}

static void
create_keyboard_layout_ui_real_eek (InputPadGtkKbdui  *kbdui,
                                    GtkWidget         *vbox,
                                    InputPadGtkWindow *window)
{
    InputPadGtkKbduiEek *kbdui_eek;
    EekKeyboard *keyboard;
    EekLayout *layout;
    EekBounds bounds;
    GtkWidget *widget;

    g_return_if_fail (INPUT_PAD_IS_GTK_KBDUI_EEK (kbdui));

    kbdui_eek = INPUT_PAD_GTK_KBDUI_EEK (kbdui);
    keyboard = kbdui_eek->priv->eek_keyboard = eek_gtk_keyboard_new ();
    g_object_ref_sink (keyboard);
    layout = kbdui_eek->priv->eek_layout = eek_xkl_layout_new ();
    g_object_ref_sink (layout);
    eek_keyboard_set_layout (keyboard, layout);
    bounds.width = 640;
    bounds.height = 480;
    eek_element_set_bounds (EEK_ELEMENT(keyboard), &bounds);
    widget = eek_gtk_keyboard_get_widget (EEK_GTK_KEYBOARD(keyboard));
    eek_element_get_bounds (EEK_ELEMENT(keyboard), &bounds);

    gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
    gtk_box_reorder_child (GTK_BOX (vbox), widget, 0);
    gtk_widget_show (widget);
    gtk_widget_set_size_request (widget, bounds.width, bounds.height);

    g_signal_connect (G_OBJECT (window), "keyboard-changed",
                      G_CALLBACK (on_window_keyboard_changed_eek),
                      (gpointer) kbdui);
    g_signal_connect (G_OBJECT (keyboard), "key-pressed",
		      G_CALLBACK (on_eek_keyboard_key_pressed),
		      (gpointer) window);
}

static void
destroy_prev_keyboard_layout_eek (InputPadGtkKbdui  *kbdui,
                                  GtkWidget         *vbox,
                                  InputPadGtkWindow *window)
{
    InputPadGtkKbduiEek *kbdui_eek;
    GList *children;
    GtkWidget *widget;

    g_return_if_fail (INPUT_PAD_IS_GTK_KBDUI_EEK (kbdui));

    kbdui_eek = INPUT_PAD_GTK_KBDUI_EEK (kbdui);
    children = gtk_container_get_children (GTK_CONTAINER (vbox));
    widget = GTK_WIDGET (children->data);
    gtk_widget_hide (widget);
    gtk_widget_destroy (widget);

    g_object_unref (kbdui_eek->priv->eek_keyboard);
    g_object_unref (kbdui_eek->priv->eek_layout);
}

static void
input_pad_gtk_kbdui_eek_init (InputPadGtkKbduiEek *kbdui)
{
    InputPadGtkKbduiEekPrivate *priv = INPUT_PAD_GTK_KBDUI_EEK_GET_PRIVATE (kbdui);
    kbdui->priv = priv;
}

static void
input_pad_gtk_kbdui_eek_class_init (InputPadGtkKbduiEekClass *klass)
{
#if 0
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
#endif
    InputPadGtkKbduiClass *kbdui_class = (InputPadGtkKbduiClass *) klass;

    g_type_class_add_private (klass, sizeof (InputPadGtkKbduiEekPrivate));

    kbdui_class->create_keyboard_layout =
        create_keyboard_layout_ui_real_eek;
    kbdui_class->destroy_keyboard_layout =
        destroy_prev_keyboard_layout_eek;
}

InputPadGtkKbdui *
input_pad_gtk_kbdui_eek_new (void)
{
    return g_object_new (INPUT_PAD_TYPE_GTK_KBDUI_EEK, NULL);
}

gboolean
input_pad_module_arg_init (int *argc, char ***argv)
{
    return TRUE;
}

gboolean
input_pad_module_init (InputPadGtkWindow *window)
{
    return TRUE;
}

InputPadGtkKbdui *
input_pad_module_kbdui_new (InputPadGtkWindow *window)
{
    return input_pad_gtk_kbdui_eek_new ();
}

const gchar*
g_module_check_init (GModule *module)
{
    return NULL;
}
