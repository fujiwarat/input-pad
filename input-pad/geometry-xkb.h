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

#ifndef __INPUT_PAD_GEOMETRY_XKB_H__
#define __INPUT_PAD_GEOMETRY_XKB_H__

#include <X11/Xproto.h>
#include <X11/extensions/XKB.h>

#define input_pad_xkb_build_core_state XkbBuildCoreState

typedef struct _InputPadXKBKeyRow  InputPadXKBKeyRow;
typedef struct _InputPadXKBKeyRowPrivate  InputPadXKBKeyRowPrivate;
typedef struct _InputPadXKBKeyList InputPadXKBKeyList;
typedef struct _InputPadXKBKeyListPrivate InputPadXKBKeyListPrivate;
typedef struct _InputPadXKBConfigReg  InputPadXKBConfigReg;
typedef struct _InputPadXKBConfigRegPrivate  InputPadXKBConfigRegPrivate;
typedef struct _InputPadXKBLayoutList  InputPadXKBLayoutList;
typedef struct _InputPadXKBLayoutListPrivate  InputPadXKBLayoutListPrivate;
typedef struct _InputPadXKBVariantList  InputPadXKBVariantList;
typedef struct _InputPadXKBVariantListPrivate  InputPadXKBVariantListPrivate;
typedef struct _InputPadXKBOptionGroupList  InputPadXKBOptionGroupList;
typedef struct _InputPadXKBOptionGroupListPrivate  InputPadXKBOptionGroupListPrivate;
typedef struct _InputPadXKBOptionList  InputPadXKBOptionList;
typedef struct _InputPadXKBOptionListPrivate  InputPadXKBOptionListPrivate;

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

struct _InputPadXKBConfigReg {
    InputPadXKBLayoutList              *layouts;
    InputPadXKBOptionGroupList         *option_groups;
    InputPadXKBConfigRegPrivate        *priv;
};

struct _InputPadXKBLayoutList {
    char                               *layout;
    char                               *desc;
    InputPadXKBVariantList             *variants;
    InputPadXKBLayoutList              *next;
    InputPadXKBLayoutListPrivate       *priv;
};

struct _InputPadXKBVariantList {
    char                               *variant;
    char                               *desc;
    InputPadXKBVariantList             *next;
    InputPadXKBVariantListPrivate      *priv;
};

struct _InputPadXKBOptionGroupList {
    char                               *option_group;
    char                               *desc;
    InputPadXKBOptionList              *options;
    InputPadXKBOptionGroupList         *next;
    InputPadXKBOptionGroupListPrivate  *priv;
};

struct _InputPadXKBOptionList {
    char                               *option;
    char                               *desc;
    InputPadXKBOptionList              *next;
    InputPadXKBOptionListPrivate       *priv;
};

#endif
