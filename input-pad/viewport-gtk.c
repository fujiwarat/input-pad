/* vim:set et sts=4: */
/* input-pad - The input pad
 * Copyright (C) 2014 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2014 Red Hat, Inc.
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

#include <gtk/gtk.h>

#include "button-gtk.h"
#include "viewport-gtk.h"

#define INPUT_PAD_STEP_INCREMENT 20
#define INPUT_PAD_PAGE_INCREMENT 180
#define INPUT_PAD_PAGE_SIZE 200

enum {
    PROP_0,
    PROP_HADJUSTMENT,
    PROP_VADJUSTMENT,
    PROP_HSCROLL_POLICY,
    PROP_VSCROLL_POLICY,
};

struct _InputPadGtkViewportPrivate
{
    GtkAdjustment  *hadjustment;
    GtkAdjustment  *vadjustment;


    guint           hscroll_policy : 1;
    guint           vscroll_policy : 1;

    GtkWidget      *table;

    unsigned int    table_code_min;
    unsigned int    table_code_max;

    /* Do not use gtk_adjustment_configure() directly.
     * When input_pad_gtk_viewport_new() is called,
     * input_pad_gtk_viewport_set_vadjustment() is called with set_property
     * internally.
     * But the adjustment will be overrid by
     * input_pad_gtk_viewport_set_vadjustment() again when
     * gtk_container_add() -> gtk_scrolled_window_add() is called.
     * the new adjustment is copied from GtkScrolledWindow to
     * InputPadGtkViewport so the adjustment values need to be saved here.
     */
    double          value;
    double          lower;
    double          upper;
    double          step_increment;
    double          page_increment;
    double          page_size;
};


static void input_pad_gtk_viewport_set_property   (GObject         *object,
                                                   guint            prop_id,
                                                   const GValue    *value,
                                                   GParamSpec      *pspec);
static void input_pad_gtk_viewport_get_property   (GObject         *object,
                                                   guint            prop_id,
                                                   GValue          *value,
                                                   GParamSpec      *pspec);


G_DEFINE_TYPE_WITH_CODE (InputPadGtkViewport, input_pad_gtk_viewport,
                         GTK_TYPE_BIN,
                         G_ADD_PRIVATE (InputPadGtkViewport)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_SCROLLABLE,
                         NULL))

static void
update_scrollbar_adjustment (InputPadGtkViewport *viewport)
{
    InputPadGtkViewportPrivate *priv = viewport->priv;

    if (!priv->vadjustment)
        return;

    gtk_adjustment_configure (priv->vadjustment,
                              priv->value,
                              priv->lower,
                              priv->upper,
                              priv->step_increment,
                              priv->page_increment,
                              priv->page_size);
}

static void
input_pad_gtk_viewport_hadjustment_value_changed_cb (GtkAdjustment *hadjustment,
                                                     gpointer       data)
{
}

static void
input_pad_gtk_viewport_vadjustment_value_changed_cb (GtkAdjustment *vadjustment,
                                                     gpointer       data)
{
    InputPadGtkViewport *viewport = INPUT_PAD_GTK_VIEWPORT (data);
    InputPadGtkViewportPrivate *priv = viewport->priv;
    GtkWidget *table = priv->table;
    double value = gtk_adjustment_get_value (vadjustment);
    double step = gtk_adjustment_get_step_increment (vadjustment);
#if 0
    double page_incre = gtk_adjustment_get_page_increment (vadjustment);
    double page = gtk_adjustment_get_page_size (vadjustment);
    double upper = gtk_adjustment_get_upper (vadjustment);
#endif
    unsigned int start;
#if 0
    unsigned int end;
#endif
    unsigned int min = priv->table_code_min;
    unsigned int max = priv->table_code_max;
    unsigned int row;
    unsigned int num;
    GList *orig_list, *list;

    if (table == NULL)
        return;

    row = (int) (value / step);
    start = min + row * INPUT_PAD_MAX_COLUMN;
#if 0
    end = start + INPUT_PAD_MAX_COLUMN * INPUT_PAD_MAX_WINDOW_ROW - 1;
#endif
    num = start;

#if 0
    g_warning ("test value-changed %lf %lf %lf %lf %lf\n",
               value, step, page_incre, page, upper);
    g_warning ("test value-changed %d %d %d %d\n",
               start, end, min, max);
#endif

    list = gtk_container_get_children (GTK_CONTAINER (table));
    list = g_list_reverse (list);
    orig_list = list;
    while (list != NULL) {
        InputPadGtkButton *button = list->data;
        if (num > max)
            input_pad_gtk_button_set_unicode (button, 0);
        else
            input_pad_gtk_button_set_unicode (button, num++);
        list = list->next;
    }
    g_list_free (orig_list);
}

