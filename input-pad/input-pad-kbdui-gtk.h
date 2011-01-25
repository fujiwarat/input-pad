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

#ifndef __INPUT_PAD_KBDUI_H__
#define __INPUT_PAD_KBDUI_H__

#include <gtk/gtk.h>
#include <glib.h>
#include "input-pad-window-gtk.h"
#include "input-pad.h"

G_BEGIN_DECLS

#define INPUT_PAD_TYPE_GTK_KBDUI                (input_pad_gtk_kbdui_get_type ())
#define INPUT_PAD_GTK_KBDUI(o)                  (G_TYPE_CHECK_INSTANCE_CAST ((o), INPUT_PAD_TYPE_GTK_KBDUI, InputPadGtkKbdui))
#define INPUT_PAD_GTK_KBDUI_CLASS(k)            (G_TYPE_CHECK_CLASS_CAST ((k), INPUT_PAD_TYPE_GTK_KBDUI, InputPadGtkKbduiClass))
#define INPUT_PAD_IS_GTK_KBDUI(o)               (G_TYPE_CHECK_INSTANCE_TYPE ((o), INPUT_PAD_TYPE_GTK_KBDUI))
#define INPUT_PAD_IS_GTK_KBDUI_CLASS(k)         (G_TYPE_CHECK_CLASS_TYPE ((k), INPUT_PAD_TYPE_GTK_KBDUI))
#define INPUT_PAD_GTK_KBDUI_GET_CLASS(o)        (G_TYPE_INSTANCE_GET_CLASS ((o), INPUT_PAD_TYPE_GTK_KBDUI, InputPadGtkKbduiClass))

typedef struct _InputPadGtkKbdui InputPadGtkKbdui;
typedef struct _InputPadGtkKbduiPrivate InputPadGtkKbduiPrivate;
typedef struct _InputPadGtkKbduiClass InputPadGtkKbduiClass;
typedef struct _InputPadGtkKbduiContext InputPadGtkKbduiContext;
typedef struct _InputPadGtkKbduiContextPrivate InputPadGtkKbduiContextPrivate;

struct _InputPadGtkKbdui {
    GObject                             parent;
    GModule                            *module;

    InputPadGtkKbduiPrivate            *priv;
};

struct _InputPadGtkKbduiClass {
    GObjectClass                        parent_class;

    void     (* create_keyboard_layout)
                                       (InputPadGtkKbdui       *kbdui,
                                        GtkWidget              *vbox,
                                        InputPadGtkWindow      *window);

    void     (* destroy_keyboard_layout)
                                       (InputPadGtkKbdui       *kbdui,
                                        GtkWidget              *vbox,
                                        InputPadGtkWindow      *window);

    /* Padding for future expansion */
    void (*_kbdui_reserved1) (void);
    void (*_kbdui_reserved2) (void);
    void (*_kbdui_reserved3) (void);
    void (*_kbdui_reserved4) (void);
};

struct _InputPadGtkKbduiContext {
    InputPadGtkKbdui                   *kbdui;
    GOptionContext                     *context;

    InputPadGtkKbduiContextPrivate     *priv;
};

G_MODULE_EXPORT
GType               input_pad_gtk_kbdui_get_type (void);
G_MODULE_EXPORT
InputPadGtkKbduiContext *
                    input_pad_gtk_kbdui_context_new (void);
G_MODULE_EXPORT
const gchar *       input_pad_gtk_kbdui_context_get_kbdui_name
                                        (InputPadGtkKbduiContext *context);
G_MODULE_EXPORT
void                input_pad_gtk_kbdui_context_set_kbdui_name
                                        (InputPadGtkKbduiContext *context,
                                         const gchar             *name);
G_MODULE_EXPORT
void                input_pad_gtk_kbdui_context_destroy
                                        (InputPadGtkKbduiContext *context);
G_MODULE_EXPORT
const gchar *       input_pad_module_get_description (void);
G_MODULE_EXPORT
InputPadWindowType  input_pad_module_get_type (void);
G_MODULE_EXPORT
gboolean            input_pad_module_arg_init
                                        (int                     *argc,
                                         char                  ***argv,
                                         InputPadGtkKbduiContext *context);
G_MODULE_EXPORT
gboolean            input_pad_module_arg_init_post
                                        (int                     *argc,
                                         char                  ***argv,
                                         InputPadGtkKbduiContext *context);
G_MODULE_EXPORT
gboolean            input_pad_module_init (InputPadGtkWindow *window);
G_MODULE_EXPORT
InputPadGtkKbdui *  input_pad_module_kbdui_new (InputPadGtkWindow *window);

G_END_DECLS

#endif
