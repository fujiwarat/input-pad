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
#include <gdk/gdkkeys.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <string.h> /* strlen */
#include <stdlib.h> /* exit */

#include "i18n.h"
#include "button-gtk.h"
#include "combobox-gtk.h"
#include "geometry-gdk.h"
#include "input-pad.h"
#include "input-pad-group.h"
#include "input-pad-window-gtk.h"
#include "input-pad-private.h"
#include "input-pad-marshal.h"
#include "unicode_block.h"

#define N_KEYBOARD_LAYOUT_PART 3
#define INPUT_PAD_UI_FILE INPUT_PAD_UI_GTK_DIR "/input-pad.ui"

#define INPUT_PAD_GTK_WINDOW_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), INPUT_PAD_TYPE_GTK_WINDOW, InputPadGtkWindowPrivate))

typedef struct _CodePointData CodePointData;
typedef struct _KeyboardLayoutPart KeyboardLayoutPart;
typedef struct _CharTreeViewData CharTreeViewData;

enum {
    BUTTON_PRESSED,
    KBD_CHANGED,
    LAST_SIGNAL
};

enum {
    DIGIT_TEXT_COL = 0,
    DIGIT_VISIBLE_COL,
    DIGIT_N_COLS,
};

enum {
    LAYOUT_LAYOUT_NAME_COL = 0,
    LAYOUT_LAYOUT_DESC_COL,
    LAYOUT_VARIANT_NAME_COL,
    LAYOUT_VARIANT_DESC_COL,
    LAYOUT_VISIBLE_COL,
    LAYOUT_N_COLS,
};

enum {
    CHAR_BLOCK_LABEL_COL = 0,
    CHAR_BLOCK_UNICODE_COL,
    CHAR_BLOCK_UTF8_COL,
    CHAR_BLOCK_START_COL,
    CHAR_BLOCK_END_COL,
    CHAR_BLOCK_VISIBLE_COL,
    CHAR_BLOCK_N_COLS,
};

struct _InputPadGtkWindowPrivate {
    InputPadGroup              *group;
    guint                       show_all : 1;
    GModule                    *module_gdk_xtest;
    InputPadXKBKeyList         *xkb_key_list;
    guint                       keyboard_state;
    InputPadXKBConfigReg       *xkb_config_reg;
    gchar                     **group_layouts;
};

struct _CodePointData {
    GtkWidget                  *signal_window;
    GtkWidget                  *digit_hbox;
    GtkWidget                  *char_label;
};

struct _KeyboardLayoutPart {
    int                         key_row_id;
    int                         row;
    int                         col;
    GtkWidget                  *table;
};

struct _CharTreeViewData {
    GtkWidget                  *viewport;
    GtkWidget                  *window;
};

static guint                    signals[LAST_SIGNAL] = { 0 };
static gboolean                 use_module_xtest = FALSE;
static gboolean                 ask_version = FALSE;
#if 0
static GtkBuildableIface       *parent_buildable_iface;
#endif

static GOptionEntry entries[] = {
#ifdef MODULE_XTEST_GDK_BASE
  { "enable-module-xtest", 'x', 0, G_OPTION_ARG_NONE, &use_module_xtest,
    N_("Use XTEST module to send key events"), NULL},
  { "version", 'v', 0, G_OPTION_ARG_NONE, &ask_version,
    N_("Display version"), NULL},
#endif
  { NULL }
};

static void xor_modifiers (InputPadGtkWindow *window, guint modifiers);
static guint digit_hbox_get_code_point (GtkWidget *digit_hbox);
static void char_label_set_code_point (GtkWidget *char_label, guint code);
static void set_code_point_base (CodePointData *cp_data, int n_encoding);
static InputPadGroup *get_nth_pad_group (InputPadGroup *group, int nth);
static InputPadTable *get_nth_pad_table (InputPadTable *table, int nth);
static void create_char_table (GtkWidget *vbox, InputPadTable *table_data);
static char *get_keysym_display_name (guint keysym, GtkWidget *widget, gchar **tooltipp);
static void create_keyboard_layout_ui_real (GtkWidget *vbox, InputPadGtkWindow *window);
static void destroy_prev_keyboard_layout (GtkWidget *vbox, InputPadGtkWindow *window);
static void create_keyboard_layout_list_ui_real (GtkWidget *vbox, InputPadGtkWindow *window);
static void destroy_char_view_table (GtkWidget *viewport);
static void append_char_view_table (GtkWidget *viewport, unsigned int start, unsigned int end, GtkWidget *window);
static void input_pad_gtk_window_real_destroy (GtkObject *object);
static void input_pad_gtk_window_buildable_interface_init (GtkBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (InputPadGtkWindow, input_pad_gtk_window,
                         GTK_TYPE_WINDOW,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE,
                                                input_pad_gtk_window_buildable_interface_init))

static void
on_window_close (InputPadGtkWindow *window, gpointer data)
{
    g_return_if_fail (window != NULL &&
                      INPUT_PAD_IS_GTK_WINDOW (window));

    if (window->child) {
        gtk_widget_hide (GTK_WIDGET (window));
    } else {
        gtk_main_quit ();
    }
}

static void
on_window_keyboard_changed (InputPadGtkWindow *window,
                            gint               group,
                            gpointer           data)
{
    InputPadGtkButton *button;
    guint **keysyms;
    int i = 0;
    gchar *tooltip, *display_name;

    g_return_if_fail (window != NULL &&
                      INPUT_PAD_IS_GTK_WINDOW (window));
    g_return_if_fail (data != NULL &&
                      INPUT_PAD_IS_GTK_BUTTON (data));

    button = INPUT_PAD_GTK_BUTTON (data);
    keysyms = input_pad_gtk_button_get_all_keysyms (button);
    g_return_if_fail (keysyms != NULL);
    while (keysyms[i]) i++;
    if (group >= i) {
        return;
    }

    input_pad_gtk_button_set_keysym_group (button, group);
    input_pad_gtk_button_set_keysym (button, keysyms[group][0]);
    display_name = get_keysym_display_name (keysyms[group][0],
                                            GTK_WIDGET (window), &tooltip);
    gtk_button_set_label (GTK_BUTTON (button), display_name);
    gtk_widget_set_tooltip_text (GTK_WIDGET (button), tooltip);
    g_free (display_name);
}

static void
on_window_keyboard_changed_combobox (InputPadGtkWindow *window,
                                     gint               group,
                                     gpointer           data)
{
    GtkTreeModel *model;
    GtkTreeIter   active_iter;
    GtkTreeIter   iter;
    gchar **group_layouts;
    gchar *active_layout = NULL;
    gchar *layout;
    gchar *desc;

    g_return_if_fail (window != NULL &&
                      INPUT_PAD_IS_GTK_WINDOW (window));

    if ((group_layouts = window->priv->group_layouts) == NULL) {
        return;
    }

    g_return_if_fail (data != NULL &&
                      GTK_IS_COMBO_BOX (data));

    model = gtk_combo_box_get_model (GTK_COMBO_BOX (data));
    if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (data), &active_iter)) {
        gtk_tree_model_get (model, &active_iter,
                            LAYOUT_LAYOUT_NAME_COL, &active_layout, -1);
    }
    if (!gtk_tree_model_get_iter_first (model, &iter)) {
        g_warning ("Could not get the first iter in layout combo box");
        g_free (active_layout);
        return;
    }
    do {
        gtk_tree_model_get (model, &iter,
                            LAYOUT_LAYOUT_NAME_COL, &layout,
                            LAYOUT_LAYOUT_DESC_COL, &desc, -1);
        if (!g_strcmp0 (group_layouts[group], layout) &&
            (g_strcmp0 (active_layout, layout) != 0)) {
            gtk_combo_box_set_active_iter (GTK_COMBO_BOX (data), &iter);
            g_free (layout);
            break;
        }
        g_free (layout);
    } while (gtk_tree_model_iter_next (model, &iter));
    g_free (active_layout);
}

static void
on_item_dialog_activate (GtkAction *action, gpointer data)
{
    GtkWidget *dlg = GTK_WIDGET (data);

    gtk_dialog_run (GTK_DIALOG (dlg));
    gtk_widget_hide (dlg);
}

static void
on_button_encoding_clicked (GtkButton *button, gpointer data)
{
    gboolean active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
    const gchar *name;

    if (!active)
        return;

    name = gtk_buildable_get_name (GTK_BUILDABLE (button));
    if (name == NULL) {
        name = (const gchar *) g_object_get_data (G_OBJECT (button),
                                                  "gtk-builder-name");
    }
    g_return_if_fail (name != NULL);
    g_return_if_fail (g_str_has_prefix (name, "Encoding"));
    g_debug ("test %s %d\n", name ? name : "(null)", active);
}

static void
on_button_base_clicked (GtkButton *button, gpointer data)
{
    CodePointData *cp_data;
    gboolean active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
    const gchar *name;
    int base;

    if (!active)
        return;

    cp_data = (CodePointData *) data;
    name = gtk_buildable_get_name (GTK_BUILDABLE (button));
    if (name == NULL) {
        name = (const gchar *) g_object_get_data (G_OBJECT (button),
                                                  "gtk-builder-name");
    }
    g_return_if_fail (name != NULL);
    g_return_if_fail (g_str_has_prefix (name, "Base"));
    base = (int) g_ascii_strtoll (name + strlen ("Base"), NULL, 10);

    set_code_point_base (cp_data, base);
}

static void
on_button_shift_clicked (GtkButton *button, gpointer data)
{
    InputPadGtkButton *gen_button;
    guint current_keysym;
    guint new_keysym = 0;
    int i, group;
    guint **keysyms;
    gchar *tooltip;
    gchar *display_name;

    g_return_if_fail (INPUT_PAD_IS_GTK_BUTTON (data));
    gen_button = INPUT_PAD_GTK_BUTTON (data);
    current_keysym = input_pad_gtk_button_get_keysym (gen_button);
    group = input_pad_gtk_button_get_keysym_group (gen_button);
    keysyms = input_pad_gtk_button_get_all_keysyms (gen_button);
    if (keysyms == NULL || keysyms[group] == NULL) {
        return;
    }
    if (current_keysym == 0 ||
        current_keysym == XK_Shift_L || current_keysym == XK_Shift_R) {
        return;
    }
    for (i = 0; keysyms[group][i] && (i < 20); i++) {
        if (keysyms[group][i] == current_keysym) {
            break;
        }
    }
    if (i == 0 && keysyms[group][i] && keysyms[group][i + 1]) {
        new_keysym = keysyms[group][i + 1];
    } else {
        new_keysym = keysyms[group][0];
    }
    if (new_keysym) {
        display_name = get_keysym_display_name (new_keysym,
                                                GTK_WIDGET (button),
                                                &tooltip);
        input_pad_gtk_button_set_keysym (gen_button, new_keysym);
        gtk_button_set_label (GTK_BUTTON (gen_button), display_name);
        g_free (display_name);
        gtk_widget_set_tooltip_text (GTK_WIDGET (gen_button), tooltip);
    }
}

