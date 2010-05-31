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

#include <gtk/gtk.h>
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

#ifdef HAVE_LIBXKLAVIER
#include <libxklavier/xklavier.h>
#endif

#include <string.h> /* strlen */

#include "geometry-gdk.h"
#include "input-pad-window-gtk.h"

#ifdef HAVE_LIBXKLAVIER
#if 0
extern gboolean xkl_engine_find_toplevel_window(XklEngine * engine,
                                                Window win,
                                                Window * toplevel_win_out);
extern void xkl_engine_save_toplevel_window_state(XklEngine * engine,
                                                  Window toplevel_win,
                                                  XklState * state);
#endif
#endif

#ifdef HAVE_LIBXKLAVIER
typedef struct _XklSignalData XklSignalData;

static XklEngine *xklengine;

struct _XklSignalData {
    GObject   *object;
    guint      signal_id;
};
#endif

static gboolean
input_pad_xkb_init (InputPadGtkWindow *window)
{
    static gboolean retval = FALSE;
    Display *xdisplay = GDK_WINDOW_XDISPLAY (GTK_WIDGET(window)->window);

    if (retval) {
        return retval;
    }

    if (!XkbQueryExtension (xdisplay, NULL, NULL, NULL, NULL, NULL)) {
        g_warning ("Could not init XKB");
        return FALSE;
    }

    XkbInitAtoms (NULL);
    retval = TRUE;
    return TRUE;
}

