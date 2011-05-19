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

#ifndef __INPUT_PAD_GEOMETRY_GDK_H__
#define __INPUT_PAD_GEOMETRY_GDK_H__

#include "input-pad-window-gtk.h"
#include "geometry-xkb.h"

void                    input_pad_gdk_xkb_destroy_keyboard_layouts
                                        (InputPadGtkWindow     *window,
                                         InputPadXKBKeyList    *xkb_key_list);
InputPadXKBKeyList *    input_pad_gdk_xkb_parse_keyboard_layouts
                                        (InputPadGtkWindow     *window);
void                    input_pad_gdk_xkb_signal_emit
                                        (InputPadGtkWindow     *window,
                                         guint                  signal_id);
char **                 input_pad_gdk_xkb_get_group_layouts
                                        (InputPadGtkWindow     *window,
                                         InputPadXKBKeyList    *xkb_key_list);
char **                 input_pad_gdk_xkb_get_group_variants
                                        (InputPadGtkWindow     *window,
                                         InputPadXKBKeyList    *xkb_key_list);
char **                 input_pad_gdk_xkb_get_group_options
                                        (InputPadGtkWindow     *window,
                                         InputPadXKBKeyList    *xkb_key_list);
InputPadXKBConfigReg *  input_pad_gdk_xkb_parse_config_registry
                                        (InputPadGtkWindow     *window,
                                         InputPadXKBKeyList    *xkb_key_list);
Bool                    input_pad_gdk_xkb_set_layout
                                        (InputPadGtkWindow     *window,
                                         InputPadXKBKeyList    *xkb_key_list,
                                         const char            *layouts,
                                         const char            *variants,
                                         const char            *options);
#endif