static void
on_button_ctrl_clicked (GtkButton *button, gpointer data)
{
    InputPadGtkWindow *window;

    g_return_if_fail (INPUT_PAD_IS_GTK_WINDOW (data));
    window = INPUT_PAD_GTK_WINDOW (data);
    xor_modifiers (window, ControlMask);
}

static void
on_button_alt_clicked (GtkButton *button, gpointer data)
{
    InputPadGtkWindow *window;

    g_return_if_fail (INPUT_PAD_IS_GTK_WINDOW (data));
    window = INPUT_PAD_GTK_WINDOW (data);
    xor_modifiers (window, Mod1Mask);
}

static void
on_button_ok_clicked (GtkButton *button, gpointer data)
{
    GtkWidget *contents_dlg = GTK_WIDGET (data);

    gtk_widget_hide (contents_dlg);
}

static void
on_close_activate (GtkAction *quit, gpointer data)
{
    g_return_if_fail (data != NULL &&
                      INPUT_PAD_IS_GTK_WINDOW (data));

    on_window_close (INPUT_PAD_GTK_WINDOW (data), NULL);
}

static void
on_combobox_layout_changed (GtkComboBox *combobox,
                            gpointer     data)
{
    InputPadGtkWindow *window;
    GtkTreeIter iter;
    GtkTreeModel *model;
    gchar *layout;
    GtkWidget *keyboard_vbox;

    g_return_if_fail (data != NULL &&
                      INPUT_PAD_IS_GTK_WINDOW (data));

    window = INPUT_PAD_GTK_WINDOW (data);
    if (!gtk_combo_box_get_active_iter (combobox, &iter)) {
        return;
    }
    model = gtk_combo_box_get_model (combobox);
    gtk_tree_model_get (model, &iter,
                        LAYOUT_LAYOUT_NAME_COL, &layout, -1);
    input_pad_gdk_xkb_set_layout (window, window->priv->xkb_key_list,
                                  layout, NULL, NULL);
    g_free (layout);

    if (window->priv->xkb_key_list) {
        input_pad_gdk_xkb_destroy_keyboard_layouts (window,
                                                    window->priv->xkb_key_list);
        window->priv->xkb_key_list = NULL;
    }
    window->priv->xkb_key_list =
        input_pad_gdk_xkb_parse_keyboard_layouts (window);
    if (window->priv->xkb_key_list == NULL) {
        return;
    }

    keyboard_vbox = gtk_widget_get_parent (gtk_widget_get_parent (GTK_WIDGET (combobox)));
    destroy_prev_keyboard_layout (keyboard_vbox, window);
    create_keyboard_layout_ui_real (keyboard_vbox, window);

    if (window->priv->group_layouts) {
        g_strfreev (window->priv->group_layouts);
        window->priv->group_layouts = NULL;
    }
    window->priv->group_layouts =
        input_pad_gdk_xkb_get_group_layouts (window,
                                             window->priv->xkb_key_list);
    input_pad_gdk_xkb_signal_emit (window, signals[KBD_CHANGED]);
}

static void
on_main_switch_page (GtkNotebook       *notebook,
                     GtkNotebookPage   *page,
                     guint              page_num,
                     gpointer           data)
{
    GtkWidget *sub_notebook;
    gint sub_page;
    GtkWidget *vbox;
    InputPadGroup *group = (InputPadGroup *) data;
    InputPadTable *table;

    sub_notebook = gtk_notebook_get_nth_page (notebook, (int) page_num);
    sub_page = gtk_notebook_get_current_page (GTK_NOTEBOOK (sub_notebook));
    vbox = gtk_notebook_get_nth_page (GTK_NOTEBOOK (sub_notebook), sub_page);
    group = get_nth_pad_group (group, (int) page_num);
    table = get_nth_pad_table (group->table, sub_page);

    g_return_if_fail (table != NULL);
    create_char_table (vbox, table);
}

static void
on_sub_switch_page (GtkNotebook        *notebook,
                    GtkNotebookPage    *page,
                    guint               page_num,
                    gpointer            data)
{
    GtkWidget *vbox;
    InputPadTable *table = (InputPadTable *) data;

    vbox = gtk_notebook_get_nth_page (notebook, (int) page_num);
    table = get_nth_pad_table (table, (int) page_num);

    g_return_if_fail (table != NULL);
    create_char_table (vbox, table);
}

static void
on_toggle_action (GtkToggleAction *action, gpointer data)
{
    GtkWidget *widget;

    g_return_if_fail (data != NULL && GTK_IS_WIDGET (data));
    widget = GTK_WIDGET (data);
    if (gtk_toggle_action_get_active (action)) {
        gtk_widget_show (widget);
    } else {
        gtk_widget_hide (widget);
    }
}

static void
xor_modifiers (InputPadGtkWindow *window, guint modifiers)
{
    guint state = window->priv->keyboard_state;

    if (state & modifiers) {
        state ^= modifiers;
    } else {
        state |= modifiers;
    }
    window->priv->keyboard_state = state;
}

/*
 * % xterm -xrm "XTerm*allowSendEvents: true"
 */
static int
send_key_event (GdkWindow      *gdkwindow,
                guint           keysym,
                guint           keycode,
                guint           state)
{
    XEvent xevent;
    Window input_focus;
    int revert;

    XGetInputFocus (GDK_WINDOW_XDISPLAY (gdkwindow), &input_focus, &revert);
    if (input_focus == (Window) 0) {
        g_warning ("WARNING: Could not get a focused window.");
        return FALSE;
    }

    xevent.type = KeyPress;
    xevent.xkey.type = KeyPress;
    xevent.xkey.serial = 0L;
    xevent.xkey.send_event = True;
    xevent.xkey.display = GDK_WINDOW_XDISPLAY (gdkwindow);
    xevent.xkey.root = XDefaultRootWindow (xevent.xkey.display);
    xevent.xkey.window = input_focus;
    xevent.xkey.subwindow = xevent.xkey.window;
    xevent.xkey.time = CurrentTime;
    xevent.xkey.state = state;
    if (keycode) {
        xevent.xkey.keycode = keycode;
    } else {
        xevent.xkey.keycode = XKeysymToKeycode (xevent.xkey.display,
                                                (KeySym) keysym);
    }
    xevent.xkey.same_screen = True;
    XSendEvent (xevent.xkey.display, xevent.xkey.window, True,
                KeyPressMask, &xevent);

    XSync (xevent.xkey.display, False);
    xevent.type = KeyRelease;
    xevent.xkey.type = KeyRelease;
    XSendEvent (xevent.xkey.display, xevent.xkey.window, True,
                KeyReleaseMask, &xevent);
    XSync (xevent.xkey.display, False);

    return TRUE;
}

#if 0
/* deprecated */
static guint
get_state (const char *str, guint keysym)
{
    guint state = 0;

    if (!str || !*str || keysym > 0x7f) {
        return 0;
    }
    if (*str >= 'A' && *str <= 'Z') {
        return ShiftMask;
    }
    switch (*str) {
    case '!':
    case '\"':
    case '#':
    case '$':
    case '%':
    case '&':
    case '\'':
    case '(':
    case ')':
    case '~':
    case '=':
    case '|':
    case '{':
    case '}':
    case '`':
    case '+':
    case '*':
    case '?':
    case '_':
        state = ShiftMask;
        break;
    default:;
    }
    return state;
}
#endif

static void
on_button_send_clicked (GtkButton *button, gpointer data)
{
    CodePointData *cp_data;
    const gchar *str;
    guint code;
    guint state = 0;
    gboolean retval = FALSE;
    InputPadTableType  type;

    cp_data = (CodePointData *) data;
    g_return_if_fail (GTK_IS_LABEL (cp_data->char_label));
    g_return_if_fail (GTK_IS_CONTAINER (cp_data->digit_hbox));
    g_return_if_fail (GTK_IS_WIDGET (cp_data->signal_window));

    str = gtk_label_get_label (GTK_LABEL (cp_data->char_label));
    /* Decided input-pad always sends char but not keysym.
     * Now keyboard layout can be used instead. */
#if 0
    code = digit_hbox_get_code_point (cp_data->digit_hbox);
    state = get_state (str, code);
    if (code < 0x7f) {
        type = INPUT_PAD_TABLE_TYPE_KEYSYMS;
    } else {
        type = INPUT_PAD_TABLE_TYPE_CHARS;
        code = 0;
    }
#else
    type = INPUT_PAD_TABLE_TYPE_CHARS;
    code = 0;
    state = 0;
#endif

    g_signal_emit (cp_data->signal_window, signals[BUTTON_PRESSED], 0,
                   str, type, code, 0, state, &retval);
}

static void
on_button_pressed (GtkButton *button, gpointer data)
{
    InputPadGtkWindow *window;
    InputPadGtkButton *ibutton;
    InputPadTableType  type;
    const char *str = gtk_button_get_label (button);
    guint keycode;
    guint keysym;
    guint **keysyms;
    guint group;
    guint state = 0;
    gboolean retval = FALSE;

    g_return_if_fail (INPUT_PAD_IS_GTK_BUTTON (button));
    g_return_if_fail (data != NULL &&
                      INPUT_PAD_IS_GTK_WINDOW (data));

    window = INPUT_PAD_GTK_WINDOW (data);
    ibutton = INPUT_PAD_GTK_BUTTON (button);
    type = input_pad_gtk_button_get_table_type (ibutton);
    keycode  = input_pad_gtk_button_get_keycode (ibutton);
    keysym = input_pad_gtk_button_get_keysym (ibutton);
    keysyms = input_pad_gtk_button_get_all_keysyms (ibutton);
    group = input_pad_gtk_button_get_keysym_group (ibutton);
    state = window->priv->keyboard_state;
    if (keysyms && (keysym != keysyms[group][0])) {
        state |= ShiftMask;
    }
    state = input_pad_xkb_build_core_state (state, group);

    g_signal_emit (window, signals[BUTTON_PRESSED], 0,
                   str, type, keysym, keycode, state, &retval);

    if (state & ShiftMask) {
        state ^= ShiftMask;
    }
    if ((state & ControlMask) && 
        (keysym != XK_Control_L) && (keysym != XK_Control_R)) {
        state ^= ControlMask;
    }
    if ((state & Mod1Mask) && 
        (keysym != XK_Alt_L) && (keysym != XK_Alt_R)) {
        state ^= Mod1Mask;
    }
    window->priv->keyboard_state = state;
}

