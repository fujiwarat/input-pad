/* vim:set et sts=4: */
/* input-pad - The input pad
 * Copyright (C) 2010-2011 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2010 Daiki Ueno <ueno@unixuser.org>
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

#include <X11/keysym.h>
#include <eek/eek-gtk.h>
#include <eek/eek-xkl.h>
#include <gdk/gdkx.h>

#include "eek-gtk.h"
#include "geometry-xkb.h"
#include "i18n.h"
#include "input-pad-group.h"
#include "input-pad-kbdui-gtk.h"
#include "input-pad-window-gtk.h"
#include "input-pad.h"

#define INPUT_PAD_GTK_KBDUI_EEK_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), INPUT_PAD_TYPE_GTK_KBDUI_EEK, InputPadGtkKbduiEekPrivate))

struct _InputPadGtkKbduiEekPrivate {
    EekKeyboard                *eek_keyboard;
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

static gboolean                 use_eek = FALSE;

static GOptionEntry entries[] = {
  { "enable-eek", 'e', 0, G_OPTION_ARG_NONE, &use_eek,
    N_("Use libeek to draw keyboard"), NULL},
  { NULL }
};

static void
on_window_keyboard_changed_eek (InputPadGtkWindow *window,
                                gint               group,
                                gpointer           data)
{
    InputPadGtkKbduiEek *kbdui;

    g_return_if_fail (INPUT_PAD_IS_GTK_KBDUI_EEK (data));

    kbdui = INPUT_PAD_GTK_KBDUI_EEK (data);
    eek_element_set_group (EEK_ELEMENT(kbdui->priv->eek_keyboard), group);
}

static void
on_eek_keyboard_key_pressed (EekKeyboard *keyboard,
                             EekKey      *key,
                             gpointer     user_data)
{
    InputPadGtkWindow *window;
    const gchar *str, *empty = "";
    guint keycode;
    EekSymbol *symbol;
    EekSymbol *symbol0;
    guint keysym = EEK_INVALID_KEYSYM;
    guint keysym0 = EEK_INVALID_KEYSYM;
    gint group;
    guint state = 0;
    gboolean retval = FALSE;

    g_return_if_fail (user_data != NULL &&
                      INPUT_PAD_IS_GTK_WINDOW (user_data));

    window = INPUT_PAD_GTK_WINDOW (user_data);
    keycode = eek_key_get_keycode (key);
    symbol = eek_key_get_symbol_with_fallback (key, 0, 0);
    if (EEK_IS_KEYSYM(symbol))
        keysym = eek_keysym_get_xkeysym (EEK_KEYSYM(symbol));
    str = eek_symbol_get_label (symbol);
    if (str == NULL)
        str = empty;
    group = eek_element_get_group (EEK_ELEMENT(keyboard));
    symbol0 = eek_key_get_symbol_at_index (key, group, 0, 0, 0);
    if (EEK_IS_KEYSYM(symbol0))
        keysym0 = eek_keysym_get_xkeysym (EEK_KEYSYM(symbol0));

    if (keysym0 == EEK_INVALID_KEYSYM) {
        keysym0 = keysym;
    }
    state = input_pad_gtk_window_get_keyboard_state (window);
    if (keysym != EEK_INVALID_KEYSYM && keysym != keysym0) {
        state |= ShiftMask;
    }
    state = input_pad_xkb_build_core_state (state, group);

    g_signal_emit_by_name (window, "button-pressed",
                   str, INPUT_PAD_TABLE_TYPE_KEYSYMS, keysym, keycode, state,
                   &retval);

    if (keysym0 == XK_Num_Lock) {
        keysym0 = XK_Shift_L;
    }
    input_pad_gtk_window_set_keyboard_state_with_keysym (window, keysym0);
    if (keysym0 == XK_Shift_L || keysym0 == XK_Shift_R) {
        state = input_pad_gtk_window_get_keyboard_state (window);
        eek_element_set_level (EEK_ELEMENT(keyboard), state & ShiftMask ? 1 : 0);
    }
}

static void
create_keyboard_layout_ui_real_eek (InputPadGtkKbdui  *kbdui,
                                    GtkWidget         *vbox,
                                    InputPadGtkWindow *window)
{
    InputPadGtkKbduiEek *kbdui_eek;
    EekKeyboard *keyboard;
    EekLayout *layout;
    GtkWidget *widget;
    gdouble width, height;
    Display *xdisplay = GDK_WINDOW_XDISPLAY (gtk_widget_get_window (GTK_WIDGET (window)));
    GError *error = NULL;

    g_return_if_fail (INPUT_PAD_IS_GTK_KBDUI_EEK (kbdui));

    kbdui_eek = INPUT_PAD_GTK_KBDUI_EEK (kbdui);
    layout = eek_xkl_layout_new (xdisplay, &error);
    if (layout == NULL) {
        g_warning ("Can't get XKL layout: %s", error->message);
        g_error_free (error);
        return;
    }

    keyboard = kbdui_eek->priv->eek_keyboard = eek_keyboard_new (layout, 640, 480);
    g_object_unref (layout);
    eek_keyboard_set_modifier_behavior (keyboard, EEK_MODIFIER_BEHAVIOR_LATCH);
    widget = eek_gtk_keyboard_new (keyboard);
    eek_keyboard_get_size (keyboard, &width, &height);

    gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
    gtk_box_reorder_child (GTK_BOX (vbox), widget, 0);
    gtk_widget_show (widget);
    gtk_widget_set_size_request (widget, width, height);

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

InputPadWindowType
input_pad_module_get_type (void)
{
    return INPUT_PAD_WINDOW_TYPE_GTK;
}

const gchar *
input_pad_module_get_description (void)
{
    return _("eekboard layout");
}

gboolean
input_pad_module_arg_init (int                         *argc,
                           char                      ***argv,
                           InputPadGtkKbduiContext     *context)
{
    GOptionGroup *group;
    if (context == NULL || context->context == NULL) {
        return TRUE;
    }
    group = g_option_group_new ("eek", N_("eekboard Options"), N_("Show eekboard Options"), NULL, NULL);
    g_option_group_add_entries (group, entries);
    g_option_group_set_translation_domain (group, GETTEXT_PACKAGE);
    g_option_context_add_group (context->context, group);
    return TRUE;
}

gboolean
input_pad_module_arg_init_post (int                            *argc,
                                char                         ***argv,
                                InputPadGtkKbduiContext        *context)
{
    if (use_eek) {
        input_pad_gtk_kbdui_context_set_kbdui_name (context, "eek-gtk");
    }
    return TRUE;
}

gboolean
input_pad_module_init (InputPadGtkWindow *window)
{
    eek_init ();
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
