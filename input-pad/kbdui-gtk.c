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

#include "input-pad-window-gtk.h"
#include "input-pad-kbdui-gtk.h"
#include "input-pad-marshal.h"
#include "i18n.h"

enum {
    CREATE_KEYBOARD_LAYOUT,
    DESTROY_KEYBOARD_LAYOUT,
    LAST_SIGNAL
};

static guint            signals[LAST_SIGNAL] = { 0 };

struct _InputPadGtkKbduiContextPrivate {
    gchar *kbdui_name;
};

G_DEFINE_ABSTRACT_TYPE (InputPadGtkKbdui, input_pad_gtk_kbdui, G_TYPE_OBJECT);

static void
input_pad_gtk_kbdui_init (InputPadGtkKbdui *kbdui)
{
}

static void
input_pad_gtk_kbdui_class_init (InputPadGtkKbduiClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    signals[CREATE_KEYBOARD_LAYOUT] = 
        g_signal_new (I_("create-keyboard-layout"),
                      G_TYPE_FROM_CLASS (gobject_class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (InputPadGtkKbduiClass,
                                       create_keyboard_layout),
                      NULL, NULL,
                      INPUT_PAD_VOID__OBJECT_OBJECT,
                      G_TYPE_NONE,
                      2, GTK_TYPE_WIDGET, INPUT_PAD_TYPE_GTK_WINDOW);

    signals[DESTROY_KEYBOARD_LAYOUT] = 
        g_signal_new (I_("destroy-keyboard-layout"),
                      G_TYPE_FROM_CLASS (gobject_class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (InputPadGtkKbduiClass,
                                       destroy_keyboard_layout),
                      NULL, NULL,
                      INPUT_PAD_VOID__OBJECT_OBJECT,
                      G_TYPE_NONE,
                      2, GTK_TYPE_WIDGET, INPUT_PAD_TYPE_GTK_WINDOW);
}

G_MODULE_EXPORT
InputPadGtkKbduiContext *
input_pad_gtk_kbdui_context_new (void)
{
    InputPadGtkKbduiContext *context;

    context = g_new0 (InputPadGtkKbduiContext, 1);
    context->priv = g_new0 (InputPadGtkKbduiContextPrivate, 1);
    return context;
}

G_MODULE_EXPORT
const gchar *
input_pad_gtk_kbdui_context_get_kbdui_name (InputPadGtkKbduiContext *context)
{
    return context->priv->kbdui_name;
}

G_MODULE_EXPORT
void
input_pad_gtk_kbdui_context_set_kbdui_name (InputPadGtkKbduiContext *context,
                                            const gchar             *name)
{
    g_free (context->priv->kbdui_name);
    if (name) {
        context->priv->kbdui_name = g_strdup (name);
    } else {
        context->priv->kbdui_name = NULL;
    }
}

void
input_pad_gtk_kbdui_context_destroy (InputPadGtkKbduiContext *context)
{
    if (context) {
        if (context->priv) {
            g_free (context->priv->kbdui_name);
            context->priv->kbdui_name = NULL;
            g_free (context->priv);
            context->priv = NULL;
        }
        g_free (context);
    }
}