static void
on_button_layout_arrow_pressed (GtkButton *button, gpointer data)
{
    KeyboardLayoutPart *table_data = (KeyboardLayoutPart *) data;
    int i;
    const gchar *prev_label;
    gboolean extend = FALSE;

    prev_label = gtk_button_get_label (button);
    if (!g_strcmp0 (prev_label, "->")) {
        extend = TRUE;
    } else {
        extend = FALSE;
    }
    for (i = 1; i < N_KEYBOARD_LAYOUT_PART; i++) {
        if (extend) {
            gtk_widget_show (table_data[i].table);
            gtk_button_set_label (button, "<-");
            gtk_widget_set_tooltip_text (GTK_WIDGET (button),
                                         _("Fold layout"));
        } else {
            gtk_widget_hide (table_data[i].table);
            gtk_button_set_label (button, "->");
            gtk_widget_set_tooltip_text (GTK_WIDGET (button),
                                         _("Extend layout"));
        }
    }
}

static void
on_combobox_changed (GtkComboBox *combobox, gpointer data)
{
    CodePointData *cp_data;
    guint code;

    g_return_if_fail (GTK_IS_COMBO_BOX (combobox));
    g_return_if_fail (data != NULL);

    cp_data = (CodePointData *) data;
    g_return_if_fail (GTK_IS_CONTAINER (cp_data->digit_hbox));
    g_return_if_fail (GTK_IS_LABEL (cp_data->char_label));

    code = digit_hbox_get_code_point (cp_data->digit_hbox);
    char_label_set_code_point (cp_data->char_label, code);
}

static void
on_tree_view_select_char_block (GtkTreeSelection     *selection,
                                gpointer              data)
{
    CharTreeViewData *tv_data = (CharTreeViewData*) data;
    GtkWidget *viewport;
    GtkWidget *window;
    GtkTreeModel *model;
    GtkTreeIter iter;
    unsigned int start, end;

    g_return_if_fail (data != NULL);

    viewport = GTK_WIDGET (tv_data->viewport);
    window = GTK_WIDGET (tv_data->window);
    if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
        return;
    }
    gtk_tree_model_get (model, &iter,
                        CHAR_BLOCK_START_COL, &start,
                        CHAR_BLOCK_END_COL, &end, -1);
    destroy_char_view_table (viewport);
    append_char_view_table (viewport, start, end, window);
}


static void
on_window_realize (GtkWidget *window, gpointer data)
{
    GtkWidget *keyboard_vbox;
    InputPadGtkWindow *input_pad;

    g_return_if_fail (INPUT_PAD_IS_GTK_WINDOW (window));
    g_return_if_fail (GTK_IS_WIDGET (data));

    input_pad = INPUT_PAD_GTK_WINDOW (window);
    keyboard_vbox = GTK_WIDGET (data);

    input_pad->priv->xkb_key_list = 
        input_pad_gdk_xkb_parse_keyboard_layouts (input_pad);
    if (input_pad->priv->xkb_key_list == NULL) {
        return;
    }

    create_keyboard_layout_ui_real (keyboard_vbox, input_pad);
    input_pad->priv->group_layouts =
        input_pad_gdk_xkb_get_group_layouts (input_pad,
                                             input_pad->priv->xkb_key_list);
    input_pad->priv->xkb_config_reg =
        input_pad_gdk_xkb_parse_config_registry (input_pad,
                                                 input_pad->priv->xkb_key_list);
    if (input_pad->priv->xkb_config_reg == NULL) {
        input_pad_gdk_xkb_signal_emit (input_pad, signals[KBD_CHANGED]);
        return;
    }

    create_keyboard_layout_list_ui_real (keyboard_vbox, input_pad);
    input_pad_gdk_xkb_signal_emit (input_pad, signals[KBD_CHANGED]);
}

static InputPadGroup *
get_nth_pad_group (InputPadGroup *group, int nth)
{
    InputPadGroup *retval = group;
    int i;
    for (i = 0; i < nth; i++) {
        retval = retval->next;
    }
    return retval;
}

static InputPadTable *
get_nth_pad_table (InputPadTable *table, int nth)
{
    InputPadTable *retval = table;
    int i;
    for (i = 0; i < nth; i++) {
        retval = retval->next;
    }
    return retval;
}

static guint
digit_hbox_get_code_point (GtkWidget *digit_hbox)
{
    GList *list;
    GtkComboBox *combobox;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GString *string = NULL;
    gchar *line = NULL;
    guint base = 0;
    guint code;

    list = gtk_container_get_children (GTK_CONTAINER (digit_hbox));
    string = g_string_new (NULL);

    while (list) {
        g_return_val_if_fail (GTK_IS_COMBO_BOX (list->data), 0);
        combobox = GTK_COMBO_BOX (list->data);

        if (!gtk_widget_get_visible (GTK_WIDGET (combobox))) {
            break;
        }
        base = input_pad_gtk_combo_box_get_base (INPUT_PAD_GTK_COMBO_BOX (combobox));
        model = gtk_combo_box_get_model (combobox);
        if (!gtk_combo_box_get_active_iter (combobox, &iter)) {
            g_warning ("Could not find active iter");
            line = g_strdup ("0");
        } else {
            gtk_tree_model_get (model, &iter, DIGIT_TEXT_COL, &line, -1);
        }
        g_string_append (string, line);
        g_free (line);
        list = list->next;
    }
    line = g_string_free (string, FALSE);
    g_return_val_if_fail (line != NULL && base > 0, 0);

    code = (guint) g_ascii_strtoll (line, NULL, base);
    g_free (line);
    return code;
}

static void
digit_hbox_set_code_point (GtkWidget *digit_hbox, guint code)
{
    GList *list, *orig_list;
    GtkComboBox *combobox;
    GtkTreeModel *model;
    GtkTreeIter iter;
    int i, j, line_digit, n_digit = 0;
    guint base = 0;
    gchar *line = NULL;
    gchar buff[2];

    orig_list = list = gtk_container_get_children (GTK_CONTAINER (digit_hbox));

    while (list) {
        g_return_if_fail (GTK_IS_COMBO_BOX (list->data));
        combobox = GTK_COMBO_BOX (list->data);

        if (!gtk_widget_get_visible (GTK_WIDGET (combobox))) {
            break;
        }
        base = input_pad_gtk_combo_box_get_base (INPUT_PAD_GTK_COMBO_BOX (combobox));
        n_digit++;
        list = list->next;
    }

    switch (base) {
    case 16:
        line = g_strdup_printf ("%0*x", n_digit, code);
        break;
    case 10:
        line = g_strdup_printf ("%0*u", n_digit, code);
        break;
    case 8:
        line = g_strdup_printf ("%0*o", n_digit, code);
        break;
    case 2:
        line = g_strdup_printf ("%0*x", n_digit, 0);
        for (i = 0; i < n_digit / 4; i++) {
            for (j = 0; j < 4; j++) {
                line[n_digit - 1 - (i * 4) - j] = ((code >> ((i * 4) + j)) & 0x1) + '0';
            }
        }
        break;
    default:
        g_warning ("Base %d is not supported", base);
        return;
    }

    list = orig_list;
    for (i = 0; i < n_digit && list; i++) {
        combobox = GTK_COMBO_BOX (list->data);

        if (!gtk_widget_get_visible (GTK_WIDGET (combobox))) {
            break;
        }
        model = gtk_combo_box_get_model (combobox);
        if (!gtk_tree_model_get_iter_first (model, &iter)) {
            g_warning ("Could not find first iter");
            goto end_set_code_point;
        }
        buff[0] = *(line + i);
        buff[1] = '\0';
        line_digit = (int) g_ascii_strtoll (buff, NULL, 16);
        for (j = 0; j < line_digit; j++) {
            if (!gtk_tree_model_iter_next (model, &iter)) {
                g_warning ("Could not find %dth iter", j);
                goto end_set_code_point;
            }
        }
        gtk_combo_box_set_active_iter (combobox, &iter);
        list = list->next;
    }
end_set_code_point:
    g_free (line);
}

static void
char_label_set_code_point (GtkWidget *char_label, guint code)
{
    gunichar ch = (gunichar) code;
    gchar buff[7];
    if (!g_unichar_validate (ch)) {
        g_warning ("Invalid code point %x\n", code);
        return;
    }
    buff[g_unichar_to_utf8 (ch, buff)] = '\0';
    gtk_label_set_text (GTK_LABEL (char_label), buff);
}

static GtkTreeModel *
digit_model_new ()
{
    GtkTreeStore *store;
    GtkTreeIter   iter;
    int i = 0;
    gchar *str = NULL;

    store = gtk_tree_store_new (DIGIT_N_COLS, G_TYPE_STRING, G_TYPE_BOOLEAN);
    for (i = 0; i < 16; i ++) {
        str = g_strdup_printf ("%x", i);
        gtk_tree_store_append (store, &iter, NULL);
        gtk_tree_store_set (store, &iter, DIGIT_TEXT_COL, str,
                            DIGIT_VISIBLE_COL, TRUE, -1);
    }
    return GTK_TREE_MODEL (store);
}

static void
set_code_point_base (CodePointData *cp_data, int n_encoding)
{
    int i, n_digit, created_num;
    char *formatted = NULL;
    const guint max_ucode = 0x10ffff;
    guint code = 0;
    GList *list;
    GtkWidget *digit_hbox;
    GtkWidget *char_label;
    GtkWidget *combobox;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkCellRenderer *renderer;

    g_return_if_fail (cp_data != NULL);
    g_return_if_fail (GTK_IS_CONTAINER (cp_data->digit_hbox));
    digit_hbox = GTK_WIDGET (cp_data->digit_hbox);
    g_return_if_fail (GTK_IS_LABEL (cp_data->char_label));
    char_label = GTK_WIDGET (cp_data->char_label);

    switch (n_encoding) {
    case 16:
        formatted = g_strdup_printf ("%x", max_ucode);
        n_digit = strlen (formatted);
        break;
    case 10:
        formatted = g_strdup_printf ("%u", max_ucode);
        n_digit = strlen (formatted);
        break;
    case 8:
        formatted = g_strdup_printf ("%o", max_ucode);
        n_digit = strlen (formatted);
        break;
    case 2:
        formatted = g_strdup_printf ("%x", max_ucode);
        n_digit = strlen (formatted) * 4;
        break;
    default:
        g_warning ("Base %d is not supported", n_encoding);
        return;
    }
    g_free (formatted);

    list = gtk_container_get_children (GTK_CONTAINER (digit_hbox));
    created_num = g_list_length (list);
    if (created_num > 0) {
        code = digit_hbox_get_code_point (digit_hbox);
    }
    if (created_num >= n_digit) {
        for (i = 0; i < n_digit; i++) {
            g_return_if_fail (INPUT_PAD_IS_GTK_COMBO_BOX (list->data));
            input_pad_gtk_combo_box_set_base (INPUT_PAD_GTK_COMBO_BOX (list->data),
                                              n_encoding);
            gtk_widget_show (GTK_WIDGET (list->data));
            list = list->next;
        }
        while (list) {
            gtk_widget_hide (GTK_WIDGET (list->data));
            list = list->next;
        }
        digit_hbox_set_code_point (digit_hbox, code);
        char_label_set_code_point (char_label, code);
        return;
    }

    for (i = 0; i < created_num; i++) {
        g_return_if_fail (INPUT_PAD_IS_GTK_COMBO_BOX (list->data));
        input_pad_gtk_combo_box_set_base (INPUT_PAD_GTK_COMBO_BOX (list->data),
                                          n_encoding);
        gtk_widget_show (GTK_WIDGET (list->data));
        list = list->next;
    }

    for (i = created_num; i < n_digit; i++) {
        combobox = input_pad_gtk_combo_box_new ();
        input_pad_gtk_combo_box_set_base (INPUT_PAD_GTK_COMBO_BOX (combobox),
                                          n_encoding);
        gtk_box_pack_start (GTK_BOX (digit_hbox), combobox, FALSE, FALSE, 0);
        model = digit_model_new ();
        gtk_combo_box_set_model (GTK_COMBO_BOX (combobox), model);
        g_object_unref (G_OBJECT (model));
        if (gtk_tree_model_get_iter_first (model, &iter)) {
            gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combobox), &iter);
        }

        renderer = gtk_cell_renderer_text_new ();
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combobox), renderer, FALSE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combobox), renderer,
                                        "text", DIGIT_TEXT_COL,
                                        "visible", DIGIT_VISIBLE_COL,
                                        NULL);

        gtk_widget_show (combobox);
        g_signal_connect (G_OBJECT (combobox), "changed",
                          G_CALLBACK (on_combobox_changed), cp_data);
    }
    digit_hbox_set_code_point (digit_hbox, code);
    char_label_set_code_point (char_label, code);
}

