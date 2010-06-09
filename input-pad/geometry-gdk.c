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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <glib.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h> /* XA_STRING */
#include <stdio.h>
#include <X11/extensions/XKB.h>
#include <X11/extensions/XKBstr.h>
#include <X11/extensions/XKBgeom.h>
#include <X11/extensions/XKBfile.h>
#include <X11/extensions/XKM.h>
#include <X11/XKBlib.h>

#ifdef HAVE_LIBXKLAVIER
#include <libxklavier/xklavier.h>
#else
#include <X11/extensions/XKBrules.h>
#endif

#include <string.h> /* strlen */

#include "input-pad-window-gtk.h"
#include "geometry-gdk.h"

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
typedef struct _XklLayoutData XklLayoutData;

static XklEngine *xklengine;
static XklConfigRec *initial_xkl_rec;

struct _XklSignalData {
    GObject   *object;
    guint      signal_id;
};

struct _XklLayoutData {
    const XklConfigItem   *layout;
    InputPadXKBConfigReg **config_regp;
};
#endif

struct _InputPadXKBKeyListPrivate {
    XkbFileInfo  *xkb_info;
};

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
    strncpy (key_buff.name.name, new_key_name, XkbKeyNameLength);
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
init_xkl_engine (InputPadGtkWindow *window, XklConfigRec **initial_xkl_recp)
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
    if (initial_xkl_recp) {
        *initial_xkl_recp = xklrec;
    }
    return xklengine;
}

