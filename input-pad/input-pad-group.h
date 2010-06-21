/* vim:set et sts=4: */
/* input-pad - The input pad
 * Copyright (C) 2010 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2010 Red Hat, Inc.
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

#ifndef __INPUT_PAD_GROUP_H__
#define __INPUT_PAD_GROUP_H__

typedef struct _InputPadGroupPrivate InputPadGroupPrivate;
typedef struct _InputPadGroup InputPadGroup;
typedef struct _InputPadTablePrivate InputPadTablePrivate;
typedef struct _InputPadTable InputPadTable;
typedef struct _InputPadTableStr InputPadTableStr;
typedef struct _InputPadTableCmd InputPadTableCmd;
typedef struct _InputPadTableXXX InputPadTableXXX;

typedef enum
{
    INPUT_PAD_TABLE_TYPE_NONE = 0,
    INPUT_PAD_TABLE_TYPE_CHARS,
    INPUT_PAD_TABLE_TYPE_KEYSYMS,
    INPUT_PAD_TABLE_TYPE_STRINGS,
    INPUT_PAD_TABLE_TYPE_COMMANDS,
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
        InputPadTableStr       *strs;
        InputPadTableCmd       *cmds;
        InputPadTableXXX       *_future;
    } data;
    InputPadTable              *next;

    InputPadTablePrivate       *priv;
};

struct _InputPadTableStr {
    char                       *label;
    char                       *comment;
    char                       *rawtext;
};

struct _InputPadTableCmd {
    char                       *label;
    char                       *execl;
};

struct _InputPadTableXXX {
    /* Padding for future expansion */
    void                       *reserved1;
    void                       *reserved2;
    void                       *reserved3;
    void                       *reserved4;
    void                       *reserved5;
};

InputPadGroup * input_pad_group_append_from_file
                               (InputPadGroup        *group_data,
                                const char           *file,
                                const char           *domain);
InputPadGroup * input_pad_group_parse_all_files
                               (const char           *custom_dirname,
                                const char           *domain);
void            input_pad_group_destroy
                               (InputPadGroup        *group_data);

#endif