#if 0
static guint
keysym_get_value_from_string (const gchar *str)
{
    int i;
    guint retval = 0x0;

    for (i = 0; input_pad_keysym_table[i].str;  i++) {
        if (!g_strcmp0 (input_pad_keysym_table[i].str, str)) {
            retval = input_pad_keysym_table[i].keysym;
            break;
        }
    }
    if (retval == 0x0) {
        g_warning ("keysym str %s does not have the value.", str);
    }
    return retval;
}
#endif

#if 0
guint **
xkb_key_list_get_all_keysyms_from_keysym (InputPadXKBKeyList *xkb_key_list,
                                          guint               keysym)
{
    InputPadXKBKeyList *list = xkb_key_list;
    InputPadXKBKeyRow  *key_row;
    int i, j;
    gboolean found = FALSE;

    g_return_val_if_fail (xkb_key_list != NULL, NULL);

    for (i = 0; i < 20; i++) {
        while (list) {
            key_row = list->row;
            while (key_row) {
                if (!key_row->keysym[i]) {
                    goto out_all_keysyms_check;
                }
                for (j = 0; key_row->keysym[i][j]; j++) {
                    if (keysym == key_row->keysym[i][j]) {
                        found = TRUE;
                        goto out_all_keysyms_check;
                    }
                }
                key_row = key_row->next;
            }
            list = list->next;
        }
    }
out_all_keysyms_check:
    if (found) {
        return key_row->keysym;
    }
    return NULL;
}
#endif

static void
create_char_table (GtkWidget *vbox, InputPadTable *table_data)
{
    GtkWidget *scrolled;
    GtkWidget *viewport;
    GtkWidget *table;
    GtkWidget *button = NULL;
    gchar **char_table;
    gchar *str;
    gchar buff[6];
    const int TABLE_COLUMN = table_data->column;
    int i, num, raw, col, len, code;
    guint keysym;
#if 0
    guint **keysyms;
    InputPadXKBKeyList *xkb_key_list = NULL;
#endif

    if (table_data->priv->inited) {
        return;
    }

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_size_request (scrolled, 300, 200);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                    GTK_POLICY_NEVER,
                                    GTK_POLICY_ALWAYS);
    gtk_box_pack_start (GTK_BOX (vbox), scrolled, FALSE, FALSE, 0);
    gtk_widget_show (scrolled);

    viewport = gtk_viewport_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (scrolled), viewport);
    gtk_widget_show (viewport);

    if (table_data->type == INPUT_PAD_TABLE_TYPE_CHARS) {
        char_table = g_strsplit_set (table_data->data.chars, " \t\n", -1);
    } else if (table_data->type == INPUT_PAD_TABLE_TYPE_KEYSYMS) {
        char_table = g_strsplit_set (table_data->data.keysyms, " \t\n", -1);
    } else {
        g_warning ("Currently your table type is not supported.");
        table_data->priv->inited = 1;
        return;
    }
    for (i = 0, num = 0; char_table[i]; i++) {
        str = char_table[i];
        len = strlen (str);
        if (len > 0) {
            num++;
        }
    }
    col = TABLE_COLUMN;
    raw = num / col;
    if (num % col) {
        raw++;
    }

#if 0
    if (INPUT_PAD_IS_GTK_WINDOW (table_data->priv->signal_window)) {
        xkb_key_list =
            INPUT_PAD_GTK_WINDOW (table_data->priv->signal_window)->priv->xkb_key_list;
    }
#endif

    table = gtk_table_new (raw, col, TRUE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 0);
    gtk_table_set_col_spacings (GTK_TABLE (table), 0);
    gtk_container_add (GTK_CONTAINER (viewport), table);
    gtk_widget_show (table);

    for (i = 0, num = 0; char_table[i]; i++) {
        str = char_table[i];
        len = strlen (str);
        if (len > 0) {
            if (table_data->type == INPUT_PAD_TABLE_TYPE_CHARS) {
                if (str[0] == '0' && 
                    (str[1] == 'x' || str[1] == 'X')) {
                    str += 2;
                    len -= 2;
                }
                code = (int) g_ascii_strtoll (str, NULL, 16);
                buff[g_unichar_to_utf8 ((gunichar) code, buff)] = '\0';
                button = input_pad_gtk_button_new_with_label (buff);
                /* Decided input-pad always sends char but not keysym.
                 * Now keyboard layout can be used instead. */
#if 0
                if (code <= 0x7f) {
                    input_pad_gtk_button_set_keysym (INPUT_PAD_GTK_BUTTON (button),
                                                     code);
                    keysyms = NULL;
                    if (xkb_key_list) {
                        keysyms = xkb_key_list_get_all_keysyms_from_keysym (xkb_key_list,
                                                                           code);
                    }
                    if (keysyms) {
                        input_pad_gtk_button_set_all_keysyms (INPUT_PAD_GTK_BUTTON (button),
                                                              keysyms);
                    }
                }
#endif
            } else if (table_data->type == INPUT_PAD_TABLE_TYPE_KEYSYMS) {
                button = input_pad_gtk_button_new_with_label (str);
                keysym = XStringToKeysym (str);
                if (keysym == NoSymbol) {
                    g_warning ("keysym str %s does not have the value.", str);
                }
                input_pad_gtk_button_set_keysym (INPUT_PAD_GTK_BUTTON (button),
                                                 keysym);
            }
            input_pad_gtk_button_set_table_type (INPUT_PAD_GTK_BUTTON (button),
                                                 table_data->type);
            raw = num / TABLE_COLUMN;
            col = num % TABLE_COLUMN;
            gtk_table_attach_defaults (GTK_TABLE (table), button,
                                       col, col + 1, raw, raw + 1);
            gtk_widget_show (button);
            g_signal_connect (G_OBJECT (button), "pressed",
                              G_CALLBACK (on_button_pressed),
                              table_data->priv->signal_window);
            num++;
        }
    }
    g_strfreev (char_table);

    table_data->priv->inited = 1;
}

gchar *
get_keysym_display_name (guint keysym, GtkWidget *widget, gchar **tooltipp)
{
    char *tooltip;
    char *keysym_name;
    char *display_name;
    char buff[7];
    gunichar ch;

    if ((tooltip = XKeysymToString (keysym)) == NULL) {
        keysym_name = tooltip = "";
    } else if (g_str_has_prefix (tooltip, "KP_")) {
        keysym_name = tooltip + 3;
    } else {
        keysym_name = tooltip;
    }
    // or KeySymToUcs4 (keysym);
    if ((ch = gdk_keyval_to_unicode (keysym)) != 0 &&
        g_unichar_validate (ch)) {
        buff[g_unichar_to_utf8 (ch, buff)] = '\0';
        display_name = g_strdup (buff);
    } else if (g_str_has_prefix (keysym_name, "XF86_Switch_VT_")) {
        display_name = g_strdup_printf ("V%s",
                                        keysym_name + strlen ("XF86_Switch_VT_"));
    } else if (!g_strncasecmp (keysym_name, "Control_", strlen ("Control_"))) {
        display_name = g_strdup ("Ctl");
    } else if (!g_strcmp0 (keysym_name, "Zenkaku_Hankaku") ||
               !g_strcmp0 (keysym_name, "Henkan_Mode")) {
        display_name = g_strdup ("\xe5\x8d\x8a");
    } else if (!g_strcmp0 (keysym_name, "Return") ||
               !g_strcmp0 (keysym_name, "Enter")) {
        display_name = g_strdup ("\xe2\x86\xb5");
    } else if (!g_strcmp0 (keysym_name, "BackSpace")) {
        display_name = g_strdup ("BS");
    } else if (!g_strcmp0 (keysym_name, "Left")) {
        display_name = g_strdup ("\xe2\x86\x90");
    } else if (!g_strcmp0 (keysym_name, "Up")) {
        display_name = g_strdup ("\xe2\x86\x91");
    } else if (!g_strcmp0 (keysym_name, "Right")) {
        display_name = g_strdup ("\xe2\x86\x92");
    } else if (!g_strcmp0 (keysym_name, "Down")) {
        display_name = g_strdup ("\xe2\x86\x93");
    } else if (!g_strcmp0 (keysym_name, "Prior")) {
        display_name = g_strdup ("PU");
    } else if (!g_strcmp0 (keysym_name, "Next")) {
        display_name = g_strdup ("PD");
    } else if (!g_strcmp0 (keysym_name, "Begin")) {
        display_name = g_strdup ("\xc2\xb7");
    } else if (strlen (keysym_name) > 3) {
        display_name = g_strndup (keysym_name, 3);
    } else {
        display_name = g_strdup (keysym_name);
    }
    if (tooltipp) {
        *tooltipp = tooltip;
    }
    return display_name;
}