static XkbFileInfo *
input_pad_xkb_get_file_info (InputPadGtkWindow *window)
{
    Display *xdisplay = GDK_WINDOW_XDISPLAY (GTK_WIDGET(window)->window);
    XkbFileInfo *xkb_info;

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

static gboolean
xkb_key_list_insert_row_with_name (InputPadXKBKeyList *head,
                                   InputPadXKBKeyRow  *new_row,
                                   const gchar        *find_name)
{
    InputPadXKBKeyList *list = head;
    InputPadXKBKeyRow *row, *row_backup;
    gboolean found = FALSE;

    g_return_val_if_fail (head != NULL && new_row != NULL, FALSE);
    g_return_val_if_fail (find_name != NULL, FALSE);

    while (list != NULL) {
        row = list->row;
        while (row) {
            if (!g_strcmp0 (row->name, find_name)) {
                row_backup = row->next;
                row->next = new_row;
                new_row->next = row_backup;
                found = TRUE;

                goto end_insert_row;
            }
            row = row->next;
        }
        list = list->next;
    }

end_insert_row:
    if (found) {
        return TRUE;
    }
    return FALSE;
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
            xkb_key_row->keysym = g_new0 (unsigned int *, groups + 1);
            bulk = 0;
            for (k = 0; k < groups; k++) {
                n_group = XkbKeyGroupWidth (xkb, keycode, k);
                xkb_key_row->keysym[k] = g_new0 (unsigned int, n_group + 1);
                for (l = 0; (l < n_group) && (bulk + l < n_keysyms); l++) {
                    xkb_key_row->keysym[k][l] = (unsigned int) keysyms[bulk + l];
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
add_xkb_key (InputPadXKBKeyList        *xkb_key_list,
             XkbDescPtr                 xkb,
             const gchar               *new_key_name,
             const gchar               *prev_key_name)
{
    XkbKeyRec key_buff;
    XkbKeyPtr key;
    int k, l, n_keysyms, groups, n_group, bulk;
    unsigned int keycode;
    KeySym *keysyms;
    InputPadXKBKeyRow *xkb_key_row;

    g_return_if_fail (new_key_name != NULL && prev_key_name != NULL);

    memset (&key_buff, 0, sizeof (XkbKeyRec));
    strcpy (key_buff.name.name, new_key_name);
    key_buff.gap = 0;
    key_buff.shape_ndx = 0;
    key_buff.color_ndx = 1;
    key = &key_buff;

    keycode = XkbFindKeycodeByName (xkb, key->name.name, True);
    if (keycode == 0) {
        g_debug ("%s is not defined in XKB.",
                 XkbKeyNameText (key->name.name, XkbMessage));
        return;
    }

    keysyms = XkbKeySymsPtr (xkb, keycode);
    n_keysyms = XkbKeyNumSyms (xkb, keycode);
    if (n_keysyms == 0) {
        g_debug ("%s is not included in your keyboard.",
                 XkbKeyNameText (key->name.name, XkbMessage));
        return;
    }
    xkb_key_row = g_new0 (InputPadXKBKeyRow, 1);
    xkb_key_list_insert_row_with_name (xkb_key_list,
                                       xkb_key_row,
                                       prev_key_name);
    xkb_key_row_set_keycode (xkb_key_row, keycode, key->name.name);
    groups = XkbKeyNumGroups (xkb, keycode);
    xkb_key_row->keysym = g_new0 (unsigned int *, groups + 1);
    bulk = 0;
    for (k = 0; k < groups; k++) {
        n_group = XkbKeyGroupWidth (xkb, keycode, k);
        xkb_key_row->keysym[k] = g_new0 (unsigned int, n_group + 1);
        for (l = 0; (l < n_group) && (bulk + l < n_keysyms); l++) {
            xkb_key_row->keysym[k][l] = (unsigned int) keysyms[bulk + l];
        }
        bulk += n_group;
        while (groups > 1 && keysyms[bulk] == 0) {
            bulk++;
        }
    }
}

#ifdef HAVE_LIBXKLAVIER
static XklEngine *
input_pad_xkl_init (InputPadGtkWindow *window)
{
    Display *xdisplay = GDK_WINDOW_XDISPLAY (GTK_WIDGET(window)->window);
    XklConfigRec *xklrec;

    if (xklengine) {
        return xklengine;
    }

    xklrec = xkl_config_rec_new ();
    xklengine = xkl_engine_get_instance (xdisplay);

    if (!xkl_config_rec_get_from_server (xklrec, xklengine)) {
        xkl_debug (150, "Could not load keyboard config from server: [%s]\n",
                   xkl_get_last_error ());
    }
    return xklengine;
}
#endif

#if 0
static void
update_keysym_mapping (Display *xdisplay,
                       InputPadXKBKeyList   *xkb_key_list,
                       int group)
{
    int n;
    InputPadXKBKeyList *list = xkb_key_list;
    InputPadXKBKeyRow *row;
    KeySym keysyms[20];
    int i, j, min_keycode, max_keycode;

    XDisplayKeycodes (xdisplay, &min_keycode, &max_keycode);
    for (i = min_keycode; i < max_keycode; i++) {
        KeySym *sym = XGetKeyboardMapping (xdisplay, i, 1, &n);
    }
    while (list) {
        row = list->row;
        while (row) {
            n = 0;
            while (row->keysym[n]) n++;
            if (n <= group) {
                row = row->next;
                continue;
            }
            for (n = 0; row->keysym[group][n] && (n < 20); n++) {
                keysyms[n] = (KeySym) row->keysym[group][n];
            }
            XChangeKeyboardMapping (xdisplay, row->keycode, n,
                                    keysyms, 1);
            row = row->next;
        }
        list = list->next;
    }
}
#endif

#ifdef HAVE_LIBXKLAVIER
static GdkFilterReturn
on_filter_x_evt (GdkXEvent * xev, GdkEvent * event, gpointer data)
{
    XEvent *xevent = (XEvent *) xev;

    xkl_engine_filter_events (xklengine, xevent);
    return GDK_FILTER_CONTINUE;
}

static void
on_state_changed (XklEngine * engine,
                  XklEngineStateChange changeType,
                  gint group, gboolean restore,
                  gpointer data)
{
    XklState *state;
    XklSignalData *signal_data = (XklSignalData *) data;
#if 0
    Window toplevel_win;
#endif

    if (changeType != GROUP_CHANGED) {
        return;
    }

    state = xkl_engine_get_current_state (xklengine);
    g_return_if_fail (data != NULL);

#if 0
    if (!xkl_engine_find_toplevel_window (xklengine, 
                                          GDK_WINDOW_XWINDOW (GTK_WIDGET(signal_data->object)->window),
                                          &toplevel_win)) {
        g_warning ("Could not find toplevel window");
    }
    xkl_engine_save_toplevel_window_state (xklengine, toplevel_win, state);
#endif

    g_signal_emit (signal_data->object, signal_data->signal_id, 0,
                   state->group);
}

static void
xkl_setup_events (XklEngine            *xklengine,
                  InputPadGtkWindow    *window,
                  guint                 signal_id)
{
    static XklSignalData signal_data;

    signal_data.object = G_OBJECT (window);
    signal_data.signal_id = signal_id;

    g_signal_connect (xklengine, "X-state-changed",
                      G_CALLBACK (on_state_changed),
                      (gpointer) &signal_data);

    gdk_window_add_filter (NULL, (GdkFilterFunc)
                           on_filter_x_evt, NULL);
    gdk_window_add_filter (gdk_get_default_root_window (), (GdkFilterFunc)
                           on_filter_x_evt, NULL);
    xkl_engine_start_listen (xklengine,
                             XKLL_TRACK_KEYBOARD_STATE);
}
#endif

guint
xkb_get_current_group (InputPadGtkWindow *window)
{
    Display *xdisplay = GDK_WINDOW_XDISPLAY (GTK_WIDGET(window)->window);
    XkbStateRec state;

    if (XkbGetState (xdisplay, XkbUseCoreKbd, &state) != Success) {
        g_warning ("Could not get state");
        return 0;
    }

    return state.group;
}

static void
xkb_setup_events (InputPadGtkWindow    *window,
                  guint                 signal_id)
{
    guint group = 0;

    if (!input_pad_xkb_init (window)) {
        return;
    }

    group = xkb_get_current_group (window);
    g_signal_emit (G_OBJECT (window), signal_id, 0, group);

#ifdef HAVE_LIBXKLAVIER
    xkl_setup_events (xklengine, window, signal_id);
#endif
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
                                            j, XKeysymToString ((KeySym) row->keysym[j][k]));
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

    if (!input_pad_xkb_init (window)) {
        return NULL;
    }
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
    /* Japanese extension */
    add_xkb_key (xkb_key_list, xkb_info->xkb, "AE13", "AE12");
    add_xkb_key (xkb_key_list, xkb_info->xkb, "AB11", "AB10");
    debug_print_layout (xkb_key_list);

#ifdef HAVE_LIBXKLAVIER
    xklengine = input_pad_xkl_init (window);
#endif

    return xkb_key_list;
}

void
input_pad_xkb_signal_emit (InputPadGtkWindow   *window, guint signal_id)
{
    g_return_if_fail (window != NULL && INPUT_PAD_IS_GTK_WINDOW (window));

    xkb_setup_events (window, signal_id);
}
