/* vim:set et sts=4: */
/* input-pad - The input pad
 * Copyright (C) 2010-2014 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2010-2014 Red Hat, Inc.
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

#ifndef __INPUT_PAD_H__
#define __INPUT_PAD_H__

typedef enum
{
    INPUT_PAD_WINDOW_TYPE_GTK = 0,
} InputPadWindowType;

typedef enum
{
    INPUT_PAD_WINDOW_SHOW_TABLE_TYPE_NOTHING = 0,
    INPUT_PAD_WINDOW_SHOW_TABLE_TYPE_CUSTOM,
    INPUT_PAD_WINDOW_SHOW_TABLE_TYPE_ALL,
} InputPadWindowShowTableType;

typedef enum
{
    INPUT_PAD_WINDOW_SHOW_LAYOUT_TYPE_NOTHING = 0,
    INPUT_PAD_WINDOW_SHOW_LAYOUT_TYPE_DEFAULT,
} InputPadWindowShowLayoutType;

typedef struct _InputPadWindowKbduiName InputPadWindowKbduiName;

struct _InputPadWindowKbduiName {
    char               *name;
    char               *description;
    InputPadWindowType  type;
};

const char *        input_pad_get_version (void);

/**
 * input_pad_window_init:
 * @argc: (inout): Address of the argc
 * @argv: (array length=argc) (inout) (allow-none): Address of the argv
 * @type: currently it's ignored.
 * @do_exit: (out): If this is %TRUE, can call exit.
 *
 * Returns: the exit value.
 */
int                 input_pad_window_init (int         *argc,
                                           char      ***argv,
                                           InputPadWindowType type,
                                           int         *do_exit);
void *              input_pad_window_new (void);
void                input_pad_window_show (void *window_data);
void                input_pad_window_hide (void *window_data);
unsigned int        input_pad_window_get_visible (void *window_data);
void                input_pad_window_set_paddir
                                        (void          *window_data,
                                         const char    *paddir,
                                         const char    *domain);
void                input_pad_window_append_padfile
                                        (void          *window_data,
                                         const char    *padfile,
                                         const char    *domain);
void                input_pad_window_set_char_button_sensitive
                                        (void          *window_data,
                                         unsigned int   sensitive);
void                input_pad_window_reorder_button_pressed
                                        (void          *window_data);
InputPadWindowKbduiName *
                    input_pad_window_get_kbdui_name_list (void);
int                 input_pad_window_get_kbdui_name_list_length (void);
char *              input_pad_window_get_kbdui_name_get_name_with_index
                                        (int            index);
char *              input_pad_window_get_kbdui_name_get_description_with_index
                                        (int            index);
InputPadWindowType  input_pad_window_get_kbdui_name_get_type_with_index
                                        (int            index);
void                input_pad_window_set_kbdui_name
                                        (void          *window_data,
                                         const char    *name);
void                input_pad_window_set_show_table
                                        (void           *window_data,
                                         InputPadWindowShowTableType type);
void                input_pad_window_set_show_layout
                                        (void           *window_data,
                                         InputPadWindowShowLayoutType type);
void *              input_pad_window_get_window (void *window_data);
void                input_pad_window_activate (void *window_data);
int                 input_pad_window_main (void *window_data);
void                input_pad_window_destroy (void *window_data);

#endif