static void
destroy_prev_keyboard_layout (GtkWidget *vbox, InputPadGtkWindow *window)
{
    GList *children, *buttons;
    GtkWidget *hbox;
    GtkWidget *table;
    GtkWidget *button;

    children = gtk_container_get_children (GTK_CONTAINER (vbox));
    hbox = GTK_WIDGET (children->data);
    children = gtk_container_get_children (GTK_CONTAINER (hbox));
    while (children) {
        table = GTK_WIDGET (children->data);
        buttons = gtk_container_get_children (GTK_CONTAINER (table));
        while (buttons) {
            button = GTK_WIDGET (buttons->data);
            buttons = buttons->next;
            g_signal_handlers_disconnect_by_func (G_OBJECT (window),
                                                  G_CALLBACK (on_window_keyboard_changed),
                                                  (gpointer) button);
            gtk_widget_hide (button);
            gtk_widget_destroy (button);
        }
        children = children->next;
        gtk_widget_hide (table);
        gtk_widget_destroy (table);
    }
    gtk_widget_hide (hbox);
    gtk_widget_destroy (hbox);
}

static void
create_keyboard_layout_ui_real (GtkWidget *vbox, InputPadGtkWindow *window)
{
    InputPadXKBKeyList         *xkb_key_list = window->priv->xkb_key_list;
    InputPadXKBKeyList         *list;
    InputPadXKBKeyRow          *key_row;
    static KeyboardLayoutPart table_data[N_KEYBOARD_LAYOUT_PART] = {
        {0, 0, 0, NULL},
        {6, 0, 0, NULL},
        {10, 0, 0, NULL},
    };
    int i, j, n, col, max_col, total_col, row, max_row;
    GtkWidget *hbox;
    GtkWidget *table;
    GtkWidget *button;
    GtkWidget *button_shift_l = NULL;
    GtkWidget *button_shift_r = NULL;
    GtkWidget *button_num_lock = NULL;
    GList *children;
    char *tooltip;
    char *display_name;

    g_return_if_fail (xkb_key_list != NULL);

    hbox = gtk_hbox_new (FALSE, 5);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_box_reorder_child (GTK_BOX (vbox), hbox, 0);
    gtk_widget_show (hbox);

    total_col = max_col = max_row = row = 0;
    list = xkb_key_list;
    for (i = 0; list; i++) {
        for (j = 0; j < N_KEYBOARD_LAYOUT_PART; j++) {
            if (table_data[j].key_row_id == i) {
                if (j > 0) {
                    table_data[j - 1].row = row;
                    table_data[j - 1].col = max_col;
                }
                total_col += max_col;
                if (row > max_row) {
                    max_row = row;
                }
                max_col = 0;
                row = 0;
                break;
            }
        }
        col = 0;
        key_row = list->row;
        while (key_row) {
            col++;
            key_row = key_row->next;
        }
        if (col > max_col) {
            max_col = col;
        }
        row++;
        list = list->next;
    }
    table_data[N_KEYBOARD_LAYOUT_PART - 1].row = row;
    table_data[N_KEYBOARD_LAYOUT_PART - 1].col = max_col;
    total_col += max_col;
    if (row > max_row) {
        max_row = row;
    }
    for (i = 0; i < N_KEYBOARD_LAYOUT_PART; i++) {
        if (i == 0) {
            /* for arrow label */
            table = gtk_table_new (max_row, table_data[i].col + 1, TRUE);
            gtk_widget_show (table);
        } else {
            table = gtk_table_new (max_row, table_data[i].col, TRUE);
        }
        gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 0);
        table_data[i].table = table;
    }

    list = xkb_key_list;
    row = 0;
    col = max_col = row = 0;
    n = 0;
    for (i = 0; list; i++) {
        for (j = 0; j < N_KEYBOARD_LAYOUT_PART; j++) {
            if (table_data[j].key_row_id == i) {
                row = 0;
                max_col = 0;
                if (j > 0) {
                    n++;
                }
                break;
            }
        }
        col = 0;
        key_row = list->row;
        while (key_row) {
            display_name = get_keysym_display_name (key_row->keysym[0][0],
                                                    GTK_WIDGET (window),
                                                    &tooltip);
            button = input_pad_gtk_button_new_with_label (display_name);
            gtk_widget_set_tooltip_text (button, tooltip);
            g_free (display_name);
            input_pad_gtk_button_set_keycode (INPUT_PAD_GTK_BUTTON (button),
                                              (guint) key_row->keycode);
            input_pad_gtk_button_set_keysym (INPUT_PAD_GTK_BUTTON (button),
                                             key_row->keysym[0][0]);
            input_pad_gtk_button_set_all_keysyms (INPUT_PAD_GTK_BUTTON (button),
                                                  key_row->keysym);
            input_pad_gtk_button_set_table_type (INPUT_PAD_GTK_BUTTON (button),
                                                 INPUT_PAD_TABLE_TYPE_KEYSYMS);
            gtk_table_attach_defaults (GTK_TABLE (table_data[n].table), button,
                                       col, col + 1, row, row + 1);
            gtk_widget_show (button);
            g_signal_connect (G_OBJECT (button), "pressed",
                              G_CALLBACK (on_button_pressed),
                              (gpointer) window);
            g_signal_connect (G_OBJECT (window), "keyboard-changed",
                              G_CALLBACK (on_window_keyboard_changed),
                              (gpointer) button);
            if (key_row->keysym[0][0] == XK_Shift_L) {
                button_shift_l = button;
            } else if (key_row->keysym[0][0] == XK_Shift_R) {
                button_shift_r = button;
            } else if (key_row->keysym[0][0] == XK_Control_L ||
                       key_row->keysym[0][0] == XK_Control_R) {
                g_signal_connect (G_OBJECT (button), "clicked",
                                  G_CALLBACK (on_button_ctrl_clicked),
                                  (gpointer) window);
            } else if (key_row->keysym[0][0] == XK_Alt_L ||
                       key_row->keysym[0][0] == XK_Alt_R) {
                g_signal_connect (G_OBJECT (button), "clicked",
                                  G_CALLBACK (on_button_alt_clicked),
                                  (gpointer) window);
            } else if (key_row->keysym[0][0] == XK_Num_Lock) {
                button_num_lock = button;
            }
            col++;
            key_row = key_row->next;
        }
        if (col > max_col) {
            max_col = col;
        }
        row++;
        list = list->next;
    }

    children = gtk_container_get_children (GTK_CONTAINER (table_data[0].table));
    while (children) {
        if (!GTK_IS_BUTTON (children->data)) {
            children = children->next;
            continue;
        }
        if (button_shift_l) {
            g_signal_connect (G_OBJECT (button_shift_l), "clicked",
                              G_CALLBACK (on_button_shift_clicked),
                              (gpointer) children->data);
        }
        if (button_shift_r) {
            g_signal_connect (G_OBJECT (button_shift_r), "clicked",
                              G_CALLBACK (on_button_shift_clicked),
                              (gpointer) children->data);
        }
        children = children->next;
    }
    children = gtk_container_get_children (GTK_CONTAINER (table_data[2].table));
    while (children) {
        if (!GTK_IS_BUTTON (children->data)) {
            children = children->next;
            continue;
        }
        if (button_num_lock) {
            g_signal_connect (G_OBJECT (button_num_lock), "clicked",
                              G_CALLBACK (on_button_shift_clicked),
                              (gpointer) children->data);
        }
        children = children->next;
    }

    button = gtk_button_new_with_label ("->");
    gtk_widget_set_tooltip_text (button, _("Extend layout"));
    gtk_table_attach_defaults (GTK_TABLE (table_data[0].table), button,
                               table_data[0].col, table_data[0].col + 1,
                               max_row - 1, max_row);
    gtk_widget_show (button);
    g_signal_connect (G_OBJECT (button), "pressed",
                      G_CALLBACK (on_button_layout_arrow_pressed),
                      table_data);
}

static int
sort_layout_name (GtkTreeModel *model,
                  GtkTreeIter  *a,
                  GtkTreeIter  *b,
                  gpointer      data)
{
    gchar *layout_a = NULL;
    gchar *layout_b = NULL;
    const gchar *en_layout = "USA";
    int retval;

    gtk_tree_model_get (model, a,
                        LAYOUT_LAYOUT_DESC_COL, &layout_a, -1);
    gtk_tree_model_get (model, b,
                        LAYOUT_LAYOUT_DESC_COL, &layout_b, -1);
    if (layout_a && !g_strcmp0 (layout_a, en_layout)) {
        g_free (layout_a);
        g_free (layout_b);
        return -1;
    } else if (layout_b && !g_strcmp0 (layout_b, en_layout)) {
        g_free (layout_a);
        g_free (layout_b);
        return 1;
    }
    retval = g_strcmp0 (layout_a, layout_b);
    g_free (layout_a);
    g_free (layout_b);

    return retval;
}

static GtkTreeModel *
layout_model_new (InputPadXKBLayoutList *xkb_layout_list)
{
    InputPadXKBLayoutList *layouts = xkb_layout_list;
    GtkTreeStore *store;
    GtkTreeIter   iter;

    store = gtk_tree_store_new (LAYOUT_N_COLS, G_TYPE_STRING, G_TYPE_STRING,
                                G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);
    while (layouts) {
        gtk_tree_store_append (store, &iter, NULL);
        gtk_tree_store_set (store, &iter,
                            LAYOUT_LAYOUT_NAME_COL, layouts->layout,
                            LAYOUT_LAYOUT_DESC_COL, layouts->desc ? layouts->desc : layouts->layout,
                            LAYOUT_VARIANT_NAME_COL, "",
                            LAYOUT_VARIANT_DESC_COL, "",
                            LAYOUT_VISIBLE_COL, TRUE, -1);
        layouts = layouts->next;
    }
    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (store),
                                     LAYOUT_LAYOUT_DESC_COL,
                                     sort_layout_name,
                                     NULL, NULL);
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
                                          LAYOUT_LAYOUT_DESC_COL,
                                          GTK_SORT_ASCENDING);
    return GTK_TREE_MODEL (store);
}

static void
create_keyboard_layout_list_ui_real (GtkWidget *vbox, InputPadGtkWindow *window)
{
    InputPadXKBConfigReg *xkb_config_reg = window->priv->xkb_config_reg;
    InputPadXKBLayoutList *layouts;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *combobox;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkCellRenderer *renderer;

    g_return_if_fail (xkb_config_reg != NULL);
    layouts = xkb_config_reg->layouts;
    g_return_if_fail (layouts != NULL);

    hbox = gtk_hbox_new (FALSE, 5);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    label = gtk_label_new_with_mnemonic (_("_Layout:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    combobox = gtk_combo_box_new ();
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), combobox);
    gtk_box_pack_start (GTK_BOX (hbox), combobox, FALSE, FALSE, 0);
    model = layout_model_new (layouts);
    gtk_combo_box_set_model (GTK_COMBO_BOX (combobox), model);
    g_object_unref (G_OBJECT (model));
    if (gtk_tree_model_get_iter_first (model, &iter)) {
        gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combobox), &iter);
    }

    renderer = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combobox), renderer, FALSE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combobox), renderer,
                                    "text", LAYOUT_LAYOUT_DESC_COL,
                                    "visible", LAYOUT_VISIBLE_COL,
                                    NULL);
    gtk_widget_show (combobox);

    g_signal_connect (G_OBJECT (window), "keyboard-changed",
                      G_CALLBACK (on_window_keyboard_changed_combobox),
                      (gpointer) combobox);
    g_signal_connect (G_OBJECT (combobox), "changed",
                      G_CALLBACK (on_combobox_layout_changed),
                      (gpointer) window);
}

