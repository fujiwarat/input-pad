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

#ifndef __INPUT_PAD_VIEWPORT_GTK_H__
#define __INPUT_PAD_VIEWPORT_GTK_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define INPUT_PAD_MAX_COLUMN        15
#define INPUT_PAD_MAX_ROW           66
#define INPUT_PAD_MAX_WINDOW_ROW     8

#define INPUT_PAD_TYPE_GTK_VIEWPORT            (input_pad_gtk_viewport_get_type ())
#define INPUT_PAD_GTK_VIEWPORT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), INPUT_PAD_TYPE_GTK_VIEWPORT, InputPadGtkViewport))
#define INPUT_PAD_GTK_VIEWPORT_CLASS(class)    (G_TYPE_CHECK_CLASS_CAST ((class), INPUT_PAD_TYPE_GTK_VIEWPORT, InputPadGtkViewportClass))
#define INPUT_PAD_IS_GTK_VIEWPORT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), INPUT_PAD_TYPE_GTK_VIEWPORT))

typedef struct _InputPadGtkViewport InputPadGtkViewport;
typedef struct _InputPadGtkViewportPrivate InputPadGtkViewportPrivate;
typedef struct _InputPadGtkViewportClass InputPadGtkViewportClass;

struct _InputPadGtkViewport
{
    GtkBin bin;

    /*< private >*/
    InputPadGtkViewportPrivate         *priv;
};

/**
 * InputPadGtkViewportClass:
 * @parent_class: The parent class.
 */
struct _InputPadGtkViewportClass
{
    GtkBinClass parent_class;

    /*< private >*/

    /* Padding for future expansion */
    void (*_gtk_reserved1) (void);
    void (*_gtk_reserved2) (void);
    void (*_gtk_reserved3) (void);
    void (*_gtk_reserved4) (void);
};

GType               input_pad_gtk_viewport_get_type (void);
GtkWidget *         input_pad_gtk_viewport_new (void);

void                input_pad_gtk_viewport_table_configure
                                       (InputPadGtkViewport     *viewport,
                                        GtkWidget               *table,
                                        unsigned int             min,
                                        unsigned int             max);

G_END_DECLS

#endif
