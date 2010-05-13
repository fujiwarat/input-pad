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

#ifndef __INPUT_PAD_GROUP_H__
#define __INPUT_PAD_GROUP_H__

typedef struct _InputPadGroupPrivate InputPadGroupPrivate;
typedef struct _InputPadGroup InputPadGroup;
typedef struct _InputPadTablePrivate InputPadTablePrivate;
typedef struct _InputPadTable InputPadTable;
typedef struct _InputPadKey InputPadKey;

typedef enum
{
    INPUT_PAD_TABLE_TYPE_NONE = 0,
    INPUT_PAD_TABLE_TYPE_CHARS,
    INPUT_PAD_TABLE_TYPE_KEYSYMS,
} InputPadTableType;

struct _InputPadGroup {
    char                       *name;
    InputPadTable              *table;
    InputPadGroup              *next;

    InputPadGroupPrivate       *priv;
};

struct _InputPadTable {
    char                       *name;
    int                         column;
    InputPadTableType           type;
    union data {
        char                   *chars;
        char                   *keysyms;
        InputPadKey            *key;
    } data;
    InputPadTable              *next;

    InputPadTablePrivate       *priv;
};

InputPadGroup * input_pad_group_append_from_file
                               (InputPadGroup        *group_data,
                                const char           *file);
InputPadGroup * input_pad_group_parse_all_files  (void);
void            input_pad_group_destroy
                               (InputPadGroup        *group_data);

#endif
