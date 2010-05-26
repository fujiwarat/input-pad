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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gdk/gdkx.h>
#include <glib.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <X11/extensions/XKB.h>
#include <X11/extensions/XKBstr.h>
#include <X11/extensions/XKBgeom.h>
#include <X11/extensions/XKBfile.h>
#include <X11/extensions/XKM.h>
#include <X11/XKBlib.h>
#include <string.h> /* strlen */

#include "geometry-gdk.h"
#include "input-pad-window-gtk.h"

static gboolean
input_pad_xkb_init ()
{
    XkbInitAtoms (NULL);
    return TRUE;
}

static XkbFileInfo *
input_pad_xkb_get_file_info (InputPadGtkWindow *window)
{
    XkbFileInfo *xkb_info;
    Display *xdisplay = GDK_WINDOW_XDISPLAY (GTK_WIDGET(window)->window);

    xkb_info = g_new0 (XkbFileInfo, 1);
    xkb_info->type = XkmKeymapFile;
    xkb_info->xkb = XkbGetMap (xdisplay, XkbAllMapComponentsMask, XkbUseCoreKbd);
    if (xkb_info->xkb == NULL) {
        g_warning ("Could not get XKB map");
    }
    if (XkbGetNames (xdisplay, XkbAllNamesMask, xkb_info->xkb) !=Success) {
        g_warning ("Could not get XKB names");
    }
    if (XkbGetGeometry (xdisplay, xkb_info->xkb) !=Success) {
        g_warning ("Could not get geometry");
        return NULL;
    }
    if (XkbChangeKbdDisplay (xdisplay, xkb_info) !=Success) {
        g_warning ("Could not get display");
        return NULL;
    }
    return xkb_info;
}

static void
xkb_key_row_set_keycode (InputPadXKBKeyRow     *xkb_key_row,
                         int                    keycode,
                         char                  *name)
{
    char *formatted;

    g_return_if_fail (xkb_key_row != NULL && keycode != 0);

    xkb_key_row->keycode = keycode;
    if (name) {
        formatted = XkbKeyNameText (name, XkbMessage);
        if (strlen (formatted) > 2) {
            xkb_key_row->name = g_strndup (formatted + 1,
                                           strlen (formatted) - 2);
        } else {
            xkb_key_row->name = g_strdup (formatted);
        }
    }
}

static InputPadXKBKeyRow *
xkb_key_row_append (InputPadXKBKeyRow *head,
                    InputPadXKBKeyRow  *new)
{
    InputPadXKBKeyRow *list = head;

    if (list == NULL) {
        return new;
    }
    while (list->next != NULL) {
        list = list->next;
    }
    list->next = new;
    return head;
}

static InputPadXKBKeyList *
xkb_key_list_append (InputPadXKBKeyList *head,
                     InputPadXKBKeyList *new)
{
    InputPadXKBKeyList *list = head;

    if (list == NULL) {
        return new;
    }
    while (list->next != NULL) {
        list = list->next;
    }
    list->next = new;
    return head;
}

