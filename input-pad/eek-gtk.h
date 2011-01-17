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

#ifndef __INPUT_PAD_EEK_GTK_H__
#define __INPUT_PAD_EEK_GTK_H__

#include <gtk/gtk.h>
#include "input-pad-kbdui-gtk.h"

G_BEGIN_DECLS

#define INPUT_PAD_TYPE_GTK_KBDUI_EEK            (input_pad_gtk_kbdui_eek_get_type ())
#define INPUT_PAD_GTK_KBDUI_EEK(o)              (G_TYPE_CHECK_INSTANCE_CAST ((o), INPUT_PAD_TYPE_GTK_KBDUI_EEK, InputPadGtkKbduiEek))
#define INPUT_PAD_GTK_KBDUI_EEK_CLASS(k)        (G_TYPE_CHECK_CLASS_CAST ((k), INPUT_PAD_TYPE_GTK_KBDUI_EEK, InputPadGtkKbduiEekClass))
#define INPUT_PAD_IS_GTK_KBDUI_EEK(o)           (G_TYPE_CHECK_INSTANCE_TYPE ((o), INPUT_PAD_TYPE_GTK_KBDUI_EEK))
#define INPUT_PAD_IS_GTK_KBDUI_EEK_CLASS(k)     (G_TYPE_CHECK_CLASS_TYPE ((k), INPUT_PAD_TYPE_GTK_KBDUI_EEK))
#define INPUT_PAD_GTK_KBDUI_EEK_GET_CLASS(o)    (G_TYPE_INSTANCE_GET_CLASS ((o), INPUT_PAD_TYPE_GTK_KBDUI_EEK, InputPadGtkKbduiEekClass))

typedef struct _InputPadGtkKbduiEekPrivate InputPadGtkKbduiEekPrivate;
typedef struct _InputPadGtkKbduiEek InputPadGtkKbduiEek;
typedef struct _InputPadGtkKbduiEekClass InputPadGtkKbduiEekClass;

struct _InputPadGtkKbduiEek {
    InputPadGtkKbdui                    parent;

    InputPadGtkKbduiEekPrivate         *priv;
};

struct _InputPadGtkKbduiEekClass {
    InputPadGtkKbduiClass               parent_class;

    /* Padding for future expansion */
    void (*_kbdui_eek_reserved1) (void);
    void (*_kbdui_eek_reserved2) (void);
    void (*_kbdui_eek_reserved3) (void);
    void (*_kbdui_eek_reserved4) (void);
};

G_MODULE_EXPORT
GType               input_pad_gtk_kbdui_eek_get_type (void);
InputPadGtkKbdui *  input_pad_gtk_kbdui_eek_new (void);

G_END_DECLS

#endif
