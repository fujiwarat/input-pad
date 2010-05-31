/* vim:set et sts=4: */
/* input-pad - The input pad
 * Copyright (C) 2010 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2010 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

#include "input-pad-group.h"
#include "button-gtk.h"

#define INPUT_PAD_GTK_BUTTON_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), INPUT_PAD_TYPE_GTK_BUTTON, InputPadGtkButtonPrivate))

struct _InputPadGtkButtonPrivate {
    guint                       keycode;
    guint                       keysym;
    guint                     **keysyms;
    int                         keysym_group;
    guint                       state;
    InputPadTableType           type;
};

static GtkBuildableIface       *parent_buildable_iface;

static void input_pad_gtk_button_buildable_interface_init (GtkBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (InputPadGtkButton, input_pad_gtk_button,
                         GTK_TYPE_BUTTON,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE,
                                                input_pad_gtk_button_buildable_interface_init))

static void
input_pad_gtk_button_init (InputPadGtkButton *button)
{
    InputPadGtkButtonPrivate *priv = INPUT_PAD_GTK_BUTTON_GET_PRIVATE (button);
    button->priv = priv;
}

static void
input_pad_gtk_button_buildable_interface_init (GtkBuildableIface *iface)
{
    parent_buildable_iface = g_type_interface_peek_parent (iface);
}

static void
input_pad_gtk_button_class_init (InputPadGtkButtonClass *klass)
{
#if 0
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GtkObjectClass *object_class = (GtkObjectClass *) klass;
    GtkWidgetClass *widget_class = (GtkWidgetClass *) klass;
#endif

    g_type_class_add_private (klass, sizeof (InputPadGtkButtonPrivate));
}

GtkWidget *
input_pad_gtk_button_new_with_label (const gchar *label)
{
    return g_object_new (INPUT_PAD_TYPE_GTK_BUTTON, "label", label, NULL); 
}

guint
input_pad_gtk_button_get_keycode (InputPadGtkButton *button)
{
    g_return_val_if_fail (button != NULL &&
                          INPUT_PAD_IS_GTK_BUTTON (button), 0x0);

    return button->priv->keycode;
}

void
input_pad_gtk_button_set_keycode (InputPadGtkButton *button,
                                  guint              keycode)
{
    g_return_if_fail (button != NULL &&
                      INPUT_PAD_IS_GTK_BUTTON (button));

    button->priv->keycode = keycode;
}

guint
input_pad_gtk_button_get_keysym (InputPadGtkButton *button)
{
    g_return_val_if_fail (button != NULL &&
                          INPUT_PAD_IS_GTK_BUTTON (button), 0x0);

    return button->priv->keysym;
}

void
input_pad_gtk_button_set_keysym (InputPadGtkButton *button,
                                 guint              keysym)
{
    g_return_if_fail (button != NULL &&
                      INPUT_PAD_IS_GTK_BUTTON (button));

    button->priv->keysym = keysym;
}

guint **
input_pad_gtk_button_get_all_keysyms (InputPadGtkButton *button)
{
    g_return_val_if_fail (button != NULL &&
                          INPUT_PAD_IS_GTK_BUTTON (button), NULL);

    return button->priv->keysyms;
}

void
input_pad_gtk_button_set_all_keysyms (InputPadGtkButton         *button,
                                      guint                    **keysyms)
{
    g_return_if_fail (button != NULL &&
                      INPUT_PAD_IS_GTK_BUTTON (button));

    button->priv->keysyms = keysyms;
}

int
input_pad_gtk_button_get_keysym_group (InputPadGtkButton *button)
{
    g_return_val_if_fail (button != NULL &&
                          INPUT_PAD_IS_GTK_BUTTON (button), 0);

    return button->priv->keysym_group;
}

void
input_pad_gtk_button_set_keysym_group (InputPadGtkButton         *button,
                                       int                        group)
{
    g_return_if_fail (button != NULL &&
                      INPUT_PAD_IS_GTK_BUTTON (button));

    button->priv->keysym_group = group;
}

guint
input_pad_gtk_button_get_state (InputPadGtkButton *button)
{
    g_return_val_if_fail (button != NULL &&
                          INPUT_PAD_IS_GTK_BUTTON (button), 0x0);

    return button->priv->state;
}

void
input_pad_gtk_button_set_state (InputPadGtkButton *button,
                                guint              state)
{
    g_return_if_fail (button != NULL &&
                      INPUT_PAD_IS_GTK_BUTTON (button));

    button->priv->state = state;
}

InputPadTableType
input_pad_gtk_button_get_table_type (InputPadGtkButton *button)
{
    g_return_val_if_fail (button != NULL &&
                          INPUT_PAD_IS_GTK_BUTTON (button), INPUT_PAD_TABLE_TYPE_NONE);

    return button->priv->type;
}

void
input_pad_gtk_button_set_table_type (InputPadGtkButton *button,
                                     InputPadTableType  type)
{
    g_return_if_fail (button != NULL &&
                      INPUT_PAD_IS_GTK_BUTTON (button));

    button->priv->type = type;
}
