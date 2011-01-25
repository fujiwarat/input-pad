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

#ifndef __INPUT_PAD_WINDOW_GTK_H__
#define __INPUT_PAD_WINDOW_GTK_H__

#include <gtk/gtk.h>
#include "input-pad.h"

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
    guint                               child;

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

    void     (* group_changed)         (InputPadGtkWindow      *window,
                                        const gchar            *paddir,
                                        const gchar            *domain);

    void     (* group_appended)        (InputPadGtkWindow      *window,
                                        const gchar            *padfile,
                                        const gchar            *domain);

    void     (* char_button_sensitive) (InputPadGtkWindow      *window,
                                        gboolean                sensitive);

    void     (* reorder_button_pressed) (InputPadGtkWindow      *window);

    /* Padding for future expansion */
    void (*_window_reserved1) (void);
    void (*_window_reserved2) (void);
    void (*_window_reserved3) (void);
    void (*_window_reserved4) (void);
};

G_MODULE_EXPORT
GType               input_pad_gtk_window_get_type (void);
GtkWidget *         input_pad_gtk_window_new (GtkWindowType     type,
                                              unsigned int      child);
void                input_pad_gtk_window_set_paddir
                                       (InputPadGtkWindow      *window,
                                        const gchar            *paddir,
                                        const gchar            *domain);
void                input_pad_gtk_window_append_padfile
                                       (InputPadGtkWindow      *window,
                                        const gchar            *padfile,
                                        const gchar            *domain);
void                input_pad_gtk_window_set_char_button_sensitive
                                       (InputPadGtkWindow      *window,
                                        gboolean                sensitive);
void                input_pad_gtk_window_reorder_button_pressed
                                       (InputPadGtkWindow      *window);
guint               input_pad_gtk_window_get_keyboard_state
                                       (InputPadGtkWindow      *window);
void                input_pad_gtk_window_set_keyboard_state
                                       (InputPadGtkWindow      *window,
                                        guint                   keyboard_state);
void                input_pad_gtk_window_set_keyboard_state_with_keysym
                                       (InputPadGtkWindow      *window,
                                        guint                   keysym);
InputPadWindowKbduiName *
                    input_pad_gtk_window_get_kbdui_name_list (void);
void                input_pad_gtk_window_set_kbdui_name
                                       (InputPadGtkWindow      *window,
                                        const gchar            *name);
void                input_pad_gtk_window_set_show_table
                                       (InputPadGtkWindow      *window,
                                        InputPadWindowShowTableType type);
void                input_pad_gtk_window_set_show_layout
                                       (InputPadGtkWindow      *window,
                                        InputPadWindowShowLayoutType type);

G_END_DECLS

#endif
