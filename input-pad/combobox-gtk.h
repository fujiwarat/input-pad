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

#ifndef __INPUT_PAD_COMBOBOX_GTK_H__
#define __INPUT_PAD_COMBOBOX_GTK_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define INPUT_PAD_TYPE_GTK_COMBO_BOX            (input_pad_gtk_combo_box_get_type ())
#define INPUT_PAD_GTK_COMBO_BOX(o)              (G_TYPE_CHECK_INSTANCE_CAST ((o), INPUT_PAD_TYPE_GTK_COMBO_BOX, InputPadGtkComboBox))
#define INPUT_PAD_GTK_COMBO_BOX_CLASS(k)        (G_TYPE_CHECK_CLASS_CAST ((k), INPUT_PAD_TYPE_GTK_COMBO_BOX, InputPadGtkComboBoxClass))
#define INPUT_PAD_IS_GTK_COMBO_BOX(o)           (G_TYPE_CHECK_INSTANCE_TYPE ((o), INPUT_PAD_TYPE_GTK_COMBO_BOX))
#define INPUT_PAD_IS_GTK_COMBO_BOX_CLASS(k)     (G_TYPE_CHECK_CLASS_TYPE ((k), INPUT_PAD_TYPE_GTK_COMBO_BOX))
#define INPUT_PAD_GTK_COMBO_BOX_GET_CLASS(l)    (G_TYPE_INSTANCE_GET_CLASS ((o), INPUT_PAD_TYPE_GTK_COMBO_BOX, InputPadGtkComboBoxClass))

typedef struct _InputPadGtkComboBoxPrivate InputPadGtkComboBoxPrivate;
typedef struct _InputPadGtkComboBox InputPadGtkComboBox;
typedef struct _InputPadGtkComboBoxClass InputPadGtkComboBoxClass;

struct _InputPadGtkComboBox {
    GtkComboBox                         parent;

    InputPadGtkComboBoxPrivate         *priv;
};

struct _InputPadGtkComboBoxClass {
    GtkComboBoxClass            parent_class;

    /* Padding for future expansion */
    void (*_combobox_reserved1) (void);
    void (*_combobox_reserved2) (void);
    void (*_combobox_reserved3) (void);
    void (*_combobox_reserved4) (void);
};

GType               input_pad_gtk_combo_box_get_type (void);
GtkWidget *         input_pad_gtk_combo_box_new (void);
guint               input_pad_gtk_combo_box_get_base
                                       (InputPadGtkComboBox *combobox);
void                input_pad_gtk_combo_box_set_base
                                       (InputPadGtkComboBox *combobox,
                                        guint base);

G_END_DECLS

#endif