static void
set_about (GtkWidget *widget)
{
    GtkAboutDialog *about_dlg = GTK_ABOUT_DIALOG (widget);
    gchar *license1, *license2, *license3, *license;
    license1 = _(""
"This program is free software; you can redistribute it and/or modify "
"it under the terms of the GNU General Public License as published by "
"the Free Software Foundation; either version 2 of the License, or "
"(at your option) any later version.");

    license2 = _(""
"This program is distributed in the hope that it will be useful, "
"but WITHOUT ANY WARRANTY; without even the implied warranty of "
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
"GNU General Public License for more details.");

    license3 = _(""
"You should have received a copy of the GNU General Public License along "
"with this program; if not, write to the Free Software Foundation, Inc., "
"51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.");

    license = g_strdup_printf ("%s\n\n%s\n\n%s",
                               license1, license2, license3);
    gtk_about_dialog_set_license (about_dlg, license);
    gtk_about_dialog_set_version (about_dlg, VERSION);
    g_free (license);
}

static GtkWidget *
sub_notebook_new_with_data (InputPadTable *table_data,
                            GtkWidget         *signal_window)
{
    GtkWidget *notebook;
    InputPadTable *table = table_data;
    GtkWidget *label;
    GtkWidget *vbox;

    g_return_val_if_fail (table_data != NULL, NULL);

    notebook = gtk_notebook_new ();
    while (table) {
        label = gtk_label_new (table->name);
        vbox = gtk_vbox_new (FALSE, 10);
        gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                                  vbox,
                                  label);
        gtk_widget_show (label);
        gtk_widget_show (vbox);
        table->priv->signal_window = signal_window;
        table = table->next;
    }

    return notebook;
}

static void
load_notebook_data (GtkWidget *main_notebook, 
                    InputPadGroup *group_data,
                    GtkWidget *signal_window)
{
    InputPadGroup *group = group_data;
    GtkWidget *label;
    GtkWidget *sub_notebook;

    g_return_if_fail (GTK_IS_NOTEBOOK (main_notebook));
    g_return_if_fail (group_data != NULL);

    while (group) {
        label = gtk_label_new (group->name);
        sub_notebook = sub_notebook_new_with_data (group->table, signal_window);
        gtk_notebook_append_page (GTK_NOTEBOOK (main_notebook),
                                  sub_notebook,
                                  label);
        gtk_widget_show (label);
        gtk_widget_show (sub_notebook);
        group->priv->signal_window = signal_window;
        group = group->next;
    }
}

static int
sort_char_block_start (GtkTreeModel *model,
                       GtkTreeIter  *a,
                       GtkTreeIter  *b,
                       gpointer      data)
{
    unsigned int start_a = 0;
    unsigned int start_b = 0;

    gtk_tree_model_get (model, a,
                        CHAR_BLOCK_START_COL, &start_a, -1);
    gtk_tree_model_get (model, b,
                        CHAR_BLOCK_START_COL, &start_b, -1);
    return (start_a - start_b);
}

static GtkTreeModel *
char_block_model_new (void)
{
    GtkTreeStore *store;
    GtkTreeIter   iter;
    int i, j;
    unsigned int start, end;
    gchar *range;
    gchar *range2;
    gchar buff[7];
    gchar buff2[35]; /* 7 x 5 e.g. 'a' -> '0x61 ' */
    gchar buff3[35];

    store = gtk_tree_store_new (CHAR_BLOCK_N_COLS,
                                G_TYPE_STRING, G_TYPE_STRING,
                                G_TYPE_STRING,
                                G_TYPE_UINT, G_TYPE_UINT,
                                G_TYPE_BOOLEAN);
    for (i = 0; input_pad_unicode_block_table[i].label; i++) {
        gtk_tree_store_append (store, &iter, NULL);
        start = input_pad_unicode_block_table[i].start;
        end = input_pad_unicode_block_table[i].end;
        range = g_strdup_printf ("U+%06X - U+%06X", start, end);

        buff[g_unichar_to_utf8 ((gunichar) start, buff)] = '\0';
        buff2[0] = '\0';
        for (j = 0; buff[j] && j < 7; j++) {
            sprintf (buff2 + j * 5, "0x%02X ", (unsigned char) buff[j]);
        }
        if (buff[0] == '\0') {
            buff2[0] = '0'; buff2[0] = 'x'; buff2[1] = '0'; buff2[2] = '0';
            buff2[3] = '\0';
        }
        buff[g_unichar_to_utf8 ((gunichar) end, buff)] = '\0';
        buff3[0] = '\0';
        for (j = 0; buff[j] && j < 7; j++) {
            sprintf (buff3 + j * 5, "0x%02X ", (unsigned char) buff[j]);
        }
        range2 = g_strdup_printf ("%s - %s", buff2, buff3);

        gtk_tree_store_set (store, &iter,
                            CHAR_BLOCK_LABEL_COL,
                            _(input_pad_unicode_block_table[i].label),
                            CHAR_BLOCK_UNICODE_COL, range,
                            CHAR_BLOCK_UTF8_COL, range2,
                            CHAR_BLOCK_START_COL, start,
                            CHAR_BLOCK_END_COL, end,
                            CHAR_BLOCK_VISIBLE_COL, TRUE, -1);
        g_free (range);
        g_free (range2);
    }
    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (store),
                                     CHAR_BLOCK_START_COL,
                                     sort_char_block_start,
                                     NULL, NULL);
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
                                          CHAR_BLOCK_START_COL,
                                          GTK_SORT_ASCENDING);
    return GTK_TREE_MODEL (store);
}

static void
create_about_dialog_ui (GtkBuilder *builder, GtkWidget *window)
{
    GtkWidget *about_dlg;
    GtkAction *about_item;

    about_dlg = GTK_WIDGET (gtk_builder_get_object (builder, "AboutDialog"));
    set_about (about_dlg);
    about_item = GTK_ACTION (gtk_builder_get_object (builder, "About"));
    g_signal_connect (G_OBJECT (about_item), "activate",
                      G_CALLBACK (on_item_dialog_activate),
                      (gpointer) about_dlg);
}

static void
create_code_point_dialog_ui (GtkBuilder *builder, GtkWidget *window)
{
    GtkWidget *cp_dlg;
    GtkWidget *digit_hbox;
    GtkWidget *char_label;
    GtkWidget *encoding_hbox;
    GtkWidget *base_hbox;
    GtkWidget *send_button;
    GtkWidget *close_button;
    GList *list;
    GtkAction *cp_item;
    static CodePointData cp_data = {NULL, NULL};

    cp_dlg = GTK_WIDGET (gtk_builder_get_object (builder, "CodePointDialog"));
    cp_item = GTK_ACTION (gtk_builder_get_object (builder, "CodePoint"));
    g_signal_connect (G_OBJECT (cp_item), "activate",
                      G_CALLBACK (on_item_dialog_activate),
                      (gpointer) cp_dlg);

    digit_hbox = GTK_WIDGET (gtk_builder_get_object (builder, "CodePointDigitHBox"));
    char_label = GTK_WIDGET (gtk_builder_get_object (builder, "CodePointCharLabel"));
    cp_data.signal_window = window;
    cp_data.digit_hbox = digit_hbox;
    cp_data.char_label = char_label;
    encoding_hbox = GTK_WIDGET (gtk_builder_get_object (builder, "EncodingHBox"));
    list = gtk_container_get_children (GTK_CONTAINER (encoding_hbox));
    while (list) {
        if (GTK_IS_RADIO_BUTTON (list->data)) {
            g_signal_connect (G_OBJECT (list->data), "clicked",
                                        G_CALLBACK (on_button_encoding_clicked),
                                        (gpointer) &cp_data);
        }
        list = list->next;
    }

    base_hbox = GTK_WIDGET (gtk_builder_get_object (builder, "BaseHBox"));
    list = gtk_container_get_children (GTK_CONTAINER (base_hbox));
    while (list) {
        if (GTK_IS_RADIO_BUTTON (list->data)) {
            g_signal_connect (G_OBJECT (list->data), "clicked",
                                        G_CALLBACK (on_button_base_clicked),
                                        (gpointer) &cp_data);
        }
        list = list->next;
    }

    send_button = GTK_WIDGET (gtk_builder_get_object (builder, "CodePointSendButton"));
    close_button = GTK_WIDGET (gtk_builder_get_object (builder, "CodePointCloseButton"));
    g_signal_connect (G_OBJECT (send_button), "clicked",
                      G_CALLBACK (on_button_send_clicked), (gpointer) &cp_data);
    g_signal_connect (G_OBJECT (close_button), "clicked",
                      G_CALLBACK (on_button_ok_clicked), (gpointer) cp_dlg);

    set_code_point_base (&cp_data, 16);
}

static void
create_contents_dialog_ui (GtkBuilder *builder, GtkWidget *window)
{
    GtkWidget *contents_dlg;
    GtkAction *contents_item;
    GtkWidget *contents_ok;

    contents_dlg = GTK_WIDGET (gtk_builder_get_object (builder, "ContentsDialog"));
    contents_item = GTK_ACTION (gtk_builder_get_object (builder, "Contents"));
    contents_ok = GTK_WIDGET (gtk_builder_get_object (builder, "ContentsOKButton"));
    g_signal_connect (G_OBJECT (contents_item), "activate",
                      G_CALLBACK (on_item_dialog_activate),
                      (gpointer) contents_dlg);
    g_signal_connect (G_OBJECT (contents_ok), "clicked",
                      G_CALLBACK (on_button_ok_clicked), (gpointer) contents_dlg);
}

static void
create_char_notebook_ui (GtkBuilder *builder, GtkWidget *window)
{
    InputPadGroup *group;
    GtkWidget *main_notebook;
    GtkWidget *sub_notebook;
    GtkToggleAction *show_item;
    int i, n;

    group = INPUT_PAD_GTK_WINDOW (window)->priv->group;

    main_notebook = GTK_WIDGET (gtk_builder_get_object (builder, "TopNotebook"));
    load_notebook_data (main_notebook, group, window);
    g_signal_connect (G_OBJECT (main_notebook), "switch-page",
                      G_CALLBACK (on_main_switch_page), group);
    n = gtk_notebook_get_n_pages (GTK_NOTEBOOK (main_notebook));
    for (i = 0; i < n; i++) {
        sub_notebook = gtk_notebook_get_nth_page (GTK_NOTEBOOK (main_notebook),
                                                  i);
         g_signal_connect (G_OBJECT (sub_notebook), "switch-page",
                           G_CALLBACK (on_sub_switch_page), group->table);
        if (i == 0) {
            on_sub_switch_page (GTK_NOTEBOOK (sub_notebook), NULL, i, group->table);
        }
        group = group->next;
    }

    show_item = GTK_TOGGLE_ACTION (gtk_builder_get_object (builder, "ShowCustomChars"));
    if (gtk_toggle_action_get_active (show_item)) {
        gtk_widget_show (main_notebook);
    } else {
        gtk_widget_hide (main_notebook);
    }
    g_signal_connect (G_OBJECT (show_item), "activate",
                      G_CALLBACK (on_toggle_action),
                      (gpointer) main_notebook);
}