static void
get_xkb_section (InputPadXKBKeyList   **xkb_key_listp,
                 XkbDescPtr             xkb,
                 XkbSectionPtr          section)
{
    InputPadXKBKeyRow *xkb_key_row, *xkb_key_row_head;
    InputPadXKBKeyList *list;
    XkbDrawablePtr draw, draw_head;
    XkbRowPtr   row;
    XkbKeyPtr   key;
    XkbShapePtr shape;
    int i, j, k, l, keycode, n_keysyms, groups, n_group, bulk;
    KeySym *keysyms;

    if (section->doodads) {
        draw_head = XkbGetOrderedDrawables(NULL, section);
        for (draw = draw_head; draw; draw = draw->next) {
            if (draw->type == XkbDW_Section) {
                get_xkb_section (xkb_key_listp, xkb, draw->u.section);
            }
        }
        XkbFreeOrderedDrawables (draw_head);
    }

    row = section->rows;
    for (i = 0; i < section->num_rows; i++) {
        xkb_key_row_head = NULL;
        key = row->keys;
        for (j = 0; j < row->num_keys; j++) {
            shape = XkbKeyShape (xkb->geom, key);
            if (key->name.name == NULL) {
                g_warning ("Invalid key name at (%d, %d).\n", i, j);
                goto next_key;
            }
            keycode = XkbFindKeycodeByName (xkb, key->name.name, True);
            if (keycode == 0) {
                g_warning ("%s is not defined in XKB.",
                           XkbKeyNameText (key->name.name, XkbMessage));
                goto next_key;
            }
            keysyms = XkbKeySymsPtr (xkb, keycode);
            n_keysyms = XkbKeyNumSyms (xkb, keycode);
            if (n_keysyms == 0) {
                g_debug ("%s is not included in your keyboard.",
                         XkbKeyNameText (key->name.name, XkbMessage));
                goto next_key;
            }
            xkb_key_row = g_new0 (InputPadXKBKeyRow, 1);
            xkb_key_row_head = xkb_key_row_append (xkb_key_row_head,
                                                   xkb_key_row);
            xkb_key_row_set_keycode (xkb_key_row, keycode, key->name.name);
            groups = XkbKeyNumGroups (xkb, keycode);
            xkb_key_row->keysym = g_new0 (KeySym*, groups + 1);
            bulk = 0;
            for (k = 0; k < groups; k++) {
                n_group = XkbKeyGroupWidth (xkb, keycode, k);
                xkb_key_row->keysym[k] = g_new0 (KeySym, n_group + 1);
                for (l = 0; (l < n_group) && (bulk + l < n_keysyms); l++) {
                    xkb_key_row->keysym[k][l] = keysyms[bulk + l];
                }
                bulk += n_group;
                while (groups > 1 && keysyms[bulk] == 0) {
                    bulk++;
                }
            }
next_key:
            key++;
        }
        if (xkb_key_row_head) {
            list = g_new0 (InputPadXKBKeyList, 1);
            list->row = xkb_key_row_head;
            (*xkb_key_listp) = xkb_key_list_append (*xkb_key_listp,
                                                    list);
        }
        row++;
    }
}

static void
debug_print_layout (InputPadXKBKeyList *xkb_key_list)
{
    InputPadXKBKeyList *list = xkb_key_list;
    int i, j, k;
    GString *string;
    char *line;

    if (xkb_key_list == NULL) {
        return;
    }
    if (!g_getenv ("G_MESSAGES_PREFIXED")) {
        return;
    }

    for (i = 1; list; i++) {
        string = g_string_new ("\n");
        InputPadXKBKeyRow *row = list->row;
        while (row) {
            g_string_append_printf (string, "%d %s keycode(%x) ",
                                    i, row->name, row->keycode);
            for (j = 0; row->keysym && row->keysym[j]; j++) {
                for (k = 0; row->keysym[j][k]; k++) {
                    g_string_append_printf (string, "keysym%d(%s) ",
                                            j, XKeysymToString (row->keysym[j][k]));
                }
            }
            g_string_append_printf (string, "\n");
            row = row->next;
        }
        line = g_string_free (string, FALSE);
        if (line) {
            g_debug (line);
            g_free (line);
        }
        list = list->next;
    }
}

InputPadXKBKeyList *
input_pad_xkb_parse_keyboard_layouts (InputPadGtkWindow   *window)
{
    XkbFileInfo *xkb_info;
    XkbDrawablePtr draw, draw_head;
    static InputPadXKBKeyList *xkb_key_list = NULL;
    
    if (xkb_key_list) {
        return xkb_key_list;
    }

    g_return_val_if_fail (window != NULL &&
                          INPUT_PAD_IS_GTK_WINDOW (window), NULL);

    input_pad_xkb_init ();
    if ((xkb_info = input_pad_xkb_get_file_info (window)) == NULL) {
        return NULL;
    }
    draw_head = XkbGetOrderedDrawables(xkb_info->xkb->geom, NULL);
    for (draw = draw_head; draw; draw = draw->next) {
        if (draw->type == XkbDW_Section) {
            get_xkb_section (&xkb_key_list, xkb_info->xkb, draw->u.section);
        }
    }
    XkbFreeOrderedDrawables (draw_head);
    debug_print_layout (xkb_key_list);
    return xkb_key_list;
}
