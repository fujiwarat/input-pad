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

#ifndef __INPUT_PAD_WINDOW_GTK_H__
#define __INPUT_PAD_WINDOW_GTK_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define INPUT_PAD_TYPE_GTK_WINDOW               (input_pad_gtk_window_get_type ())
#define INPUT_PAD_GTK_WINDOW(o)                 (G_TYPE_CHECK_INSTANCE_CAST ((o), INPUT_PAD_TYPE_GTK_WINDOW, InputPadGtkWindow))
#define INPUT_PAD_GTK_WINDOW_CLASS(k)           (G_TYPE_CHECK_CLASS_CAST ((k), INPUT_PAD_TYPE_GTK_WINDOW, InputPadGtkWindowClass))
#define INPUT_PAD_IS_GTK_WINDOW(o)              (G_TYPE_CHECK_INSTANCE_TYPE ((o), INPUT_PAD_TYPE_GTK_WINDOW))
#define INPUT_PAD_IS_GTK_WINDOW_CLASS(k)        (G_TYPE_CHECK_CLASS_TYPE ((k), INPUT_PAD_TYPE_GTK_WINDOW))
#define INPUT_PAD_GTK_WINDOW_GET_CLASS(o)       (G_TYPE_INSTANCE_GET_CLASS ((o), INPUT_PAD_TYPE_GTK_WINDOW, InputPadGtkWindowClass))

typedef struct _InputPadGtkWindowPrivate InputPadGtkWindowPrivate;
typedef struct _InputPadGtkWindow InputPadGtkWindow;
typedef struct _InputPadGtkWindowClass InputPadGtkWindowClass;

struct _InputPadGtkWindow {
    GtkWindow                           parent;
    guint                               child : 1;

    InputPadGtkWindowPrivate           *priv;
};

struct _InputPadGtkWindowClass {
    GtkWindowClass              parent_class;

    gboolean (* button_pressed)        (InputPadGtkWindow      *window,
                                        const gchar            *str,
                                        guint                   type,
                                        guint                   keysym,
                                        guint                   keycode,
                                        guint                   state);

    void     (* keyboard_changed)      (InputPadGtkWindow      *window,
                                        int                     group);

    /* Padding for future expansion */
    void (*_window_reserved1) (void);
    void (*_window_reserved2) (void);
    void (*_window_reserved3) (void);
    void (*_window_reserved4) (void);
};

G_MODULE_EXPORT
GType               input_pad_gtk_window_get_type (void);
GtkWidget *         input_pad_gtk_window_new (GtkWindowType	type,
                                              unsigned int      ibus);

G_END_DECLS

#endif