static XklConfigRegistry *
init_xkl_config_registry (InputPadGtkWindow *window)
{
    static XklConfigRegistry *xklconfig_registry = NULL;

    g_return_val_if_fail (xklengine != NULL, NULL);

    if (xklconfig_registry) {
        return xklconfig_registry;
    }
    xklconfig_registry = xkl_config_registry_get_instance (xklengine);
    xkl_config_registry_load (xklconfig_registry, FALSE);
    return xklconfig_registry;
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

static guint
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
debug_print_key_list (InputPadXKBKeyList *xkb_key_list)
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

static void
debug_print_layout_list (InputPadXKBLayoutList *xkb_layout_list)
{
    InputPadXKBLayoutList *list = xkb_layout_list;
    InputPadXKBVariantList *variants;
    int i, j;

    if (xkb_layout_list == NULL) {
        return;
    }
    if (!g_getenv ("G_MESSAGES_PREFIXED")) {
        return;
    }

    for (i = 0; list; i++) {
        variants = list->variants;
        for (j = 0; variants; j++) {
            g_debug ("%d %s %s %d %s %s",
                     i, list->layout, list->desc ? list->desc : "(null)",
                     j, variants->variant, variants->desc ? variants->desc : "(null)");
            variants = variants->next;
        }
        list = list->next;
    }
}

static void
debug_print_group_layout_list (gchar **names)
{
    int i;

    if (!g_getenv ("G_MESSAGES_PREFIXED")) {
        return;
    }

    g_return_if_fail (names != NULL);

    for (i = 0; names[i]; i++) {
        g_debug ("group%d: %s\n", i, names[i]);
    }
}

#ifdef HAVE_LIBXKLAVIER
static void
input_pad_xkb_layout_list_append_layout_variant (InputPadXKBLayoutList *xkb_layout_list,
                                                 const XklConfigItem   *layout,
                                                 const XklConfigItem   *variant)
{
    InputPadXKBLayoutList *list = xkb_layout_list;
    InputPadXKBVariantList *variants;

    g_return_if_fail (xkb_layout_list != NULL);
    g_return_if_fail (layout != NULL && layout->name != NULL);
    g_return_if_fail (variant != NULL && variant->name != NULL);

    while (list) {
        if (list->layout == NULL) {
            list->layout = g_strdup (layout->name);
            list->desc = layout->description ? g_strdup (layout->description) : NULL;
            list->variants = g_new0 (InputPadXKBVariantList, 1);
            list->variants->variant = g_strdup (variant->name);
            list->variants->desc = variant->description ? g_strdup (variant->description) : NULL;
            goto end_append_layout_variant;
        }
        if (!g_strcmp0 (list->layout, layout->name)) {
            if (list->variants == NULL) {
                list->variants = g_new0 (InputPadXKBVariantList, 1);
                list->variants->variant = g_strdup (variant->name);
                list->variants->desc = variant->description ? g_strdup (variant->description) : NULL;
                goto end_append_layout_variant;
            }
            variants = list->variants;
            while (variants->next) {
                if (variants->variant == NULL) {
                    variants->variant = g_strdup (variant->name);
                    variants->desc = variant->description ? g_strdup (variant->description) : NULL;
                    goto end_append_layout_variant;
                }
                if (!g_strcmp0 (variants->variant, variant->name)) {
                    goto end_append_layout_variant;
                }
                variants = variants->next;
            }
            variants->next = g_new0 (InputPadXKBVariantList, 1);
            variants = variants->next;
            variants->variant = g_strdup (variant->name);
            variants->desc = variant->description ? g_strdup (variant->description) : NULL;
            goto end_append_layout_variant;
        }
        if (list->next == NULL) {
            break;
        }
        list = list->next;
    }
    list->next = g_new0 (InputPadXKBLayoutList, 1);
    list = list->next;
    list->layout = g_strdup (layout->name);
    list->desc = layout->description ? g_strdup (layout->description) : NULL;
    list->variants = g_new0 (InputPadXKBVariantList, 1);
    list->variants->variant = g_strdup (variant->name);
    list->variants->desc = variant->description ? g_strdup (variant->description) : NULL;
end_append_layout_variant:
    ;
}

static void
add_variant (XklConfigRegistry   *xklconfig_registry,
             const XklConfigItem *item,
             gpointer             data)
{
    XklLayoutData *layout_data = (XklLayoutData *) data;
    if (*layout_data->config_regp == NULL) {
        *layout_data->config_regp = g_new0 (InputPadXKBConfigReg, 1);
    }
    if ((*layout_data->config_regp)->layouts == NULL) {
        (*layout_data->config_regp)->layouts = g_new0 (InputPadXKBLayoutList, 1);
    }
    input_pad_xkb_layout_list_append_layout_variant ((*layout_data->config_regp)->layouts,
                                                    layout_data->layout,
                                                    item);
}

static void
add_layout (XklConfigRegistry   *xklconfig_registry,
            const XklConfigItem *item,
            gpointer             data)
{
    InputPadXKBConfigReg  **config_regp = (InputPadXKBConfigReg **) data;
    XklLayoutData layout_data;
    layout_data.layout = item;
    layout_data.config_regp = config_regp;
    xkl_config_registry_foreach_layout_variant (xklconfig_registry,
                                                item->name,
                                                add_variant,
                                                &layout_data);
    layout_data.layout = NULL;
    layout_data.config_regp = NULL;
}

static gboolean
get_config_reg_with_xkl_config_registry (InputPadXKBConfigReg  **config_regp,
                                         XklConfigRegistry *xklconfig_registry)
{
    g_return_val_if_fail (config_regp != NULL, FALSE);
    g_return_val_if_fail (xklconfig_registry != NULL, FALSE);

    xkl_config_registry_foreach_layout (xklconfig_registry, add_layout, config_regp);
    return TRUE;
}

static int
find_layouts_index (gchar **all_layouts, const gchar *sub_layouts)
{
    gchar *line, *p, *head;
    int index = 0;

    g_return_val_if_fail (all_layouts != NULL && sub_layouts != NULL, -1);

    line = g_strjoinv (",", all_layouts);
    if ((head = g_strstr_len (line, -1, sub_layouts)) == NULL) {
        g_free (line);
        return -1;
    }
    for (p = line; p < head; p++) {
      if (*p == ',') {
          index++;
      }
    }
    g_free (line);

    return index;
}

static gchar **
concat_layouts (gchar **all_layouts, const gchar *sub_layouts)
{
    gchar **sub_array;
    gchar **retval;
    const int avoid_loop = 50;
    int groups, i, n_all, n_sub;

    g_return_val_if_fail (all_layouts != NULL && sub_layouts != NULL, NULL);

    groups = MAX (xkl_engine_get_max_num_groups (xklengine), 1);
    sub_array = g_strsplit (sub_layouts, ",", -1);
    for (n_all = 0; all_layouts[n_all] && *all_layouts[n_all]; n_all++) {
        if (n_all >= avoid_loop) {
            break;
        }
    }
    for (n_sub = 0; sub_array[n_sub] && *sub_array[n_sub]; n_sub++) {
        if (n_sub >= avoid_loop) {
            break;
        }
    }
    if (n_all + n_sub > groups) {
        n_all = groups - n_sub;
        if (n_all < 1) {
            n_all = MAX (groups, 1);
        }
    }
    retval = g_new0 (char*, n_all + n_sub + 1);
    for (i = 0; i < n_all; i++) {
        retval[i] = g_strdup (all_layouts[i]);
    }
    for (i = 0; i < n_sub; i++) {
        retval[n_all + i] = g_strdup (sub_array[i]);
    }
    retval[n_all + n_sub] = NULL;
    g_strfreev (sub_array);

    return retval;
}

#else

static Bool
set_xkb_rules (Display *xdisplay,
               const char *rules_file, const char *model, 
               const char *all_layouts, const char *all_variants,
               const char *all_options)
{
    int len;
    char *pval;
    char *next;
    Atom rules_atom;
    Window root_window;

    len = (rules_file ? strlen (rules_file) : 0);
    len += (model ? strlen (model) : 0);
    len += (all_layouts ? strlen (all_layouts) : 0);
    len += (all_variants ? strlen (all_variants) : 0);
    len += (all_options ? strlen (all_options) : 0);

    if (len < 1) {
        return TRUE;
    }
    len += 5; /* trailing NULs */

    rules_atom = XInternAtom(xdisplay, _XKB_RF_NAMES_PROP_ATOM, False);
    root_window = XDefaultRootWindow (xdisplay);
    pval = next = g_new0 (char, len + 1);
    if (!pval) {
        return TRUE;
    }

    if (rules_file) {
        strcpy(next, rules_file);
        next += strlen(rules_file);
    }
    *next++ = '\0';
    if (model) {
        strcpy(next, model);
        next += strlen(model);
    }
    *next++ = '\0';
    if (all_layouts) {
        strcpy(next, all_layouts);
        next += strlen(all_layouts);
    }
    *next++ = '\0';
    if (all_variants) {
        strcpy(next, all_variants);
        next += strlen(all_variants);
    }
    *next++ = '\0';
    if (all_options) {
        strcpy(next, all_options);
        next += strlen(all_options);
    }
    *next++ = '\0';
    if ((next - pval) != len) {
        g_free(pval);
        return TRUE;
    }

    XChangeProperty (xdisplay, root_window,
                    rules_atom, XA_STRING, 8, PropModeReplace,
                    (unsigned char *) pval, len);
    XSync(xdisplay, False);

    return TRUE;
}
#endif

void
input_pad_gdk_xkb_destroy_keyboard_layouts (InputPadGtkWindow   *window,
                                            InputPadXKBKeyList  *xkb_key_list)
{
    InputPadXKBKeyList *list = xkb_key_list;
    InputPadXKBKeyRow *row;
    int i, j;

    if (xkb_key_list == NULL) {
        return;
    }

    for (i = 1; list; i++) {
        row = list->row;
        while (row) {
            g_free (row->name);
            row->name = NULL;
            for (j = 0; row->keysym && row->keysym[j]; j++) {
                g_free (row->keysym[j]);
                row->keysym[j] = NULL;
            }
            g_free (row->keysym);
            row->keysym = NULL;
            row = row->next;
        }
        list = list->next;
    }

    list = xkb_key_list;
    for (i = 1; list; i++) {
        while (1) {
            row = list->row;
            if (row == NULL) {
                break;
            }
            while (row) {
                if (row && row->next && row->next->next == NULL) {
                    break;
                } else if (row && row->next == NULL) {
                    break;
                }
                row = row->next;
            }
            if (row && row->next) {
                g_free (row->next);
                row->next = NULL;
            } else if (row) {
                g_free (list->row);
                list->row = NULL;
                break;
            }
        }
        list = list->next;
    }
    while (1) {
        list = xkb_key_list;
        if (list == NULL) {
            break;
        }
        for (i = 1; list; i++) {
            if (list && list->next && list->next->next == NULL) {
                break;
            } else if (list && list->next == NULL) {
                break;
            }
            list = list->next;
        }
        if (list && list->next) {
            g_free (list->next);
            list->next = NULL;
        } else if (list) {
            g_free (xkb_key_list);
            xkb_key_list = NULL;
            break;
        }
    }
}

InputPadXKBKeyList *
input_pad_gdk_xkb_parse_keyboard_layouts (InputPadGtkWindow   *window)
{
    XkbFileInfo *xkb_info;
    XkbDrawablePtr draw, draw_head;
    InputPadXKBKeyList *xkb_key_list = NULL;
    
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
    debug_print_key_list (xkb_key_list);

    if (xkb_key_list) {
        xkb_key_list->priv = g_new0 (InputPadXKBKeyListPrivate, 1);
        xkb_key_list->priv->xkb_info = xkb_info;
    }

#ifdef HAVE_LIBXKLAVIER
    if (xklengine == NULL) {
        xklengine = init_xkl_engine (window, &initial_xkl_rec);
    }
#endif

    return xkb_key_list;
}

void
input_pad_gdk_xkb_signal_emit (InputPadGtkWindow   *window, guint signal_id)
{
    g_return_if_fail (window != NULL && INPUT_PAD_IS_GTK_WINDOW (window));

    xkb_setup_events (window, signal_id);
}

char **
input_pad_gdk_xkb_get_group_layouts (InputPadGtkWindow   *window, 
                                     InputPadXKBKeyList  *xkb_key_list)
{
    Display *xdisplay;
    char **names;
    Atom xkb_rules_name, type;
    int format;
    unsigned long l, nitems, bytes_after;
    unsigned char *prop = NULL;

    g_return_val_if_fail (window != NULL && INPUT_PAD_IS_GTK_WINDOW (window),
                          NULL);

    xdisplay = GDK_WINDOW_XDISPLAY (GTK_WIDGET(window)->window);
    xkb_rules_name = XInternAtom (xdisplay, "_XKB_RULES_NAMES", TRUE);
    if (xkb_rules_name == None) {
        g_warning ("Could not get XKB rules atom");
        return NULL;
    }
    if (XGetWindowProperty (xdisplay,
                            XDefaultRootWindow (xdisplay),
                            xkb_rules_name,
                            0, 1024, FALSE, XA_STRING,
                            &type, &format, &nitems, &bytes_after, &prop) != Success) {
        g_warning ("Could not get X property");
        return NULL;
    }
    if (nitems < 3) {
        g_warning ("Could not get group layout from X property");
        return NULL;
    }
    for (l = 0; l < 2; l++) {
        prop += strlen ((const char *) prop) + 1;
    }
    if (prop == NULL || *prop == '\0') {
        g_warning ("No layouts form X property");
        return NULL;
    }
    names = g_strsplit ((gchar *) prop, ",", -1);
    debug_print_group_layout_list (names);

    return names;
}

InputPadXKBConfigReg *
input_pad_gdk_xkb_parse_config_registry (InputPadGtkWindow   *window,
                                         InputPadXKBKeyList  *xkb_key_list)
{
#ifdef HAVE_LIBXKLAVIER
    InputPadXKBConfigReg  *config_reg = NULL;
    XklConfigRegistry *xklconfig_registry;

    g_return_val_if_fail (window != NULL && INPUT_PAD_IS_GTK_WINDOW (window), NULL);

    if (xklengine == NULL) {
        xklengine = init_xkl_engine (window, &initial_xkl_rec);
    }
    xklconfig_registry = init_xkl_config_registry (window);
    get_config_reg_with_xkl_config_registry (&config_reg,
                                             xklconfig_registry);

    debug_print_layout_list (config_reg->layouts);
    return config_reg;
#else
    return NULL;
#endif
}

Bool
input_pad_gdk_xkb_set_layout (InputPadGtkWindow        *window,
                              InputPadXKBKeyList       *xkb_key_list,
                              const char               *layouts,
                              const char               *variants,
                              const char               *options)
{
#ifdef HAVE_LIBXKLAVIER
    XklConfigRec *xkl_rec;
    XklState *state;
    int layout_index = -1;

    g_return_val_if_fail (layouts != NULL, FALSE);
    g_return_val_if_fail (initial_xkl_rec != NULL, FALSE);
    g_return_val_if_fail (xklengine != NULL, FALSE);

    xkl_rec = xkl_config_rec_new ();
    xkl_rec->model =  initial_xkl_rec->model ?
                      g_strdup (initial_xkl_rec->model) :
                      g_strdup ("pc105");
    if (initial_xkl_rec->layouts == NULL) {
        xkl_rec->layouts = g_strsplit (layouts, ",", -1);
    } else {
        layout_index = find_layouts_index (initial_xkl_rec->layouts, layouts);
        if (layout_index >= 0) {
            state = xkl_engine_get_current_state (xklengine);
            if (state->group == layout_index) {
                g_free (xkl_rec->model);
                xkl_rec->model = NULL;
                g_object_unref (xkl_rec);
                return TRUE;
            }
            xkl_rec->layouts = g_strdupv (initial_xkl_rec->layouts);
        } else {
            xkl_rec->layouts = concat_layouts (initial_xkl_rec->layouts,
                                               layouts);
            layout_index = find_layouts_index (xkl_rec->layouts, layouts);
        }
    }
    if (variants == NULL) {
        xkl_rec->variants = g_strdupv (initial_xkl_rec->variants);
    } else if (initial_xkl_rec->variants == NULL) {
        xkl_rec->variants = variants ? g_strsplit (variants, ",", -1) : NULL;
    } else if (find_layouts_index (initial_xkl_rec->variants, variants) >= 0) {
        xkl_rec->variants = g_strdupv (initial_xkl_rec->variants);
    } else {
        xkl_rec->variants = concat_layouts (initial_xkl_rec->variants,
                                            variants);
    }
    if (options == NULL) {
        xkl_rec->options = g_strdupv (initial_xkl_rec->options);
    } else if (initial_xkl_rec->options== NULL) {
        xkl_rec->options = options ? g_strsplit (options , ",", -1) : NULL;
    } else if (find_layouts_index (initial_xkl_rec->options, options) >= 0) {
        xkl_rec->options = g_strdupv (initial_xkl_rec->options);
    } else {
        xkl_rec->options = concat_layouts (initial_xkl_rec->options,
                                           options);
    }

    xkl_config_rec_activate (xkl_rec, xklengine);
    g_object_unref (xkl_rec);
    if (layout_index >= 0) {
        xkl_engine_lock_group (xklengine, layout_index);
    } else {
        xkl_engine_lock_group (xklengine, 0);
    }
#else
    Display *xdisplay;
    guint group;
    gchar **group_layouts;

    g_return_val_if_fail (window != NULL &&
                          INPUT_PAD_IS_GTK_WINDOW (window), NULL);
    g_return_val_if_fail (layouts != NULL, FALSE);

    xdisplay = GDK_WINDOW_XDISPLAY (GTK_WIDGET(window)->window);
    group = xkb_get_current_group (window);
    group_layouts = input_pad_gdk_xkb_get_group_layouts (window, 
                                                         xkb_key_list);
    if (group_layouts != NULL) {
        if (!g_strcmp0 (group_layouts[group], layouts)) {
            g_strfreev (group_layouts);
            return TRUE;
        }
        g_strfreev (group_layouts);
    }
    set_xkb_rules (xdisplay, "evdev", "evdev", layouts, variants, options);
#endif

    return TRUE;
}
