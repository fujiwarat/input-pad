/* vim:set et sts=4: */
/* input-pad - The input pad
 * Copyright (C) 2010 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2010 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __INPUT_PAD_GEOMETRY_GDK_H__
#define __INPUT_PAD_GEOMETRY_GDK_H__

#include <X11/Xproto.h>
#include <X11/extensions/XKB.h>
#include "input-pad-window-gtk.h"

#define input_pad_xkb_build_core_state XkbBuildCoreState

typedef struct _InputPadXKBKeyRow  InputPadXKBKeyRow;
typedef struct _InputPadXKBKeyRowPrivate  InputPadXKBKeyRowPrivate;
typedef struct _InputPadXKBKeyList InputPadXKBKeyList;
typedef struct _InputPadXKBKeyListPrivate InputPadXKBKeyListPrivate;

struct _InputPadXKBKeyRow {
    KeyCode                     keycode;
    char                       *name;
    unsigned int              **keysym;
    InputPadXKBKeyRow          *next;
    InputPadXKBKeyRowPrivate   *priv;
};

struct _InputPadXKBKeyList {
    InputPadXKBKeyRow          *row;
    InputPadXKBKeyList         *next;
    InputPadXKBKeyListPrivate  *priv;
};

InputPadXKBKeyList     *input_pad_xkb_parse_keyboard_layouts
                                        (InputPadGtkWindow *window);
void                    input_pad_xkb_signal_emit
                                        (InputPadGtkWindow *window,
                                         guint              signal_id);

#endif
