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

#include "input-pad-group.h"
#include "button-gtk.h"
#include "i18n.h"

#define TIMEOUT_REPEAT 300
#define INPUT_PAD_GTK_BUTTON_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), INPUT_PAD_TYPE_GTK_BUTTON, InputPadGtkButtonPrivate))

enum {
    PRESSED_REPEAT,
    LAST_SIGNAL,
};

struct _InputPadGtkButtonPrivate {
    guint                       keycode;
    guint                       keysym;
    guint                     **keysyms;
    int                         keysym_group;
    guint                       state;
    gchar *                     rawtext;
    InputPadTableType           type;
    guint32                     timer;
    guint                       timeout_repeat;
};

static guint                    signals[LAST_SIGNAL] = { 0 };
static GtkBuildableIface       *parent_buildable_iface;

static void input_pad_gtk_button_buildable_interface_init (GtkBuildableIface *iface);

#if GTK_CHECK_VERSION (2, 90, 0)
static void input_pad_gtk_button_destroy_real (GtkWidget *widget);
#else
static void input_pad_gtk_button_destroy_real (GtkObject *widget);
#endif

static gint input_pad_gtk_button_press_real (GtkWidget *widget, GdkEventButton *event);
static gint input_pad_gtk_button_release_real (GtkWidget *widget, GdkEventButton *event);

G_DEFINE_TYPE_WITH_CODE (InputPadGtkButton, input_pad_gtk_button,
                         GTK_TYPE_BUTTON,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE,
                                                input_pad_gtk_button_buildable_interface_init))