static void
input_pad_gtk_viewport_set_hadjustment (InputPadGtkViewport    *viewport,
                                        GtkAdjustment          *adjustment)
{
    InputPadGtkViewportPrivate *priv = viewport->priv;

    if (adjustment)
        g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));
    else
        adjustment = gtk_adjustment_new (0., 0., 0., 0., 0., 0.);

    if (priv->hadjustment) {
        g_signal_handlers_disconnect_by_func (priv->hadjustment,
                                              input_pad_gtk_viewport_hadjustment_value_changed_cb,
                                              viewport);
        g_object_unref (priv->hadjustment);
        priv->hadjustment = NULL;
    }
    priv->hadjustment = adjustment;
    g_object_ref_sink (priv->hadjustment);

    g_signal_connect (priv->hadjustment, "value-changed",
                      G_CALLBACK (input_pad_gtk_viewport_hadjustment_value_changed_cb),
                      viewport);
}

static void
input_pad_gtk_viewport_set_vadjustment (InputPadGtkViewport    *viewport,
                                        GtkAdjustment          *adjustment)
{
    InputPadGtkViewportPrivate *priv = viewport->priv;

    if (adjustment)
        g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));
    else
        adjustment = gtk_adjustment_new (0., 0., 0., 0., 0., 0.);

    if (priv->vadjustment) {
        g_signal_handlers_disconnect_by_func (priv->vadjustment,
                                              input_pad_gtk_viewport_vadjustment_value_changed_cb,
                                              viewport);
        g_object_unref (priv->vadjustment);
        priv->vadjustment = NULL;
    }
    priv->vadjustment = adjustment;
    g_object_ref_sink (priv->vadjustment);

    g_signal_connect (priv->vadjustment, "value-changed",
                      G_CALLBACK (input_pad_gtk_viewport_vadjustment_value_changed_cb),
                      viewport);

    update_scrollbar_adjustment (viewport);
}

static void
input_pad_gtk_viewport_class_init (InputPadGtkViewportClass *class)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (class);

    gobject_class->get_property = input_pad_gtk_viewport_get_property;
    gobject_class->set_property = input_pad_gtk_viewport_set_property;

    g_object_class_override_property (gobject_class,
                                      PROP_HADJUSTMENT,
                                      "hadjustment");
    g_object_class_override_property (gobject_class,
                                      PROP_VADJUSTMENT,
                                      "vadjustment");
    g_object_class_override_property (gobject_class,
                                      PROP_HSCROLL_POLICY,
                                      "hscroll-policy");
    g_object_class_override_property (gobject_class,
                                      PROP_VSCROLL_POLICY,
                                      "vscroll-policy");
}

