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

#include <gtk/gtk.h>

#include "combobox-gtk.h"

#define INPUT_PAD_GTK_COMBO_BOX_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), INPUT_PAD_TYPE_GTK_COMBO_BOX, InputPadGtkComboBoxPrivate))

struct _InputPadGtkComboBoxPrivate {
    guint                       base;
};

static GtkBuildableIface       *parent_buildable_iface;

static void input_pad_gtk_combo_box_buildable_interface_init (GtkBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (InputPadGtkComboBox, input_pad_gtk_combo_box,
                         GTK_TYPE_COMBO_BOX,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE,
                                                input_pad_gtk_combo_box_buildable_interface_init))

static void
input_pad_gtk_combo_box_init (InputPadGtkComboBox *combobox)
{
    InputPadGtkComboBoxPrivate *priv = INPUT_PAD_GTK_COMBO_BOX_GET_PRIVATE (combobox);
    combobox->priv = priv;
}

static void
input_pad_gtk_combo_box_buildable_interface_init (GtkBuildableIface *iface)
{
    parent_buildable_iface = g_type_interface_peek_parent (iface);
}

static void
input_pad_gtk_combo_box_class_init (InputPadGtkComboBoxClass *klass)
{
#if 0
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GtkObjectClass *object_class = (GtkObjectClass *) klass;
    GtkWidgetClass *widget_class = (GtkWidgetClass *) klass;
#endif

    g_type_class_add_private (klass, sizeof (InputPadGtkComboBoxPrivate));
}

GtkWidget *
input_pad_gtk_combo_box_new (void)
{
    return g_object_new (INPUT_PAD_TYPE_GTK_COMBO_BOX, NULL); 
}

guint
input_pad_gtk_combo_box_get_base (InputPadGtkComboBox *combobox)
{
    g_return_val_if_fail (combobox != NULL &&
                          INPUT_PAD_IS_GTK_COMBO_BOX (combobox), 0x0);

    return combobox->priv->base;
}

void
input_pad_gtk_combo_box_set_base (InputPadGtkComboBox *combobox,
                                  guint                base)
{
    g_return_if_fail (combobox != NULL &&
                      INPUT_PAD_IS_GTK_COMBO_BOX (combobox));

    combobox->priv->base = base;
}