static void
destroy_char_view_table (GtkWidget *viewport)
{
    GList *list, *buttons;
    GtkWidget *table;
    GtkWidget *button;

    list = gtk_container_get_children (GTK_CONTAINER (viewport));
    if (list == NULL) {
        return;
    }
    table = list->data;
    g_return_if_fail (GTK_IS_TABLE (table));
    buttons = gtk_container_get_children (GTK_CONTAINER (table));
    while (buttons) {
        button = GTK_WIDGET (buttons->data);
        gtk_widget_hide (button);
        gtk_widget_destroy (button);
        buttons = buttons->next;
    }
    gtk_container_remove (GTK_CONTAINER (viewport), table);
}

static void
append_char_view_table (GtkWidget      *viewport,
                        unsigned int    start,
                        unsigned int    end,
                        GtkWidget      *window)
{
    unsigned int num;
    int col, row, i;
    const int TABLE_COLUMN = 15;
    gchar buff[7];
    gchar buff2[35]; /* 7 x 5 e.g. 'a' -> '0x61 ' */
    gchar *tooltip;
    GtkWidget *table;
    GtkWidget *button;

    if ((end - start) > 1000) {
        g_warning ("Too many chars");
        end = start + 1000;
    }
    col = TABLE_COLUMN;
    row = (end - start + 1) / col;
    if ((end - start + 1) % col) {
        row++;
    }

    table = gtk_table_new (row, col, TRUE);
    gtk_container_add (GTK_CONTAINER (viewport), table);
    gtk_widget_show (table);

    for (num = start; num <= end; num++) {
        if (num == '\t') {
            buff[0] = ' ';
            buff[1] = '\0';
            sprintf (buff2, "0x%02X ", (unsigned char) num);
        } else {
            buff[g_unichar_to_utf8 ((gunichar) num, buff)] = '\0';
            for (i = 0; buff[i] && i < 7; i++) {
                sprintf (buff2 + i * 5, "0x%02X ", (unsigned char) buff[i]);
            }
            if (buff[0] == '\0') {
                buff2[0] = '0'; buff2[0] = 'x'; buff2[1] = '0'; buff2[2] = '0';
                buff2[3] = '\0';
            }
        }
        button = input_pad_gtk_button_new_with_label (buff);
        tooltip = g_strdup_printf ("U+%04X\nUTF-8 %s", num, buff2);
        gtk_widget_set_tooltip_text (GTK_WIDGET (button), tooltip);
        g_free (tooltip);
        input_pad_gtk_button_set_table_type (INPUT_PAD_GTK_BUTTON (button),
                                             INPUT_PAD_TABLE_TYPE_CHARS);
        row = (num - start) / TABLE_COLUMN;
        col = (num - start) % TABLE_COLUMN;
#if 0
        gtk_table_attach (GTK_TABLE (table), button,
                          col, col + 1, row, row + 1,
                          0, 0, 0, 0);
#else
        gtk_table_attach_defaults (GTK_TABLE (table), button,
                                   col, col + 1, row, row + 1);
#endif
        gtk_widget_show (button);
        g_signal_connect (G_OBJECT (button), "pressed",
                          G_CALLBACK (on_button_pressed),
                          window);
    }
}

static void
create_char_view_ui (GtkBuilder *builder, GtkWidget *window)
{
    GtkWidget *hbox;
    GtkWidget *scrolled;
    GtkWidget *viewport;
    GtkWidget *tv;
    GtkTreeModel *model;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeSelection *selection;
    static CharTreeViewData tv_data;
    GtkToggleAction *show_item;

    hbox = GTK_WIDGET (gtk_builder_get_object (builder, "TopCharViewVBox"));

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_size_request (scrolled, 200, 200);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_ALWAYS);
    gtk_box_pack_start (GTK_BOX (hbox), scrolled, FALSE, FALSE, 0);
    gtk_widget_show (scrolled);

    viewport = gtk_viewport_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (scrolled), viewport);
    gtk_widget_show (viewport);

    tv = gtk_tree_view_new ();
    gtk_container_add (GTK_CONTAINER (viewport), tv);
    model = char_block_model_new ();
    gtk_tree_view_set_model (GTK_TREE_VIEW (tv), model);
    g_object_unref (G_OBJECT (model));
    gtk_widget_show (tv);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Block"), renderer,
                                                       "text", CHAR_BLOCK_LABEL_COL,
                                                       "visible", CHAR_BLOCK_VISIBLE_COL,
                                                       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tv), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Unicode", renderer,
                                                       "text", CHAR_BLOCK_UNICODE_COL,
                                                       "visible", CHAR_BLOCK_VISIBLE_COL,
                                                       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tv), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("UTF-8", renderer,
                                                       "text", CHAR_BLOCK_UTF8_COL,
                                                       "visible", CHAR_BLOCK_VISIBLE_COL,
                                                       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tv), column);

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_size_request (scrolled, 470, 200);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_ALWAYS);
    gtk_box_pack_start (GTK_BOX (hbox), scrolled, FALSE, FALSE, 0);
    gtk_widget_show (scrolled);

    viewport = gtk_viewport_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (scrolled), viewport);
    gtk_widget_show (viewport);
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));
    tv_data.viewport = viewport;
    tv_data.window = window;
    g_signal_connect (G_OBJECT (selection), "changed",
                      G_CALLBACK (on_tree_view_select_char_block), &tv_data);

    show_item = GTK_TOGGLE_ACTION (gtk_builder_get_object (builder, "ShowAllChars"));
    if (gtk_toggle_action_get_active (show_item)) {
        gtk_widget_show (hbox);
    } else {
        gtk_widget_hide (hbox);
    }
    g_signal_connect (G_OBJECT (show_item), "activate",
                      G_CALLBACK (on_toggle_action),
                      (gpointer) hbox);
}

static void
create_keyboard_layout_ui (GtkBuilder *builder, GtkWidget *window)
{
    GtkWidget *keyboard_vbox;
    GtkToggleAction *show_item;

    keyboard_vbox = GTK_WIDGET (gtk_builder_get_object (builder, "TopKeyboardLayoutVBox"));
    g_signal_connect_after (G_OBJECT (window), "realize",
                            G_CALLBACK (on_window_realize),
                            (gpointer)keyboard_vbox);

    show_item = GTK_TOGGLE_ACTION (gtk_builder_get_object (builder, "ShowLayout"));
    if (gtk_toggle_action_get_active (show_item)) {
        gtk_widget_show (keyboard_vbox);
    } else {
        gtk_widget_hide (keyboard_vbox);
    }
    g_signal_connect (G_OBJECT (show_item), "activate",
                      G_CALLBACK (on_toggle_action),
                      (gpointer) keyboard_vbox);
}

static GtkWidget *
create_ui (void)
{
    const gchar *filename = INPUT_PAD_UI_FILE;
    GError *error = NULL;
    GtkWidget *window = NULL;
    GtkAction *close_item;
    GtkBuilder *builder = gtk_builder_new ();

    if (!filename ||
        !g_file_test (filename, G_FILE_TEST_EXISTS)) {
        g_error ("File Not Found: %s\n", filename ? filename : "(null)");
        return NULL;
    }

    gtk_builder_set_translation_domain (builder, GETTEXT_PACKAGE);
    gtk_builder_add_from_file (builder, filename, &error);
    if (error) {
        g_error ("ERROR: %s\n",
                 error ? error->message ? error->message : "" : "");
        g_error_free (error);
        return NULL;
    }

    window = GTK_WIDGET (gtk_builder_get_object (builder, "TopWindow"));
    gtk_window_set_icon_from_file (GTK_WINDOW (window),
                                   DATAROOTDIR "/pixmaps/input-pad.png",
                                   &error);
    error = NULL;
    gtk_window_set_default_icon_from_file (DATAROOTDIR "/pixmaps/input-pad.png",
                                           &error);
    g_signal_connect (G_OBJECT (window), "delete_event",
                      G_CALLBACK (on_window_close), NULL);
    g_signal_connect (G_OBJECT (window), "destroy",
                      G_CALLBACK (on_window_close), NULL);

    close_item = GTK_ACTION (gtk_builder_get_object (builder, "Close"));
    g_signal_connect (G_OBJECT (close_item), "activate",
                      G_CALLBACK (on_close_activate), (gpointer) window);

    create_code_point_dialog_ui (builder, window);
    create_about_dialog_ui (builder, window);
    create_contents_dialog_ui (builder, window);
    create_char_notebook_ui (builder, window);
    create_char_view_ui (builder, window);
    create_keyboard_layout_ui (builder, window);

    gtk_builder_connect_signals (builder, NULL);
    g_object_unref (G_OBJECT (builder));

    return window;
}

#ifdef MODULE_XTEST_GDK_BASE

static void
input_pad_gtk_window_xtest_gdk_destroy (InputPadGtkWindow *window)
{
    const gchar *error = NULL;

    g_return_if_fail (window != NULL &&
                      INPUT_PAD_IS_GTK_WINDOW (window));
    g_return_if_fail (window->priv != NULL);
    g_return_if_fail (window->priv->module_gdk_xtest != NULL);

    if (!g_module_close (window->priv->module_gdk_xtest)) {
        error = g_module_error ();
        g_warning ("Cannot close %s: %s", MODULE_XTEST_GDK_BASE,
                   error ? error : "");
        window->priv->module_gdk_xtest = NULL;
        return;
    }
    window->priv->module_gdk_xtest = NULL;
}