static void
input_pad_gtk_viewport_get_property (GObject         *object,
                                     guint            prop_id,
                                     GValue          *value,
                                     GParamSpec      *pspec)
{
    InputPadGtkViewport *viewport = INPUT_PAD_GTK_VIEWPORT (object);
    InputPadGtkViewportPrivate *priv = viewport->priv;

    switch (prop_id) {
    case PROP_HADJUSTMENT:
        g_value_set_object (value, priv->hadjustment);
        break;
    case PROP_VADJUSTMENT:
        g_value_set_object (value, priv->vadjustment);
        break;
    case PROP_HSCROLL_POLICY:
        g_value_set_enum (value, priv->hscroll_policy);
        break;
    case PROP_VSCROLL_POLICY:
        g_value_set_enum (value, priv->vscroll_policy);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
input_pad_gtk_viewport_set_property (GObject         *object,
                                     guint            prop_id,
                                     const GValue    *value,
                                     GParamSpec      *pspec)
{
    InputPadGtkViewport *viewport;

    viewport = INPUT_PAD_GTK_VIEWPORT (object);

    switch (prop_id) {
    case PROP_HADJUSTMENT:
        input_pad_gtk_viewport_set_hadjustment (viewport,
                                                g_value_get_object (value));
        break;
    case PROP_VADJUSTMENT:
        input_pad_gtk_viewport_set_vadjustment (viewport,
                                                g_value_get_object (value));
        break;
    case PROP_HSCROLL_POLICY:
        if (viewport->priv->hscroll_policy != g_value_get_enum (value)) {
            viewport->priv->hscroll_policy = g_value_get_enum (value);
            //gtk_widget_queue_resize (GTK_WIDGET (viewport));
            gtk_widget_queue_resize_no_redraw (GTK_WIDGET (viewport));
            g_object_notify_by_pspec (object, pspec);
        }
        break;
    case PROP_VSCROLL_POLICY:
        if (viewport->priv->vscroll_policy != g_value_get_enum (value)) {
            viewport->priv->vscroll_policy = g_value_get_enum (value);
            //gtk_widget_queue_resize (GTK_WIDGET (viewport));
            gtk_widget_queue_resize_no_redraw (GTK_WIDGET (viewport));
            g_object_notify_by_pspec (object, pspec);
        }
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
input_pad_gtk_viewport_init (InputPadGtkViewport *viewport)
{
    InputPadGtkViewportPrivate *priv;

    viewport->priv = input_pad_gtk_viewport_get_instance_private (viewport);
    priv = viewport->priv;
    priv->step_increment = INPUT_PAD_STEP_INCREMENT;
    priv->page_increment = INPUT_PAD_PAGE_INCREMENT;
    priv->page_size = INPUT_PAD_PAGE_SIZE;

}

GtkWidget *
input_pad_gtk_viewport_new (void)
{
    return g_object_new (INPUT_PAD_TYPE_GTK_VIEWPORT, NULL);
}

void
input_pad_gtk_viewport_table_configure (InputPadGtkViewport *viewport,
                                        GtkWidget           *table,
                                        unsigned int         min,
                                        unsigned int         max)
{
    InputPadGtkViewportPrivate *priv;
    unsigned int upper;

    g_return_if_fail (INPUT_PAD_IS_GTK_VIEWPORT (viewport));
    g_return_if_fail (GTK_IS_GRID (table));

    priv = viewport->priv;

    priv->table = table;
    priv->table_code_min = min;
    priv->table_code_max = max;

    upper = (priv->table_code_max - priv->table_code_min + 1)
            / INPUT_PAD_MAX_COLUMN;
    if ((priv->table_code_max - priv->table_code_min + 1)
            % INPUT_PAD_MAX_COLUMN > 0)
        upper++;
    upper = (upper - INPUT_PAD_MAX_WINDOW_ROW + 1)
            * INPUT_PAD_STEP_INCREMENT + INPUT_PAD_PAGE_INCREMENT;

    if (upper >= 0.)
        priv->upper = (double) upper;

    priv->step_increment = INPUT_PAD_STEP_INCREMENT;
    priv->page_increment = INPUT_PAD_PAGE_INCREMENT;
    priv->page_size = INPUT_PAD_PAGE_SIZE;

    update_scrollbar_adjustment (viewport);
}
