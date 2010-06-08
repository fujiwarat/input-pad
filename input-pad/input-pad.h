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

#ifndef __INPUT_PAD_H__
#define __INPUT_PAD_H__

typedef enum
{
    INPUT_PAD_WINDOW_TYPE_GTK = 0,
} InputPadWindowType;

const char *        input_pad_get_version (void);
void                input_pad_window_init (int *argc, char ***argv,
                                           InputPadWindowType type);
void *              input_pad_window_new (unsigned int ibus);
void                input_pad_window_show (void *window_data);
void                input_pad_window_hide (void *window_data);
unsigned int        input_pad_window_get_visible (void *window_data);
void                input_pad_window_main (void *window_data);
void                input_pad_window_destroy (void *window_data);

#endif