static void
input_pad_gtk_window_xtest_gdk_init (InputPadGtkWindow *window)
{
    gchar *filename;
    const gchar *error = NULL;
    const gchar *symbol_name;
    const gchar *module_name;
    GModule *module = NULL;
    gboolean (* init) (InputPadGtkWindow *window);

    g_return_if_fail (window != NULL &&
                      INPUT_PAD_IS_GTK_WINDOW (window));
    g_return_if_fail (window->priv != NULL);

    if (window->priv->module_gdk_xtest) {
        return;
    }

    g_return_if_fail (MODULE_XTEST_GDK_BASE != NULL);

    if (!g_module_supported ()) {
        error = g_module_error ();
        g_warning ("Module (%s) is not supported on your platform: %s",
                   MODULE_XTEST_GDK_BASE, error ? error : "");
        return;
    }

    filename = g_module_build_path (MODULE_XTEST_GDK_DIR, MODULE_XTEST_GDK_BASE);
    g_return_if_fail (filename != NULL);

    module = g_module_open (filename, G_MODULE_BIND_LAZY);
    if (module == NULL) {
        error = g_module_error ();
        g_warning ("Could not open %s: %s", filename, error ? error : "");
        g_free (filename);
        return;
    }
    g_free (filename);

    window->priv->module_gdk_xtest = module;

    symbol_name = "input_pad_module_init";
    module_name = g_module_name (module);
    if (!g_module_symbol (module, symbol_name, (gpointer *)&init)) {
        error = g_module_error ();
        g_warning ("Could not find '%s' in %s: %s", symbol_name,
                   module_name ? module_name : "",
                   error ? error : "");
        input_pad_gtk_window_xtest_gdk_destroy (window);
        return;
    }
    if (init == NULL) {
        g_warning ("Function '%s' is NULL in %s", symbol_name,
                   module_name ? module_name : "");
        input_pad_gtk_window_xtest_gdk_destroy (window);
        return;
    }
    if (!init (window)) {
        g_warning ("Function '%s' failed to be run in %s", symbol_name,
                   module_name ? module_name : "");
        input_pad_gtk_window_xtest_gdk_destroy (window);
        return;
    }
}

static void
input_pad_gtk_window_xtest_gdk_setup (InputPadGtkWindow *window)
{
    const gchar *symbol_name = "input_pad_module_setup";
    const gchar *module_name;
    const gchar *error = NULL;
    gboolean (* setup) (InputPadGtkWindow *window);

    g_return_if_fail (window != NULL &&
                      INPUT_PAD_IS_GTK_WINDOW (window));
    g_return_if_fail (window->priv != NULL);
    g_return_if_fail (window->priv->module_gdk_xtest != NULL);

    module_name = g_module_name (window->priv->module_gdk_xtest);
    if (!g_module_symbol (window->priv->module_gdk_xtest, symbol_name,
                          (gpointer *)&setup)) {
        error = g_module_error ();
        g_warning ("Could not find '%s' in %s: %s", symbol_name,
                   module_name ? module_name : "",
                   error ? error : "");
        return;
    }
    if (setup == NULL) {
        g_warning ("Function '%s' is NULL in %s", symbol_name,
                   module_name ? module_name : "");
        return;
    }
    if (!setup (window)) {
        g_warning ("Function '%s' failed to be run in %s", symbol_name,
                   module_name ? module_name : "");
        return;
    }
}

#endif

static void
input_pad_gtk_window_set_group (InputPadGtkWindow *window)
{
    InputPadGtkWindowPrivate *priv = INPUT_PAD_GTK_WINDOW_GET_PRIVATE (window);
    if (priv->group == NULL) {
        priv->group = input_pad_group_parse_all_files ();
    }
    window->priv = priv;
}

static void
input_pad_gtk_window_init (InputPadGtkWindow *window)
{
    window->child = TRUE;
    input_pad_gtk_window_set_group (window);
#ifdef MODULE_XTEST_GDK_BASE
    if (use_module_xtest) {
        input_pad_gtk_window_xtest_gdk_init (window);
    }
#endif
}

#if 0
static void
input_pad_gtk_window_buildable_parser_finished (GtkBuildable *buildable,
                                                     GtkBuilder   *builder)
{
    InputPadGtkWindow *window = INPUT_PAD_GTK_WINDOW (buildable);
    input_pad_gtk_window_set_group (window);
    parent_buildable_iface->parser_finished (buildable, builder);
}
#endif

static void
input_pad_gtk_window_buildable_interface_init (GtkBuildableIface *iface)
{
#if 0
    parent_buildable_iface = g_type_interface_peek_parent (iface);
    iface->set_buildable_property = parent_buildable_iface->set_buildable_property;
    iface->parser_finished = input_pad_gtk_window_buildable_parser_finished;
    iface->custom_tag_start = parent_buildable_iface->custom_tag_start;
    iface->custom_finished = parent_buildable_iface->custom_finished;
#endif
}

static void
input_pad_gtk_window_real_destroy (GtkObject *object)
{
    InputPadGtkWindow *window = INPUT_PAD_GTK_WINDOW (object);

    if (window->priv) {
        if (window->priv->group) {
            input_pad_group_destroy (window->priv->group);
            window->priv->group = NULL;
        }
#ifdef MODULE_XTEST_GDK_BASE
        if (use_module_xtest) {
            input_pad_gtk_window_xtest_gdk_destroy (window);
        }
#endif
        window->priv = NULL;
    }
    GTK_OBJECT_CLASS (input_pad_gtk_window_parent_class)->destroy (object);
}

static void
input_pad_gtk_window_real_realize (GtkWidget *window)
{
    InputPadGtkWindow *input_pad = INPUT_PAD_GTK_WINDOW (window);

#ifdef MODULE_XTEST_GDK_BASE
    if (use_module_xtest) {
        input_pad_gtk_window_xtest_gdk_setup (input_pad);
    }
#endif
    GTK_WIDGET_CLASS (input_pad_gtk_window_parent_class)->realize (window);
}

static gboolean
input_pad_gtk_window_real_button_pressed (InputPadGtkWindow *window,
                                          const gchar       *str,
                                          guint              type,
                                          guint              keysym,
                                          guint              keycode,
                                          guint              state)
{
    if (type == INPUT_PAD_TABLE_TYPE_CHARS) {
        if (keysym > 0) {
            send_key_event (GTK_WIDGET(window)->window, keysym, keycode, state);
        } else {
            g_print ("%s", str ? str : "");
        }
    } else if (type == INPUT_PAD_TABLE_TYPE_KEYSYMS) {
        send_key_event (GTK_WIDGET(window)->window, keysym, keycode, state);
    } else {
        g_warning ("Currently your table type is not supported.");
    }
    return FALSE;
}

static void
input_pad_gtk_window_class_init (InputPadGtkWindowClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GtkObjectClass *object_class = (GtkObjectClass *) klass;
    GtkWidgetClass *widget_class = (GtkWidgetClass *) klass;

    object_class->destroy = input_pad_gtk_window_real_destroy;
    widget_class->realize = input_pad_gtk_window_real_realize;

    klass->button_pressed = input_pad_gtk_window_real_button_pressed;

    g_type_class_add_private (klass, sizeof (InputPadGtkWindowPrivate));

    signals[BUTTON_PRESSED] =
        g_signal_new (I_("button-pressed"),
                      G_TYPE_FROM_CLASS (gobject_class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (InputPadGtkWindowClass, button_pressed),
                      g_signal_accumulator_true_handled, NULL,
                      INPUT_PAD_BOOL__STRING_UINT_UINT_UINT_UINT,
                      G_TYPE_BOOLEAN,
                      5, G_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE,
                      G_TYPE_UINT, G_TYPE_UINT,
                      G_TYPE_UINT, G_TYPE_UINT);

    signals[KBD_CHANGED] =
        g_signal_new (I_("keyboard-changed"),
                      G_TYPE_FROM_CLASS (gobject_class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (InputPadGtkWindowClass, keyboard_changed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__INT,
                      G_TYPE_NONE,
                      1, G_TYPE_INT);
}

GtkWidget *
_input_pad_gtk_window_new_with_gtype (GtkWindowType type,
                                      unsigned int child,
                                      gboolean     gtype)
{
    GtkWidget *window;

    g_return_val_if_fail (type >= GTK_WINDOW_TOPLEVEL && type <= GTK_WINDOW_POPUP, NULL);

    /* FIXME: Add INPUT_PAD_TYPE_GTK_WINDOW for a workaround.
     * python + gtkbuilder outputs an error:
     * ** ERROR **: ERROR: Invalid object type `InputPadGtkWindow'
     * swig bug with gtkbuilder? Ignored -export-dynamic
     */
    if (gtype) {
        INPUT_PAD_TYPE_GTK_WINDOW;
    }

    window = create_ui ();
    INPUT_PAD_GTK_WINDOW (window)->parent.type = type;
    INPUT_PAD_GTK_WINDOW (window)->child = child;

    return window; 
}

GtkWidget *
input_pad_gtk_window_new (GtkWindowType type, unsigned int child)
{
    return _input_pad_gtk_window_new_with_gtype (type, child, FALSE);
}

const char*
input_pad_get_version (void)
{
    return VERSION;
}

void
input_pad_window_init (int *argc, char ***argv, InputPadWindowType type)
{
    GOptionContext *context;
    GError *error = NULL;

#ifdef ENABLE_NLS
    bindtextdomain (GETTEXT_PACKAGE, IBUS_LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

    gtk_set_locale ();
#endif

    if (type != INPUT_PAD_WINDOW_TYPE_GTK) {
        g_warning ("Currently GTK type only is supported. Ignoring...");
    }

    g_set_application_name (_("Input Pad"));
    context = g_option_context_new (N_("Input Pad"));
    g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);
    g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
    g_option_context_add_group (context, gtk_get_option_group (TRUE));
    g_option_context_parse (context, argc, argv, &error);
    g_option_context_free (context);

    if (ask_version) {
        g_print ("%s %s version %s\n", g_get_prgname (),
                                       g_get_application_name (),
                                       input_pad_get_version ());
        exit (0);
    }

    gtk_init (argc, argv);
}

void *
input_pad_window_new (unsigned int ibus)
{
    return input_pad_gtk_window_new (GTK_WINDOW_TOPLEVEL, ibus);
}

void *
_input_pad_window_new_with_gtype (unsigned int ibus, gboolean gtype)
{
    return _input_pad_gtk_window_new_with_gtype (GTK_WINDOW_TOPLEVEL,
                                                 ibus, gtype);
}

void
input_pad_window_show (void *window_data)
{
    GtkWidget *window;
    InputPadGtkWindow *input_pad_window;

    g_return_if_fail (window_data != NULL &&
                      INPUT_PAD_IS_GTK_WINDOW (window_data));

    window = GTK_WIDGET (window_data);
    input_pad_window = INPUT_PAD_GTK_WINDOW (window);

    if (input_pad_window->priv && !input_pad_window->priv->show_all) {
        gtk_widget_show_all (window);
        input_pad_window->priv->show_all = TRUE;
    } else {
        gtk_widget_show (window);
    }
}

void
input_pad_window_hide (void *window_data)
{
    g_return_if_fail (window_data != NULL &&
                      GTK_IS_WIDGET (window_data));

    gtk_widget_hide (GTK_WIDGET (window_data));
}

void
input_pad_window_main (void *window_data)
{
    gtk_main ();
}

void
input_pad_window_destroy (void *window_data)
{
    g_return_if_fail (window_data != NULL &&
                      GTK_IS_WIDGET (window_data));

    gtk_widget_destroy (GTK_WIDGET (window_data));
}