static void
input_pad_gtk_button_init (InputPadGtkButton *button)
{
    InputPadGtkButtonPrivate *priv = INPUT_PAD_GTK_BUTTON_GET_PRIVATE (button);
    priv->timeout_repeat = TIMEOUT_REPEAT;
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
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
#if !GTK_CHECK_VERSION (2, 90, 0)
    GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
#endif
    GtkWidgetClass *widget_class = (GtkWidgetClass *) klass;

#if GTK_CHECK_VERSION (2, 90, 0)
    widget_class->destroy = input_pad_gtk_button_destroy_real;
#else
    object_class->destroy = input_pad_gtk_button_destroy_real;
#endif
    widget_class->button_press_event = input_pad_gtk_button_press_real;
    widget_class->button_release_event = input_pad_gtk_button_release_real;

    g_type_class_add_private (klass, sizeof (InputPadGtkButtonPrivate));

    signals[PRESSED_REPEAT] =
        g_signal_new (I_("pressed-repeat"),
                      G_TYPE_FROM_CLASS (gobject_class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (InputPadGtkButtonClass, pressed_repeat),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE,
                      0);
}

static gint
button_timer_cb (InputPadGtkButton *button)
{
#if 0
    GtkSettings *settings;
#endif
    guint timeout;

    g_return_val_if_fail (button->priv != NULL, FALSE);

    if (button->priv->timer == 0) {
        return FALSE;
    }

    timeout = button->priv->timeout_repeat;

    /* "gtk-timeout-repeat" is too fast? */
#if 0
    settings = gtk_widget_get_settings (GTK_WIDGET (button));
    g_object_get (settings, "gtk-timeout-repeat", &timeout, NULL);
    //g_object_get (settings, "gtk-timeout-expand", &timeout, NULL);
#endif

    button->priv->timer = gdk_threads_add_timeout (timeout,
                                                   (GSourceFunc) button_timer_cb,
                                                   (gpointer) button);

    g_signal_emit ((gpointer) button, signals[PRESSED_REPEAT], 0);
    return TRUE;
}

static void
start_timer (InputPadGtkButton *button)
{
    GtkSettings *settings;
    guint timeout;

    g_return_if_fail (button->priv != NULL);

    if (button->priv->timer != 0) {
        return;
    }
    settings = gtk_widget_get_settings (GTK_WIDGET (button));
    g_object_get (settings, "gtk-timeout-initial", &timeout, NULL);
    button->priv->timer = gdk_threads_add_timeout (timeout,
                                                   (GSourceFunc) button_timer_cb,
                                                   (gpointer) button);
}

static void
end_timer (InputPadGtkButton *button)
{
    g_return_if_fail (button->priv != NULL);

    if (button->priv->timer == 0) {
        return;
    }
    g_source_remove (button->priv->timer);
    button->priv->timer = 0;
}

static void
#if GTK_CHECK_VERSION (2, 90, 0)
input_pad_gtk_button_destroy_real (GtkWidget *widget)
#else
input_pad_gtk_button_destroy_real (GtkObject *widget)
#endif
{
    InputPadGtkButton *ibutton;
    if (INPUT_PAD_IS_GTK_BUTTON (widget)) {
        ibutton = INPUT_PAD_GTK_BUTTON (widget);
        if (ibutton->priv) {
            end_timer (ibutton);
            g_free (ibutton->priv->rawtext);
            ibutton->priv->rawtext = NULL;
            ibutton->priv = NULL;
        }
    }
#if GTK_CHECK_VERSION (2, 90, 0)
    GTK_WIDGET_CLASS (input_pad_gtk_button_parent_class)->destroy (widget);
#else
    GTK_OBJECT_CLASS (input_pad_gtk_button_parent_class)->destroy (widget);
#endif
}

static gint
input_pad_gtk_button_press_real (GtkWidget      *widget,
                                 GdkEventButton *event)
{
    if (INPUT_PAD_IS_GTK_BUTTON (widget)) {
        start_timer (INPUT_PAD_GTK_BUTTON (widget));
    }
    return GTK_WIDGET_CLASS (input_pad_gtk_button_parent_class)->button_press_event (widget, event);
}

static gint
input_pad_gtk_button_release_real (GtkWidget      *widget,
                                   GdkEventButton *event)
{
    if (INPUT_PAD_IS_GTK_BUTTON (widget)) {
        end_timer (INPUT_PAD_GTK_BUTTON (widget));
    }
    return GTK_WIDGET_CLASS (input_pad_gtk_button_parent_class)->button_release_event (widget, event);
}

GtkWidget *
input_pad_gtk_button_new_with_label (const gchar *label)
{
    return g_object_new (INPUT_PAD_TYPE_GTK_BUTTON, "label", label, NULL); 
}

GtkWidget *
input_pad_gtk_button_new_with_unicode (guint code)
{
    gchar buff[7];
    gchar buff2[35]; /* 7 x 5 e.g. 'a' -> '0x61 ' */
    gchar *tooltip;
    int i;
    GtkWidget *button;

    /* The displaying button is too long with '\t'. */
    if (code == '\t') {
        buff[0] = ' ';
        buff[1] = '\0';
        sprintf (buff2, "0x%02X ", (unsigned char) code);
    } else {
        buff[g_unichar_to_utf8 ((gunichar) code, buff)] = '\0';
        for (i = 0; buff[i] && i < 7; i++) {
            sprintf (buff2 + i * 5, "0x%02X ", (unsigned char) buff[i]);
        }
        if (buff[0] == '\0') {
            buff2[0] = '0'; buff2[0] = 'x'; buff2[1] = '0'; buff2[2] = '0';
            buff2[3] = '\0';
        }
    }

    button = input_pad_gtk_button_new_with_label (buff);
    if (code == '\t') {
        input_pad_gtk_button_set_keysym (INPUT_PAD_GTK_BUTTON (button),
                                         code);
    }
    tooltip = g_strdup_printf ("U+%04X\nUTF-8 %s", code, buff2);
    gtk_widget_set_tooltip_text (GTK_WIDGET (button), tooltip);
    g_free (tooltip);
    input_pad_gtk_button_set_table_type (INPUT_PAD_GTK_BUTTON (button),
                                         INPUT_PAD_TABLE_TYPE_CHARS);

    return button;
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

const gchar *
input_pad_gtk_button_get_rawtext (InputPadGtkButton *button)
{
    g_return_val_if_fail (button != NULL &&
                          INPUT_PAD_IS_GTK_BUTTON (button), NULL);

    return button->priv->rawtext;
}

void
input_pad_gtk_button_set_rawtext (InputPadGtkButton *button,
                                  const gchar       *rawtext)
{
    g_return_if_fail (button != NULL &&
                      INPUT_PAD_IS_GTK_BUTTON (button));

    g_free (button->priv->rawtext);
    button->priv->rawtext = NULL;
    if (rawtext == NULL) {
        return;
    }

    button->priv->rawtext = g_strdup (rawtext);
}
