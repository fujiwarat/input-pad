/* vim:set et sts=4: */
/* input-pad - The input pad
 * Copyright (C) 2010-2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2010-2025 Red Hat, Inc.
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
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <string.h> /* strlen */

#ifdef ENABLE_NLS
#include <locale.h>
#endif

#include "i18n.h"
#include "button-gtk.h"
#include "combobox-gtk.h"
#include "geometry-gdk.h"
#include "input-pad.h"
#include "input-pad-group.h"
#include "input-pad-kbdui-gtk.h"
#include "input-pad-marshal.h"
#include "input-pad-private.h"
#include "input-pad-window-gtk.h"
#include "unicode_block.h"
#include "viewport-gtk.h"

#define N_KEYBOARD_LAYOUT_PART 3
#define INPUT_PAD_UI_FILE INPUT_PAD_UI_GTK_DIR "/input-pad.ui"
#define MAX_UCODE 0x10ffff
#define MODULE_NAME_PREFIX "input-pad-"
#define USE_GLOBAL_GMODULE 1

#if GTK_CHECK_VERSION (3, 19, 10)
#  define CSS_DATA_NARROW_BUTTON \
    "button    { padding-left: 1px;" \
    "            padding-right: 1px;" \
    "            padding-top: 1px;" \
    "            padding-bottom: 1px;" \
    "            border-top-width: 0px;" \
    "            border-left-width: 0px;" \
    "            border-bottom-width: 0px;" \
    "            border-right-width: 0px; " \
    "            min-width: 0px; }"
#  define CSS_DATA_GRAY_NARROW_BUTTON \
    "button    { padding-left: 1px;" \
    "            padding-right: 1px;" \
    "            padding-top: 1px;" \
    "            padding-bottom: 1px;" \
    "            border-top-width: 0px;" \
    "            border-left-width: 0px;" \
    "            border-bottom-width: 0px;" \
    "            border-right-width: 0px;" \
    "            background-color: LightGray; " \
    "            min-width: 0px; }"
#else
#  define CSS_DATA_NARROW_BUTTON \
    "GtkButton { padding-left: 1px;" \
    "            padding-right: 1px;" \
    "            padding-top: 1px;" \
    "            padding-bottom: 1px;" \
    "            border-top-width: 0px;" \
    "            border-left-width: 0px;" \
    "            border-bottom-width: 0px;" \
    "            border-right-width: 0px; }"
#  define CSS_DATA_GRAY_NARROW_BUTTON \
    "GtkButton { padding-left: 1px;" \
    "            padding-right: 1px;" \
    "            padding-top: 1px;" \
    "            padding-bottom: 1px;" \
    "            border-top-width: 0px;" \
    "            border-left-width: 0px;" \
    "            border-bottom-width: 0px;" \
    "            border-right-width: 0px;" \
    "            background-color: LightGray; }"
#endif

#define INPUT_PAD_GTK_WINDOW_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), INPUT_PAD_TYPE_GTK_WINDOW, InputPadGtkWindowPrivate))

typedef struct _CodePointData CodePointData;
typedef struct _KeyboardLayoutPart KeyboardLayoutPart;
typedef struct _CharTreeViewData CharTreeViewData;
typedef struct _TableForEachData TableForEachData;
typedef struct _InputPadGtkApplicationClass InputPadGtkApplicationClass;

enum {
    BUTTON_PRESSED,
    KBD_CHANGED,
    GROUP_CHANGED,
    GROUP_APPENDED,
    CHAR_BUTTON_SENSITIVE,
    REORDER_BUTTON_PRESSED,
    LAST_SIGNAL
};

enum {
    APP_ACTIVATED,
    APP_LAST_SIGNAL
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

struct _InputPadGtkApplication
{
    GtkApplication     parent;
    GtkBuilder        *builder;
    InputPadGtkWindow *window;
    gboolean           show;
};

struct _InputPadGtkApplicationClass
{
    GtkApplicationClass parent_class;

    void     (* activated) (InputPadGtkApplication *app);
};

G_DEFINE_TYPE(InputPadGtkApplication, input_pad_gtk_application, GTK_TYPE_APPLICATION);

struct _InputPadGtkWindowPrivate {
    InputPadGroup              *group;
    guint                       show_all : 1;
    GModule                    *module_gdk_xtest;
    InputPadXKBKeyList         *xkb_key_list;
    guint                       keyboard_state;
    InputPadXKBConfigReg       *xkb_config_reg;
    gchar                     **group_layouts;
    gchar                     **group_variants;
    gchar                     **group_options;
    guint                       char_button_sensitive : 1;
    /* The kbdui_name is used for each window instance. */
    gchar                      *kbdui_name;
    InputPadGtkKbdui           *kbdui;
    GtkToggleAction            *show_layout_action;
    GtkWidget                  *config_layouts_dialog;
    GtkWidget                  *config_layouts_add_treeview;
    GtkWidget                  *config_layouts_remove_treeview;
    GtkWidget                  *config_layouts_combobox;
    GtkWidget                  *config_options_dialog;
    GtkWidget                  *config_options_vbox;

    GtkWidget                  *top_custom_char_view_hbox;
    GtkWidget                  *top_char_view_hbox;
    GtkWidget                  *top_keyboard_layout_vbox;
};

struct _CodePointData {
    GtkWidget                  *signal_window;
    GtkWidget                  *digit_hbox;
    GtkWidget                  *char_label;
    GtkWidget                  *down_button;
    GtkWidget                  *up_button;
    guint                       codepoint;
};

struct _TableForEachData {
    GtkWidget                  *table;
    InputPadGtkWindow          *window;
};

struct _KeyboardLayoutPart {
    int                         key_row_id;
    int                         row;
    int                         col;
    GtkWidget                  *table;
};

struct _CharTreeViewData {
    GtkWidget                  *scrolled;
    GtkWidget                  *window;
    GtkWidget                  *main_tv;
    GtkWidget                  *sub_tv;
};

static guint                    signals[LAST_SIGNAL] = { 0 };
static guint                    app_signals[APP_LAST_SIGNAL] = { 0 };
#ifdef MODULE_XTEST_GDK_BASE
static gboolean                 use_module_xtest = FALSE;
#endif
/* The kbdui_name is used for CLI only. */
static gchar                   *kbdui_name = NULL;
static gboolean                 ask_version = FALSE;
static guint                    set_show_table_type = 1;
static guint                    set_show_layout_type = 0;

#ifdef USE_GLOBAL_GMODULE
/* module_table: global GModule hash table
 * G_DEFINE_TYPE() uses static volatile address in ##_get_type() so
 * if we close GModule and reopen it, the address will be changed and
 * g_type_register_static_simple() could not get the previous register.
 * The workaround is to have the global GModule table not to be closed
 * even thogh GtkWindow is destroyed. */
static GHashTable              *module_table = NULL;
#endif

#if 0
static GtkBuildableIface       *parent_buildable_iface;
#endif

static GOptionEntry entries[] = {
  { "version", 'v', 0, G_OPTION_ARG_NONE, &ask_version,
    N_("Display version"), NULL},
  { "with-kbdui", 'k', 0, G_OPTION_ARG_STRING, &kbdui_name,
    /* Translators: the word 'KBDUI' is not translated. */
    N_("Use KBDUI to draw keyboard"), "KBDUI"},
  { "with-table-type", 't', 0, G_OPTION_ARG_INT, &set_show_table_type,
    /* Translators: the word 'TYPE' is not translated. */
    N_("Use TYPE of char table. The available TYPE=0, 1, 2"), "TYPE"},
  { "with-layout-type", 'l', 0, G_OPTION_ARG_INT, &set_show_layout_type,
    /* Translators: the word 'TYPE' is not translated. */
    N_("Use TYPE of keyboard layout. The available TYPE=0, 1"), "TYPE"},
  { NULL }
};

#ifdef MODULE_XTEST_GDK_BASE
static GOptionEntry enable_xtest_entry[] = {
  { "enable-module-xtest", 'x', 0, G_OPTION_ARG_NONE, &use_module_xtest,
    N_("Use XTEST module to send key events"), NULL},
};

static GOptionEntry disable_xtest_entry[] = {
  { "disable-module-xtest", 'x', 0, G_OPTION_ARG_NONE, &use_module_xtest,
    N_("Use XSendEvent to send key events"), NULL},
};
#endif

static void             xor_modifiers           (InputPadGtkWindow *window,
                                                 guint              modifiers);
static guint            digit_hbox_get_code_point
                                                (GtkWidget         *digit_hbox);
static void             digit_hbox_set_code_point
                                                (GtkWidget         *digit_hbox,
                                                 guint              code);
static void             char_label_set_code_point
                                                (GtkWidget         *char_label,
                                                 guint              code);
static void             set_code_point_base     (CodePointData     *cp_data,
                                                 int                n_encoding);
static InputPadGroup *  get_nth_pad_group       (InputPadGroup     *group,
                                                 int                nth);
static InputPadTable *  get_nth_pad_table       (InputPadTable     *table,
                                                 int                nth);
static void             run_command             (const gchar       *command,
                                                 gchar            **command_output);
static void             append_custom_char_view_table
                                                (GtkWidget         *scrolled,
                                                 InputPadTable     *table_data);
static void             destroy_custom_char_view_table
                                                (GtkWidget         *scrolled,
                                                 InputPadGtkWindow *window);
static char *           get_keysym_display_name (guint              keysym,
                                                 GtkWidget         *widget,
                                                 gchar            **tooltipp);
static void             create_keyboard_layout_ui_real_default
                                                (GtkWidget         *vbox,
                                                 InputPadGtkWindow *window);
G_INLINE_FUNC void      create_keyboard_layout_ui_real
                                                (GtkWidget         *vbox,
                                                 InputPadGtkWindow *window);
static void             destroy_prev_keyboard_layout_default
                                                (GtkWidget         *vbox,
                                                 InputPadGtkWindow *window);
G_INLINE_FUNC void      destroy_prev_keyboard_layout
                                                (GtkWidget         *vbox,
                                                 InputPadGtkWindow *window);
static void             config_layouts_combobox_remove_layout
                                                (GtkTreeStore *list,
                                                 const gchar *layout_name,
                                                 const gchar *layout_desc,
                                                 const gchar *variant_name,
                                                 const gchar *variant_desc);
static void             config_layouts_list_append_layout
                                                (GtkListStore *list,
                                                 const gchar *layout_name,
                                                 const gchar *layout_desc,
                                                 const gchar *variant_name,
                                                 const gchar *variant_desc);
static void             config_layouts_list_remove_iter
                                                (GtkListStore *list,
                                                 GtkTreeIter  *iter);
static void             create_keyboard_layout_list_ui_real
                                                (GtkWidget         *vbox,
                                                 InputPadGtkWindow *window);
static GtkTreeModel *   custom_char_table_model_new
                                                (InputPadGtkWindow *window,
                                                 InputPadTable     *table);
static void             create_custom_char_views
                                                (GtkWidget         *hbox,
                                                 InputPadGtkWindow *window);
static void             destroy_custom_char_views
                                                (GtkWidget         *hbox,
                                                 InputPadGtkWindow *window);
static void             append_all_char_view_table 
                                                (GtkWidget         *scrolled,
                                                 unsigned int       start,
                                                 unsigned int       end,
                                                 GtkWidget         *window);
static void             destroy_all_char_view_table
                                                (GtkWidget         *scrolled,
                                                 InputPadGtkWindow *window);
static void             input_pad_gtk_window_real_destroy
                                                (GtkWidget         *widget);
static void             on_toggle_action        (GSimpleAction     *action,
                                                 GVariant          *parameter,
                                                 gpointer          app);
static void             on_check_action         (GSimpleAction    *action,
                                                 GVariant         *parameter,
                                                 gpointer          app);

G_DEFINE_TYPE_WITH_PRIVATE (InputPadGtkWindow,
                            input_pad_gtk_window,
                            GTK_TYPE_APPLICATION_WINDOW);

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
    input_pad_gtk_button_set_label_size (button, display_name,
                                         KEYBOARD_ICON_SIZE);
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
            g_free (desc);
            break;
        }
        g_free (layout);
        g_free (desc);
    } while (gtk_tree_model_iter_next (model, &iter));
    g_free (active_layout);
}

static void
on_window_group_changed_custom_char_views (InputPadGtkWindow *window,
                                           gchar             *paddir,
                                           gchar             *domain,
                                           gpointer           data)
{
    GtkWidget *hbox;
    InputPadGroup *custom_group = NULL;

    g_return_if_fail (INPUT_PAD_IS_GTK_WINDOW (window));
    g_return_if_fail (GTK_IS_BOX (data));
    g_return_if_fail (window->priv != NULL);
    g_return_if_fail (window->priv->group != NULL);

    hbox = GTK_WIDGET (data);
    destroy_custom_char_views (hbox, window);
    if (paddir != NULL) {
        custom_group = input_pad_group_parse_all_files (paddir, domain);
    }
    if (custom_group != NULL) {
        input_pad_group_destroy (window->priv->group);
        window->priv->group = custom_group;
    }
    create_custom_char_views (hbox, window);
}

static void
on_window_group_appended_custom_char_views (InputPadGtkWindow *window,
                                            gchar             *padfile,
                                            gchar             *domain,
                                            gpointer           data)
{
    GtkWidget *hbox;
    InputPadGroup *custom_group = NULL;

    g_return_if_fail (INPUT_PAD_IS_GTK_WINDOW (window));
    g_return_if_fail (GTK_IS_BOX (data));
    g_return_if_fail (window->priv != NULL);
    g_return_if_fail (window->priv->group != NULL);

    hbox = GTK_WIDGET (data);
    destroy_custom_char_views (hbox, window);
    if (padfile != NULL) {
        custom_group = input_pad_group_append_from_file (window->priv->group,
                                                         padfile,
                                                         domain);
    }
    if (custom_group != NULL) {
        window->priv->group = custom_group;
    }
    create_custom_char_views (hbox, window);
}

static void
on_window_char_button_sensitive (InputPadGtkWindow *window,
                                 gboolean           sensitive,
                                 gpointer           data)
{
    GtkWidget *button;

    g_return_if_fail (INPUT_PAD_IS_GTK_WINDOW (window));
    g_return_if_fail (GTK_IS_BUTTON (data));

    button = GTK_WIDGET (data);
    gtk_widget_set_sensitive (button, sensitive);
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
        input_pad_gtk_button_set_label_size (gen_button, display_name,
                                             KEYBOARD_ICON_SIZE);
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
on_button_down_pressed (GtkButton *button, gpointer data)
{
    CodePointData *cp_data = (CodePointData*) data;
    guint code;

    if (cp_data->codepoint == 0)
        return;

    code = --cp_data->codepoint;

    digit_hbox_set_code_point (cp_data->digit_hbox, code);
    char_label_set_code_point (cp_data->char_label, code);
}

static void
on_button_up_pressed (GtkButton *button, gpointer data)
{
    CodePointData *cp_data = (CodePointData*) data;
    guint code;

    if (cp_data->codepoint >= MAX_UCODE)
        return;

    code = ++cp_data->codepoint;

    digit_hbox_set_code_point (cp_data->digit_hbox, code);
    char_label_set_code_point (cp_data->char_label, code);
}

static void
on_combobox_layout_changed (GtkComboBox *combobox,
                            gpointer     data)
{
    InputPadGtkWindow *window;
    GtkTreeIter iter;
    GtkTreeModel *model;
    gchar *layout = NULL;
    gchar *variant = NULL;
    gchar *option = NULL;
    GtkWidget *keyboard_vbox;

    g_return_if_fail (data != NULL &&
                      INPUT_PAD_IS_GTK_WINDOW (data));

    window = INPUT_PAD_GTK_WINDOW (data);
    if (!gtk_combo_box_get_active_iter (combobox, &iter)) {
        return;
    }
    model = gtk_combo_box_get_model (combobox);
    gtk_tree_model_get (model, &iter,
                        LAYOUT_LAYOUT_NAME_COL, &layout,
                        LAYOUT_VARIANT_NAME_COL, &variant, -1);
    if (window->priv->group_options) {
        option = g_strjoinv (",", window->priv->group_options);
    }
    input_pad_gdk_xkb_set_layout (window, window->priv->xkb_key_list,
                                  layout, variant, option);
    g_free (layout);
    g_free (variant);
    g_free (option);

    if (window->priv->xkb_key_list) {
        input_pad_gdk_xkb_destroy_keyboard_layouts (window,
                                                    window->priv->xkb_key_list);
        window->priv->xkb_key_list = NULL;
    }
    window->priv->xkb_key_list =
        input_pad_gdk_xkb_parse_keyboard_layouts (window);
    if (window->priv->kbdui_name && window->priv->xkb_key_list == NULL) {
        return;
    }

    keyboard_vbox = gtk_widget_get_parent (gtk_widget_get_parent (GTK_WIDGET (combobox)));
    destroy_prev_keyboard_layout (keyboard_vbox, window);
    create_keyboard_layout_ui_real (keyboard_vbox, window);

    if (window->priv->group_layouts) {
        g_strfreev (window->priv->group_layouts);
        window->priv->group_layouts = NULL;
    }
    if (window->priv->group_variants) {
        g_strfreev (window->priv->group_variants);
        window->priv->group_variants = NULL;
    }
    if (window->priv->group_options) {
        g_strfreev (window->priv->group_options);
        window->priv->group_options = NULL;
    }
    window->priv->group_layouts =
        input_pad_gdk_xkb_get_group_layouts (window,
                                             window->priv->xkb_key_list);
    window->priv->group_variants =
        input_pad_gdk_xkb_get_group_variants (window,
                                              window->priv->xkb_key_list);
    window->priv->group_options =
        input_pad_gdk_xkb_get_group_options (window,
                                             window->priv->xkb_key_list);
    input_pad_gdk_xkb_signal_emit (window, signals[KBD_CHANGED]);
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
on_button_config_layouts_clicked (GtkButton *button, gpointer data)
{
    GtkWidget *dlg = GTK_WIDGET (data);
    GtkWidget *top_window = gtk_widget_get_toplevel (GTK_WIDGET (button));

    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (top_window));
    gtk_dialog_run (GTK_DIALOG (dlg));
    gtk_widget_hide (dlg);
}

static void
on_button_config_layouts_close_clicked (GtkButton *button, gpointer data)
{
    GtkWidget *dlg = GTK_WIDGET (data);

    gtk_widget_hide (dlg);
}

static void
on_button_config_layouts_add_clicked (GtkButton *button, gpointer data)
{
    InputPadGtkWindow *window;
    GtkWidget *combobox;
    GtkWidget *add_treeview;
    GtkWidget *remove_treeview;
    GtkTreeModel *combo_model;
    GtkTreeModel *add_model;
    GtkTreeModel *remove_model;
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    gchar *layout_name = NULL;
    gchar *layout_desc = NULL;
    gchar *variant_name = NULL;
    gchar *variant_desc = NULL;

    g_return_if_fail (data != NULL &&
                      INPUT_PAD_IS_GTK_WINDOW (data));

    window = INPUT_PAD_GTK_WINDOW (data);
    combobox = window->priv->config_layouts_combobox;
    add_treeview = window->priv->config_layouts_add_treeview;
    combo_model = gtk_combo_box_get_model (GTK_COMBO_BOX (combobox));
    remove_treeview = window->priv->config_layouts_remove_treeview;
    remove_model = gtk_tree_view_get_model (GTK_TREE_VIEW (remove_treeview));
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (add_treeview));

    if (!gtk_tree_selection_get_selected (selection, &add_model, &iter)) {
        return;
    }

    gtk_tree_model_get (add_model, &iter,
                        LAYOUT_LAYOUT_NAME_COL,
                        &layout_name,
                        LAYOUT_LAYOUT_DESC_COL,
                        &layout_desc,
                        LAYOUT_VARIANT_NAME_COL,
                        &variant_name,
                        LAYOUT_VARIANT_DESC_COL,
                        &variant_desc,
                        -1);
    config_layouts_list_remove_iter (GTK_LIST_STORE (add_model), &iter);
    config_layouts_list_append_layout (GTK_LIST_STORE (remove_model),
                                       layout_name, layout_desc,
                                       variant_name, variant_desc);
    gtk_tree_store_append (GTK_TREE_STORE (combo_model), &iter, NULL);
    gtk_tree_store_set (GTK_TREE_STORE (combo_model), &iter,
                        LAYOUT_LAYOUT_NAME_COL,
                        g_strdup (layout_name),
                        LAYOUT_LAYOUT_DESC_COL,
                        variant_desc ? g_strdup (variant_desc) :
                            g_strdup (layout_desc),
                        LAYOUT_VARIANT_NAME_COL,
                        g_strdup (variant_name),
                        LAYOUT_VARIANT_DESC_COL, NULL,
                        LAYOUT_VISIBLE_COL, TRUE, -1);
    g_free (layout_name);
    g_free (layout_desc);
    g_free (variant_name);
    g_free (variant_desc);
}

static void
on_button_config_layouts_remove_clicked (GtkButton *button, gpointer data)
{
    InputPadGtkWindow *window;
    gchar **group_layouts = NULL;
    gchar **group_variants = NULL;
    GtkWidget *combobox;
    GtkWidget *add_treeview;
    GtkWidget *remove_treeview;
    GtkTreeModel *combo_model;
    GtkTreeModel *add_model;
    GtkTreeModel *remove_model;
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    gchar *layout_name = NULL;
    gchar *layout_desc = NULL;
    gchar *variant_name = NULL;
    gchar *variant_desc = NULL;
    int i;
    gboolean is_default_layout = FALSE;

    g_return_if_fail (data != NULL &&
                      INPUT_PAD_IS_GTK_WINDOW (data));

    window = INPUT_PAD_GTK_WINDOW (data);
    group_layouts = window->priv->group_layouts;
    group_variants = window->priv->group_variants;
    combobox = window->priv->config_layouts_combobox;
    add_treeview = window->priv->config_layouts_add_treeview;
    remove_treeview = window->priv->config_layouts_remove_treeview;
    combo_model = gtk_combo_box_get_model (GTK_COMBO_BOX (combobox));
    add_model = gtk_tree_view_get_model (GTK_TREE_VIEW (add_treeview));
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (remove_treeview));

    if (!gtk_tree_selection_get_selected (selection, &remove_model, &iter)) {
        return;
    }

    gtk_tree_model_get (remove_model, &iter,
                        LAYOUT_LAYOUT_NAME_COL,
                        &layout_name,
                        LAYOUT_LAYOUT_DESC_COL,
                        &layout_desc,
                        LAYOUT_VARIANT_NAME_COL,
                        &variant_name,
                        LAYOUT_VARIANT_DESC_COL,
                        &variant_desc,
                        -1);

    for (i = 0; group_layouts[i]; i++) {
        if (!g_strcmp0 (group_layouts[i], layout_name)) {
            if (group_variants == NULL ||
                i >= g_strv_length (group_variants) ||
                group_variants[i] == NULL ||
                *group_variants[i] == '\0') {
                if (variant_name == NULL) {
                    is_default_layout = TRUE;
                    break;
                }
            } else if (!g_strcmp0 (group_variants[i], variant_name)) {
                is_default_layout = TRUE;
                break;
            }
        }
    }

    if (!is_default_layout) {
        config_layouts_list_remove_iter (GTK_LIST_STORE (remove_model), &iter);
        config_layouts_list_append_layout (GTK_LIST_STORE (add_model),
                                           layout_name, layout_desc,
                                           variant_name, variant_desc);
        config_layouts_combobox_remove_layout (GTK_TREE_STORE (combo_model),
                                               layout_name, layout_desc,
                                               variant_name, variant_desc);
    }

    g_free (layout_name);
    g_free (layout_desc);
    g_free (variant_name);
    g_free (variant_desc);
}

static void
on_button_config_options_close_clicked (GtkButton *button, gpointer data)
{
    InputPadGtkWindow *window;
    GList *orig_list, *orig_list_button;
    GList *list = NULL;
    GList *list_button = NULL;
    GtkWidget *expander;
    GtkWidget *vbox;
    GtkWidget *checkbutton;
    GtkWidget *combobox;
    GtkTreeIter iter;
    GtkTreeModel *model;
    const gchar *text;
    gchar *layout = NULL;
    gchar *variant = NULL;
    gchar *option = NULL;

    g_return_if_fail (data != NULL &&
                      INPUT_PAD_IS_GTK_WINDOW (data));

    window = INPUT_PAD_GTK_WINDOW (data);

    gtk_widget_hide (window->priv->config_options_dialog);

    list = gtk_container_get_children (GTK_CONTAINER (window->priv->config_options_vbox));
    orig_list = list;
    while (list) {
        GList *tmp_list;

        expander = GTK_WIDGET (list->data);

        tmp_list = gtk_container_get_children (GTK_CONTAINER (expander));
        vbox = GTK_WIDGET (tmp_list->data);
        g_list_free (tmp_list);

        list_button = gtk_container_get_children (GTK_CONTAINER (vbox));
        orig_list_button = list_button;
        while (list_button) {
            checkbutton = GTK_WIDGET (list_button->data);
            text = (gchar *) g_object_get_data (G_OBJECT (checkbutton), "option");
            if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbutton))) {
                if (option == NULL) {
                    option = g_strdup (text);
                } else {
                    gchar *p = g_strdup_printf ("%s,%s", option, text);
                    g_free (option);
                    option = p;
                }
            }
            list_button = list_button->next;
        }
        g_list_free (orig_list_button);
        list = list->next;
    }
    g_list_free (orig_list);

    combobox = window->priv->config_layouts_combobox;
    if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combobox), &iter)) {
        return;
    }
    model = gtk_combo_box_get_model (GTK_COMBO_BOX (combobox));
    gtk_tree_model_get (model, &iter,
                        LAYOUT_LAYOUT_NAME_COL, &layout,
                        LAYOUT_VARIANT_NAME_COL, &variant, -1);
    if (option) {
        g_strfreev (window->priv->group_options);
        window->priv->group_options = g_strsplit (option, ",", -1);
    }
    if (layout) {
        input_pad_gdk_xkb_set_layout (window, window->priv->xkb_key_list,
                                      layout, variant, option);
    }
    g_free (layout);
    g_free (variant);
    g_free (option);
}

static void
on_button_pressed (GtkButton *button, gpointer data)
{
    InputPadGtkWindow *window;
    InputPadGtkButton *ibutton;
    InputPadTableType  type;
    const char *str;
    const char *rawtext;
    char *command_output = NULL;
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
    str = input_pad_gtk_button_get_label (ibutton);
    rawtext = input_pad_gtk_button_get_rawtext (ibutton);
    type = input_pad_gtk_button_get_table_type (ibutton);
    keycode  = input_pad_gtk_button_get_keycode (ibutton);
    keysym = input_pad_gtk_button_get_keysym (ibutton);
    keysyms = input_pad_gtk_button_get_all_keysyms (ibutton);
    group = input_pad_gtk_button_get_keysym_group (ibutton);
    state = window->priv->keyboard_state;
    if (!g_strcmp0 (str, " ") &&
        keysym == (guint) '\t' && keycode == 0 && keysyms == NULL) {
        str = "\t";
        keysym = 0;
    } else if (type == INPUT_PAD_TABLE_TYPE_COMMANDS) {
        run_command (rawtext, &command_output);
        if (command_output == NULL) {
            return;
        }
        str = command_output;
    } else if (rawtext) {
        str = rawtext;
    }
    if (keysyms && (keysym != keysyms[group][0])) {
        state |= ShiftMask;
    }
    state = input_pad_xkb_build_core_state (state, group);

    g_signal_emit (window, signals[BUTTON_PRESSED], 0,
                   str, type, keysym, keycode, state, &retval);

    if (type == INPUT_PAD_TABLE_TYPE_COMMANDS) {
        g_free (command_output);
    }
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
on_button_pressed_repeat (InputPadGtkButton *button, gpointer data)
{
    guint keysym;

    g_return_if_fail (INPUT_PAD_IS_GTK_BUTTON (button));

    keysym = input_pad_gtk_button_get_keysym (button);
    if ((keysym == XK_Control_L) || (keysym == XK_Control_R) ||
        (keysym == XK_Alt_L) || (keysym == XK_Alt_L) ||
        (keysym == XK_Shift_L) || (keysym == XK_Shift_R) ||
        (keysym == XK_Num_Lock) ||
        FALSE) {
        return;
    }
    on_button_pressed (GTK_BUTTON (button), data);
}

static void
on_button_layout_arrow_pressed (GtkButton *button, gpointer data)
{
    KeyboardLayoutPart *table_data = (KeyboardLayoutPart *) data;
    int i;
    int reduced_width = 0;
    int width, height;
    const gchar *prev_label;
    gboolean extend = FALSE;
    GtkWidget *toplevel;

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
            reduced_width += gtk_widget_get_allocated_width (table_data[i].table);
        }
    }

    if (!extend && reduced_width > 0) {
        toplevel = gtk_widget_get_toplevel (table_data[0].table);
        g_return_if_fail (GTK_IS_WINDOW (toplevel));
        gtk_window_get_size (GTK_WINDOW (toplevel), &width, &height);
        if (width > reduced_width) {
            width -= reduced_width;
            gtk_window_resize (GTK_WINDOW (toplevel), width, height);
            gtk_widget_queue_resize (toplevel);
        }
    }
}

static void
on_checkbutton_config_options_option_clicked (GtkButton *button, gpointer data)
{
    GtkWidget *expander;
    GtkWidget *label;
    int checked;
    gchar *text;

    g_return_if_fail (GTK_IS_EXPANDER (data));
    expander = GTK_WIDGET (data);
    label = gtk_expander_get_label_widget (GTK_EXPANDER (expander));
    checked = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (expander), "checked"));
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button))) {
        text = g_strdup_printf ("<b>%s</b>",
                                gtk_label_get_text (GTK_LABEL (label)));
        gtk_label_set_markup (GTK_LABEL (label), text);
        checked++;
    } else {
        checked--;
        if (checked <= 0) {
            text = g_strdup (gtk_label_get_text (GTK_LABEL (label)));
            gtk_label_set_text (GTK_LABEL (label), text);
            g_free (text);
        }
    }
    g_object_set_data (G_OBJECT (expander), "checked",
                       GINT_TO_POINTER (checked));
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
    cp_data->codepoint = code;
    char_label_set_code_point (cp_data->char_label, code);
}

static void
on_tree_view_select_custom_char_group (GtkTreeSelection     *selection,
                                       gpointer              data)
{
    CharTreeViewData *tv_data = (CharTreeViewData *) data;
    InputPadGtkWindow *window;
    InputPadGroup *group;
    GtkWidget *sub_tv;
    GtkTreeModel *model;
    GtkTreeModel *sub_model;
    GtkTreeIter iter;
    GtkTreeSelection *sub_selection;
    int n;

    g_return_if_fail (INPUT_PAD_IS_GTK_WINDOW (tv_data->window));
    window = INPUT_PAD_GTK_WINDOW (tv_data->window);
    g_return_if_fail (window->priv != NULL && window->priv->group != NULL);
    g_return_if_fail (GTK_IS_TREE_VIEW (tv_data->sub_tv));

    group = window->priv->group;
    sub_tv = GTK_WIDGET (tv_data->sub_tv);

    if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
        g_warning ("Main treeview is not selected.");
        return;
    }
    gtk_tree_model_get (model, &iter,
                        CHAR_BLOCK_START_COL, &n, -1);
    group = get_nth_pad_group (group, n);
    g_return_if_fail (group != NULL);
    sub_model = custom_char_table_model_new (window, group->table);
    g_return_if_fail (sub_model != NULL);
    gtk_widget_hide (sub_tv);
    gtk_tree_view_set_model (GTK_TREE_VIEW (sub_tv), sub_model);
    if (gtk_tree_model_get_iter_first (sub_model, &iter)) {
        sub_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (sub_tv));
        gtk_tree_selection_select_iter (sub_selection, &iter);
    }
    gtk_widget_show (sub_tv);
}

static void
on_tree_view_select_custom_char_table (GtkTreeSelection     *selection,
                                       gpointer              data)
{
    CharTreeViewData *tv_data = (CharTreeViewData *) data;
    InputPadGtkWindow *window;
    InputPadGroup *group;
    InputPadTable *table;
    GtkWidget *main_tv;
    GtkWidget *scrolled;
    GtkTreeSelection *main_selection;
    GtkTreeModel *main_model;
    GtkTreeModel *sub_model;
    GtkTreeIter main_iter;
    GtkTreeIter sub_iter;
    int n;

    g_return_if_fail (INPUT_PAD_IS_GTK_WINDOW (tv_data->window));
    window = INPUT_PAD_GTK_WINDOW (tv_data->window);
    g_return_if_fail (window->priv != NULL && window->priv->group != NULL);
    g_return_if_fail (GTK_IS_TREE_VIEW (tv_data->main_tv));
    g_return_if_fail (GTK_IS_SCROLLED_WINDOW (tv_data->scrolled));

    group = window->priv->group;
    main_tv = GTK_WIDGET (tv_data->main_tv);
    scrolled = GTK_WIDGET (tv_data->scrolled);
    main_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (main_tv));

    if (!gtk_tree_selection_get_selected (main_selection, &main_model, &main_iter)) {
        g_warning ("Main treeview is not selected.");
        return;
    }
    if (!gtk_tree_selection_get_selected (selection, &sub_model, &sub_iter)) {
        /* gtk_tree_view_set_model() in main model causes this. */
        return;
    }
    gtk_tree_model_get (main_model, &main_iter,
                        CHAR_BLOCK_START_COL, &n, -1);
    group = get_nth_pad_group (group, n);
    g_return_if_fail (group != NULL);
    gtk_tree_model_get (sub_model, &sub_iter,
                        CHAR_BLOCK_START_COL, &n, -1);
    table = get_nth_pad_table (group->table, n);
    g_return_if_fail (table != NULL && table->priv != NULL);
    table->priv->signal_window = window;
    destroy_custom_char_view_table (scrolled, window);
    append_custom_char_view_table (scrolled, table);
}

static void
on_tree_view_select_all_char (GtkTreeSelection     *selection,
                              gpointer              data)
{
    CharTreeViewData *tv_data = (CharTreeViewData*) data;
    GtkWidget *scrolled;
    GtkWidget *window;
    GtkTreeModel *model;
    GtkTreeIter iter;
    unsigned int start, end;

    g_return_if_fail (data != NULL);

    scrolled = GTK_WIDGET (tv_data->scrolled);
    window = GTK_WIDGET (tv_data->window);
    if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
        return;
    }
    gtk_tree_model_get (model, &iter,
                        CHAR_BLOCK_START_COL, &start,
                        CHAR_BLOCK_END_COL, &end, -1);
    destroy_all_char_view_table (scrolled, INPUT_PAD_GTK_WINDOW (window));
    append_all_char_view_table (scrolled, start, end, window);
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
    if (input_pad->priv->kbdui_name && input_pad->priv->xkb_key_list == NULL) {
        return;
    }

    create_keyboard_layout_ui_real (keyboard_vbox, input_pad);
    input_pad->priv->group_layouts =
        input_pad_gdk_xkb_get_group_layouts (input_pad,
                                             input_pad->priv->xkb_key_list);
    input_pad->priv->group_variants =
        input_pad_gdk_xkb_get_group_variants (input_pad,
                                              input_pad->priv->xkb_key_list);
    input_pad->priv->group_options =
        input_pad_gdk_xkb_get_group_options (input_pad,
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

static void
resize_toplevel_window_with_hide_widget (GtkWidget *widget)
{
    GtkWidget *toplevel;
    GtkAllocation allocation;
    int width, height;

    toplevel = gtk_widget_get_toplevel (widget);
    g_return_if_fail (GTK_IS_WINDOW (toplevel));
    gtk_window_get_size (GTK_WINDOW (toplevel), &width, &height);
    gtk_widget_get_allocation (widget, &allocation);
    if (height > allocation.height &&
        allocation.x >= 0) {
        height -= allocation.height;
        gtk_window_resize (GTK_WINDOW (toplevel), width, height);
        gtk_widget_queue_resize (toplevel);
    }
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
    GList *orig_list;
    GList *list = NULL;
    GtkComboBox *combobox;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GString *string = NULL;
    gchar *line = NULL;
    guint base = 0;
    guint code;

    orig_list = list = gtk_container_get_children (GTK_CONTAINER (digit_hbox));
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
    g_list_free (orig_list);
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
    g_list_free (orig_list);
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
digit_model_new (int base)
{
    GtkTreeStore *store;
    GtkTreeIter   iter;
    int i = 0;
    gchar *str = NULL;

    store = gtk_tree_store_new (DIGIT_N_COLS, G_TYPE_STRING, G_TYPE_BOOLEAN);
    for (i = 0; i < base; i ++) {
        str = g_strdup_printf ("%x", i);
        gtk_tree_store_append (store, &iter, NULL);
        gtk_tree_store_set (store, &iter, DIGIT_TEXT_COL, str,
                            DIGIT_VISIBLE_COL, TRUE, -1);
    }
    return GTK_TREE_MODEL (store);
}

static GtkTreeModel *
digit_model_renew (GtkTreeModel *model, int prev_base, int new_base)
{
    GtkTreeStore *store;
    GtkTreeIter   iter;
    int i = 0;
    gchar *str = NULL;

    g_return_val_if_fail (GTK_IS_TREE_MODEL (model), model);

    if (!gtk_tree_model_get_iter_first (model, &iter)) {
        g_warning ("Could not get the first iter");
        return model;
    }
    if (prev_base > new_base) {
        for (i = 0; i < new_base; i++) {
            if (!gtk_tree_model_iter_next (model, &iter)) {
                g_warning ("Could not get the %dth iter", i);
                return model;
            }
        }
        for (i = new_base; i < prev_base; i++) {
            store = GTK_TREE_STORE (model);
            gtk_tree_store_remove (store, &iter);
        }
    }
    if (new_base >= prev_base) {
        for (i = prev_base; i < new_base; i++) {
            store = GTK_TREE_STORE (model);
            str = g_strdup_printf ("%x", i);
            gtk_tree_store_append (store, &iter, NULL);
            gtk_tree_store_set (store, &iter, DIGIT_TEXT_COL, str,
                                DIGIT_VISIBLE_COL, TRUE, -1);
        }
    }
    return model;
}

static void
init_spin_button (GtkWidget     *down_button,
                  GtkWidget     *up_button,
                  CodePointData *cp_data)
{
    GtkWidget *image;

    cp_data->codepoint = 0;

    image = gtk_image_new_from_icon_name ("list-remove-symbolic",
                                          GTK_ICON_SIZE_MENU);
    g_object_set (G_OBJECT (down_button), "image", image, NULL);

    image = gtk_image_new_from_icon_name ("list-add-symbolic",
                                          GTK_ICON_SIZE_MENU);
    g_object_set (G_OBJECT (up_button), "image", image, NULL);

    g_signal_connect (G_OBJECT (down_button), "pressed",
                      G_CALLBACK (on_button_down_pressed),
                      cp_data);
    g_signal_connect (G_OBJECT (down_button), "pressed-repeat",
                      G_CALLBACK (on_button_down_pressed),
                      cp_data);
    g_signal_connect (G_OBJECT (up_button), "pressed",
                      G_CALLBACK (on_button_up_pressed),
                      cp_data);
    g_signal_connect (G_OBJECT (up_button), "pressed-repeat",
                      G_CALLBACK (on_button_up_pressed),
                      cp_data);
}

static int
get_max_digits_from_base (int base)
{
    char *formatted = NULL;
    int n_digit;

    switch (base) {
    case 16:
        formatted = g_strdup_printf ("%x", MAX_UCODE);
        n_digit = strlen (formatted);
        break;
    case 10:
        formatted = g_strdup_printf ("%u", MAX_UCODE);
        n_digit = strlen (formatted);
        break;
    case 8:
        formatted = g_strdup_printf ("%o", MAX_UCODE);
        n_digit = strlen (formatted);
        break;
    case 2:
        formatted = g_strdup_printf ("%x", MAX_UCODE);
        n_digit = strlen (formatted) * 4;
        break;
    default:
        g_warning ("Base %d is not supported", base);
        return 0;
    }
    g_free (formatted);
    return n_digit;
}

static void
set_code_point_base (CodePointData *cp_data, int n_encoding)
{
    int i, n_digit, created_num, prev_base;
    guint code = 0;
    gboolean do_resize = FALSE;
    GList *orig_list;
    GList *list = NULL;
    GtkWidget *digit_hbox;
    GtkWidget *char_label;
    GtkWidget *combobox;
    GtkWidget *toplevel;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkCellRenderer *renderer;
    GtkAllocation combobox_allocation;
    GtkAllocation toplevel_allocation;

    g_return_if_fail (cp_data != NULL);
    g_return_if_fail (GTK_IS_CONTAINER (cp_data->digit_hbox));
    digit_hbox = GTK_WIDGET (cp_data->digit_hbox);
    g_return_if_fail (GTK_IS_LABEL (cp_data->char_label));
    char_label = GTK_WIDGET (cp_data->char_label);

    if ((n_digit = get_max_digits_from_base (n_encoding)) == 0) {
        return;
    }
    orig_list = list = gtk_container_get_children (GTK_CONTAINER (digit_hbox));
    created_num = g_list_length (list);
    if (created_num > 0) {
        code = digit_hbox_get_code_point (digit_hbox);
        combobox = GTK_WIDGET (list->data);
        prev_base = input_pad_gtk_combo_box_get_base (INPUT_PAD_GTK_COMBO_BOX (combobox));
        if (n_digit < get_max_digits_from_base (prev_base)) {
            do_resize = TRUE;
        }
    }
    if (created_num >= n_digit) {
        for (i = 0; i < n_digit; i++) {
            g_return_if_fail (INPUT_PAD_IS_GTK_COMBO_BOX (list->data));
            combobox = GTK_WIDGET (list->data);
            prev_base = input_pad_gtk_combo_box_get_base (INPUT_PAD_GTK_COMBO_BOX (combobox));
            model = gtk_combo_box_get_model (GTK_COMBO_BOX (combobox));
            if (gtk_tree_model_get_iter_first (model, &iter)) {
                gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combobox), &iter);
            }
            model = digit_model_renew (model, prev_base, n_encoding);
            gtk_combo_box_set_model (GTK_COMBO_BOX (combobox), model);
            input_pad_gtk_combo_box_set_base (INPUT_PAD_GTK_COMBO_BOX (combobox),
                                              n_encoding);
            gtk_widget_show (GTK_WIDGET (combobox));
            list = list->next;
        }
        while (list) {
            gtk_widget_hide (GTK_WIDGET (list->data));
            list = list->next;
        }
        goto end_configure_combobox;
    }

    for (i = 0; i < created_num; i++) {
        g_return_if_fail (INPUT_PAD_IS_GTK_COMBO_BOX (list->data));
        combobox = GTK_WIDGET (list->data);
        prev_base = input_pad_gtk_combo_box_get_base (INPUT_PAD_GTK_COMBO_BOX (combobox));
        model = gtk_combo_box_get_model (GTK_COMBO_BOX (combobox));
        if (gtk_tree_model_get_iter_first (model, &iter)) {
            gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combobox), &iter);
        }
        model = digit_model_renew (model, prev_base, n_encoding);
        gtk_combo_box_set_model (GTK_COMBO_BOX (combobox), model);
        input_pad_gtk_combo_box_set_base (INPUT_PAD_GTK_COMBO_BOX (combobox),
                                          n_encoding);
        gtk_widget_show (GTK_WIDGET (combobox));
        list = list->next;
    }

    for (i = created_num; i < n_digit; i++) {
        combobox = input_pad_gtk_combo_box_new ();
        input_pad_gtk_combo_box_set_base (INPUT_PAD_GTK_COMBO_BOX (combobox),
                                          n_encoding);
        gtk_box_pack_start (GTK_BOX (digit_hbox), combobox, FALSE, FALSE, 0);
        model = digit_model_new (n_encoding);
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
end_configure_combobox:
    g_list_free (orig_list);
    orig_list = list = gtk_container_get_children (GTK_CONTAINER (digit_hbox));
    combobox = GTK_WIDGET (g_list_nth (list, n_digit - 1)->data);
    toplevel = gtk_widget_get_toplevel (digit_hbox);
    gtk_widget_get_allocation (combobox, &combobox_allocation);
    gtk_widget_get_allocation (toplevel, &toplevel_allocation);
    if (do_resize &&
        combobox_allocation.x >= 0 && combobox_allocation.width > 10) {
        toplevel_allocation.width = combobox_allocation.x +
                                    combobox_allocation.width + 10;
        gtk_widget_set_allocation (toplevel, &toplevel_allocation);
        gtk_widget_queue_resize (toplevel);
    }
    digit_hbox_set_code_point (digit_hbox, code);
    char_label_set_code_point (char_label, code);
    g_list_free (orig_list);
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

char **
string_table_get_label_array (InputPadTableStr *strs)
{
    int i = 0, len;
    char **retval = NULL;

    if (strs == NULL) {
        return NULL;
    }
    while (strs[i].label) {
        i++;
    }
    len = i;
    retval = g_new0 (char *, len + 1);
    for (i = 0; strs[i].label; i++) {
        retval[i] = g_strdup (strs[i].label);
    }

    return retval;
}

char **
command_table_get_label_array (InputPadTableCmd *cmds)
{
    int i = 0, len;
    char **retval = NULL;

    if (cmds == NULL) {
        return NULL;
    }
    while (cmds[i].execl) {
        i++;
    }
    len = i;
    retval = g_new0 (char *, len + 1);
    for (i = 0; cmds[i].execl; i++) {
        if (cmds[i].label) {
            retval[i] = g_strdup (cmds[i].label);
        } else {
            retval[i] = g_strdup (cmds[i].execl);
        }
    }

    return retval;
}

static void
run_command (const gchar *command, gchar **command_output)
{
    GError  *error;
    char   **argv;
    char   **envp = NULL;
    char    *p;
    char    *end;
    char    *name;
    const char *value;
    char    *extracted;
    char    *new_arg;
    char    *std_output = NULL;
    char    *std_error = NULL;
    int      i, n, exit_status;

    error = NULL;
    if (!g_shell_parse_argv (command, NULL, &argv, &error)) {
        g_warning ("Could not parse command: %s", error->message);
        g_error_free (error);
        return;
    }
    n = 0;
    for (i = 0; argv[i]; i++) {
        if (g_strstr_len (argv[i], -1, "=") != NULL) {
            n++;
        } else {
            break;
        }
    }
    if (n > 0) {
        envp = g_new0 (char *, n + 1);
    }
    for (i = 0; i < n; i++) {
        envp[i] = g_strdup (argv[i]);
    }
    for (i = n; argv[i]; i++) {
        if ((p = g_strstr_len (argv[i], -1, "$")) != NULL &&
            *(p + 1) != '\0') {
            end = g_strstr_len (p + 1, -1, " ");
            if (end) {
                name = g_strndup (p, end - p);
            } else {
                name = g_strdup (p);
            }
            value = g_getenv (name + 1);
            g_free (name);
            if (value != NULL) {
                extracted = g_strdup (value);
            } else {
                extracted = g_strdup ("");
            }
            *p = '\0';
            if (end) {
                new_arg = g_strconcat (argv[i], extracted, end, NULL);
            } else {
                new_arg = g_strconcat (argv[i], extracted, NULL);
            }
            g_free (argv[i]);
            argv[i] = new_arg;
        }
    }
    error = NULL;
    if (!g_spawn_sync (NULL,
                       &argv[n],
                       envp,
                       G_SPAWN_SEARCH_PATH,
                       NULL, NULL,
                       &std_output, &std_error,
                       &exit_status, &error)) {
        g_warning ("Could not run the script %s: %s: %s",
                   command,
                   error->message,
                   std_error ? std_error : "");
        g_error_free (error);
        g_strfreev (argv);
        return;
    }
    g_strfreev (argv);
    g_strfreev (envp);
    if (exit_status != 0) {
        g_warning ("Failed to run the script %s: %s", command,
                   std_error ? std_error : "");
    }
    if (std_output && strlen (std_output) > 2) {
        if (command_output) {
            *command_output = g_strdup (g_strchomp (std_output));
        }
    }
    g_free (std_output);
    g_free (std_error);
}

static void
append_unicode_table (GtkWidget         *table,
                      unsigned           int min,
                      unsigned           int start,
                      unsigned           int end,
                      InputPadGtkWindow *input_pad)
{
    GtkCssProvider *css_provider;
    GtkStyleContext *style_context;
    GtkWidget *button;
    GError *error = NULL;
    unsigned int num;
    int row, col;

    css_provider = gtk_css_provider_new ();
    if (input_pad->child) {
        gtk_css_provider_load_from_data (
                css_provider,
                CSS_DATA_NARROW_BUTTON,
                -1,
                &error);
    } else {
        /* FIXME: cannot change background in GTK3 button. */
        gtk_css_provider_load_from_data (
                css_provider,
                CSS_DATA_GRAY_NARROW_BUTTON,
                -1,
                &error);
    }

    for (num = start; num <= end; num++) {
        button = input_pad_gtk_button_new_with_unicode (num);
        row = (num - min) / INPUT_PAD_MAX_COLUMN;
        col = (num - min) % INPUT_PAD_MAX_COLUMN;
        style_context = gtk_widget_get_style_context (button);
        gtk_style_context_add_provider (style_context,
                                        GTK_STYLE_PROVIDER (css_provider),
                                        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        gtk_grid_attach (GTK_GRID (table), button,
                         col, row, 1, 1);
        gtk_widget_show (button);
        if (input_pad->child)
            gtk_widget_set_sensitive (button,
                                      input_pad->priv->char_button_sensitive);
        g_signal_connect (G_OBJECT (button), "pressed",
                          G_CALLBACK (on_button_pressed),
                          input_pad);
        g_signal_connect (G_OBJECT (button), "pressed-repeat",
                          G_CALLBACK (on_button_pressed_repeat),
                          input_pad);
        g_signal_connect (G_OBJECT (input_pad),
                          "char-button-sensitive",
                          G_CALLBACK (on_window_char_button_sensitive),
                          (gpointer) button);
    }

    g_object_unref (css_provider);
}

static void
table_element_remove (GtkWidget *button, gpointer data)
{
    TableForEachData *foreach_data = (TableForEachData*) data;
    GtkWidget *table = foreach_data->table;
    InputPadGtkWindow *window = foreach_data->window;

    g_signal_handlers_disconnect_by_func (G_OBJECT (window),
                                          G_CALLBACK (on_window_char_button_sensitive),
                                          (gpointer) button);
    g_signal_handlers_disconnect_by_func (G_OBJECT (button),
                                          G_CALLBACK (on_button_pressed),
                                          (gpointer) window);
    g_signal_handlers_disconnect_by_func (G_OBJECT (button),
                                          G_CALLBACK (on_button_pressed_repeat),
                                          (gpointer) window);
    gtk_container_remove (GTK_CONTAINER (table), button);
}

static void
destroy_char_view_table_common (GtkWidget *scrolled, InputPadGtkWindow *window)
{
    GList *scrolled_list, *viewport_list;
    GtkWidget *viewport;
    GtkWidget *table;
    static TableForEachData foreach_data = { NULL, };

    scrolled_list = gtk_container_get_children (GTK_CONTAINER (scrolled));
    if (scrolled_list == NULL) {
        return;
    }
    viewport = scrolled_list->data;
    g_return_if_fail (GTK_IS_VIEWPORT (viewport) ||
                      INPUT_PAD_IS_GTK_VIEWPORT (viewport));
    viewport_list = gtk_container_get_children (GTK_CONTAINER (viewport));
    if (viewport_list == NULL) {
        return;
    }
    table = viewport_list->data;
    g_return_if_fail (GTK_IS_GRID (table));
    foreach_data.table = table;
    foreach_data.window = window;
    gtk_container_foreach (GTK_CONTAINER (table),
                           table_element_remove, &foreach_data);
    g_list_free (viewport_list);
    g_list_free (scrolled_list);
    gtk_container_remove (GTK_CONTAINER (viewport), table);
    gtk_container_remove (GTK_CONTAINER (scrolled), viewport);
}

static void
append_custom_char_view_table (GtkWidget *scrolled, InputPadTable *table_data)
{
    InputPadGtkWindow *input_pad;
    GtkCssProvider *css_provider;
    GtkStyleContext *style_context;
    GtkWidget *table;
    GtkWidget *button = NULL;
    GError *error = NULL;
    gchar **char_table;
    gchar *str;
    const int max_column = table_data->column;
    int i, num, row, col, len, code;
    guint keysym;
#if 0
    guint **keysyms;
    InputPadXKBKeyList *xkb_key_list = NULL;
#endif

    g_return_if_fail (INPUT_PAD_IS_GTK_WINDOW (table_data->priv->signal_window));

    input_pad = INPUT_PAD_GTK_WINDOW (table_data->priv->signal_window);

    if (table_data->type == INPUT_PAD_TABLE_TYPE_CHARS) {
        char_table = g_strsplit_set (table_data->data.chars, " \t\n", -1);
    } else if (table_data->type == INPUT_PAD_TABLE_TYPE_KEYSYMS) {
        char_table = g_strsplit_set (table_data->data.keysyms, " \t\n", -1);
    } else if (table_data->type == INPUT_PAD_TABLE_TYPE_STRINGS) {
        char_table = string_table_get_label_array (table_data->data.strs);
    } else if (table_data->type == INPUT_PAD_TABLE_TYPE_COMMANDS) {
        char_table = command_table_get_label_array (table_data->data.cmds);
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
    col = max_column;
    row = num / col;
    if (num % col) {
        row++;
    }

#if 0
    if (INPUT_PAD_IS_GTK_WINDOW (table_data->priv->signal_window)) {
        xkb_key_list =
            INPUT_PAD_GTK_WINDOW (table_data->priv->signal_window)->priv->xkb_key_list;
    }
#endif

    css_provider = gtk_css_provider_new ();
    if (input_pad->child) {
        gtk_css_provider_load_from_data (
                css_provider,
                CSS_DATA_NARROW_BUTTON,
                -1,
                &error);
    } else {
        /* FIXME: cannot change background in GTK3 button. */
        gtk_css_provider_load_from_data (
                css_provider,
                CSS_DATA_GRAY_NARROW_BUTTON,
                -1,
                &error);
    }

    table = gtk_grid_new ();
    /* gtk_table_attach_defaults assigns GTK_EXPAND and
     * the button is too big when max row is 1 . */
    g_object_set (table,
                  "halign", GTK_ALIGN_START,
                  "valign", GTK_ALIGN_START,
                  "margin", 0,
                  "expand", FALSE,
                  "row-spacing", 0,
                  "column-spacing", 0,
                  "row-homogeneous", TRUE,
                  "column-homogeneous", TRUE,
                  NULL);
#if GTK_CHECK_VERSION (3, 8, 0)
    gtk_container_add (GTK_CONTAINER (scrolled), table);
#else
    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled),
                                           table);
#endif
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
                button = input_pad_gtk_button_new_with_unicode (code);
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
            } else if (table_data->type == INPUT_PAD_TABLE_TYPE_STRINGS) {
                button = input_pad_gtk_button_new_with_label (char_table[i]);
                if (table_data->data.strs[i].rawtext) {
                    gtk_widget_set_tooltip_text (button,
                                                 table_data->data.strs[i].rawtext);
                    input_pad_gtk_button_set_rawtext (INPUT_PAD_GTK_BUTTON (button),
                                                      table_data->data.strs[i].rawtext);
                }
                if (table_data->data.strs[i].comment) {
                    gtk_widget_set_tooltip_text (button,
                                                 table_data->data.strs[i].comment);
                }
            } else if (table_data->type == INPUT_PAD_TABLE_TYPE_COMMANDS) {
                button = input_pad_gtk_button_new_with_label (char_table[i]);
                gtk_widget_set_tooltip_text (button,
                                             table_data->data.cmds[i].execl);
                input_pad_gtk_button_set_rawtext (INPUT_PAD_GTK_BUTTON (button),
                                                  table_data->data.cmds[i].execl);
            }
            input_pad_gtk_button_set_table_type (INPUT_PAD_GTK_BUTTON (button),
                                                 table_data->type);
            row = num / max_column;
            col = num % max_column;
            style_context = gtk_widget_get_style_context (button);
            gtk_style_context_add_provider (style_context,
                                            GTK_STYLE_PROVIDER (css_provider),
                                            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
            gtk_grid_attach (GTK_GRID (table), button,
                             col, row, 1, 1);

            gtk_widget_show (button);
            if (input_pad->child)
                gtk_widget_set_sensitive (button,
                                          input_pad->priv->char_button_sensitive);
            g_signal_connect (G_OBJECT (button), "pressed",
                              G_CALLBACK (on_button_pressed),
                              (gpointer) table_data->priv->signal_window);
            g_signal_connect (G_OBJECT (button), "pressed-repeat",
                              G_CALLBACK (on_button_pressed_repeat),
                              (gpointer) table_data->priv->signal_window);
            g_signal_connect (G_OBJECT (table_data->priv->signal_window),
                              "char-button-sensitive",
                              G_CALLBACK (on_window_char_button_sensitive),
                              (gpointer) button);
            num++;
        }
    }
    g_object_unref (css_provider);
    g_strfreev (char_table);

    table_data->priv->inited = 1;
}

static void
destroy_custom_char_view_table (GtkWidget *scrolled, InputPadGtkWindow *window)
{
    /* Currently same contents. */
    destroy_char_view_table_common (scrolled, window);
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
        g_unichar_validate (ch) &&
        g_unichar_isprint (ch)) {
        buff[g_unichar_to_utf8 (ch, buff)] = '\0';
        display_name = g_strdup (buff);
    } else if (g_str_has_prefix (keysym_name, "XF86_Switch_VT_")) {
        display_name = g_strdup_printf ("V%s",
                                        keysym_name + strlen ("XF86_Switch_VT_"));
    } else if (!g_ascii_strncasecmp (keysym_name, "Control_",
                                     strlen ("Control_"))) {
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
create_keyboard_layout_ui_real_default (GtkWidget *vbox, InputPadGtkWindow *window)
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
    GtkCssProvider *css_provider;
    GtkStyleContext *style_context;
    GtkWidget *hbox;
    GtkWidget *table;
    GtkWidget *button;
    GtkWidget *button_shift_l = NULL;
    GtkWidget *button_shift_r = NULL;
    GtkWidget *button_num_lock = NULL;
    GError *error = NULL;
    GList *orig_children, *children = NULL;
    char *tooltip;
    char *display_name;

    g_return_if_fail (xkb_key_list != NULL);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_box_reorder_child (GTK_BOX (vbox), hbox, 0);
    gtk_widget_show (hbox);

    css_provider = gtk_css_provider_new ();
    gtk_css_provider_load_from_data (css_provider,
                                     CSS_DATA_NARROW_BUTTON,
                                     -1,
                                     &error);

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
        table = gtk_grid_new ();
        g_object_set (table,
                      "halign", GTK_ALIGN_START,
                      "valign", GTK_ALIGN_START,
                      "margin", 0,
                      "expand", FALSE,
                      "row-spacing", 0,
                      "column-spacing", 0,
                      "row-homogeneous", TRUE,
                      "column-homogeneous", TRUE,
                      NULL);
#if 1
        if (i == 0)
            /* for arrow label */
            gtk_widget_show (table);
#endif
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
            button = input_pad_gtk_button_new_with_label_size (display_name,
                                                               KEYBOARD_ICON_SIZE);
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
            style_context = gtk_widget_get_style_context (button);
            gtk_style_context_add_provider (style_context,
                                            GTK_STYLE_PROVIDER (css_provider),
                                            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
            gtk_grid_attach (GTK_GRID (table_data[n].table), button,
                             col, row, 1, 1);
            gtk_widget_show (button);
            gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
            g_signal_connect (G_OBJECT (button), "pressed",
                              G_CALLBACK (on_button_pressed),
                              (gpointer) window);
            g_signal_connect (G_OBJECT (button), "pressed-repeat",
                              G_CALLBACK (on_button_pressed_repeat),
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
    orig_children = children;
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
    g_list_free (orig_children);
    children = gtk_container_get_children (GTK_CONTAINER (table_data[2].table));
    orig_children = children;
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
    g_list_free (orig_children);

    button = gtk_button_new_with_label ("->");
    gtk_widget_set_tooltip_text (button, _("Extend layout"));
    style_context = gtk_widget_get_style_context (button);
    gtk_style_context_add_provider (style_context,
                                    GTK_STYLE_PROVIDER (css_provider),
                                    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_grid_attach (GTK_GRID (table_data[0].table), button,
                     table_data[0].col, max_row - 1, 1, 1);
    gtk_widget_show (button);
    g_signal_connect (G_OBJECT (button), "pressed",
                      G_CALLBACK (on_button_layout_arrow_pressed),
                      table_data);

    g_object_unref (css_provider);
}

G_INLINE_FUNC void
create_keyboard_layout_ui_real (GtkWidget         *vbox,
                                InputPadGtkWindow *window)
{
    if (window->priv->kbdui_name && window->priv->kbdui) {
        g_signal_emit_by_name (window->priv->kbdui,
                               "create-keyboard-layout",
                               vbox, window);
        return;
    }
    create_keyboard_layout_ui_real_default (vbox, window);
}

static void
keyboard_element_remove (GtkWidget *button, gpointer data)
{
    TableForEachData *foreach_data = (TableForEachData *) data;
    GtkWidget *table = foreach_data->table;
    InputPadGtkWindow *window = foreach_data->window;

    g_signal_handlers_disconnect_by_func (G_OBJECT (window),
                                          G_CALLBACK (on_window_keyboard_changed),
                                          (gpointer) button);
    gtk_container_remove (GTK_CONTAINER (table), button);
}

static void
keyboard_box_remove (GtkWidget *table, gpointer data)
{
    TableForEachData *parent_foreach_data = (TableForEachData *) data;
    static TableForEachData foreach_data = { NULL, };
    GtkWidget *hbox = parent_foreach_data->table;
    InputPadGtkWindow *window = parent_foreach_data->window;

    foreach_data.table = table;
    foreach_data.window = window;
    gtk_container_foreach (GTK_CONTAINER (table),
                           keyboard_element_remove, &foreach_data);
    gtk_container_remove (GTK_CONTAINER (hbox), table);
}

static void
destroy_prev_keyboard_layout_default (GtkWidget *vbox, InputPadGtkWindow *window)
{
    GList *children = NULL;
    GtkWidget *hbox;
    static TableForEachData foreach_data = { NULL, };

    children = gtk_container_get_children (GTK_CONTAINER (vbox));
    hbox = GTK_WIDGET (children->data);
    g_list_free (children);
    foreach_data.table = hbox;
    foreach_data.window = window;
    gtk_container_foreach (GTK_CONTAINER (hbox),
                           keyboard_box_remove, &foreach_data);
    gtk_container_remove (GTK_CONTAINER (vbox), hbox);
}

G_INLINE_FUNC void
destroy_prev_keyboard_layout (GtkWidget         *vbox,
                              InputPadGtkWindow *window)
{
    if (window->priv->kbdui_name && window->priv->kbdui) {
        g_signal_emit_by_name (window->priv->kbdui,
                               "destroy-keyboard-layout",
                               vbox, window);
        return;
    }
    destroy_prev_keyboard_layout_default (vbox, window);
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

static void
config_layouts_combobox_remove_layout (GtkTreeStore *list,
                                       const gchar *layout_name,
                                       const gchar *layout_desc,
                                       const gchar *variant_name,
                                       const gchar *variant_desc)
{
    GtkTreeIter   iter;

    gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list), &iter);
    do {
        gchar *combo_layout_name = NULL;
        gchar *combo_layout_desc = NULL;
        gchar *combo_variant_name = NULL;
        gchar *combo_variant_desc = NULL;

        gtk_tree_model_get (GTK_TREE_MODEL (list), &iter,
                            LAYOUT_LAYOUT_NAME_COL,
                            &combo_layout_name,
                            LAYOUT_LAYOUT_DESC_COL,
                            &combo_layout_desc,
                            LAYOUT_VARIANT_NAME_COL,
                            &combo_variant_name,
                            LAYOUT_VARIANT_DESC_COL,
                            &combo_variant_desc,
                            -1);
        if (!g_strcmp0 (combo_layout_name, layout_name) &&
            !g_strcmp0 (combo_variant_name, variant_name)) {
            gtk_tree_store_remove (GTK_TREE_STORE (list), &iter);
            g_free (combo_layout_name);
            g_free (combo_layout_desc);
            g_free (combo_variant_name);
            g_free (combo_variant_desc);
            break;
        }
        g_free (combo_layout_name);
        g_free (combo_layout_desc);
        g_free (combo_variant_name);
        g_free (combo_variant_desc);
    } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (list), &iter));
}

static void
config_layouts_list_append_layout (GtkListStore *list,
                                   const gchar *layout_name,
                                   const gchar *layout_desc,
                                   const gchar *variant_name,
                                   const gchar *variant_desc)
{
    GtkTreeIter   iter;

    gtk_list_store_append (list, &iter);
    gtk_list_store_set (list, &iter,
                        LAYOUT_LAYOUT_NAME_COL,
                        layout_name,
                        LAYOUT_LAYOUT_DESC_COL,
                        variant_desc ? variant_desc : layout_desc,
                        LAYOUT_VARIANT_NAME_COL,
                        variant_name,
                        LAYOUT_VARIANT_DESC_COL, NULL,
                        LAYOUT_VISIBLE_COL, TRUE, -1);
}

static void
config_layouts_list_remove_iter (GtkListStore *list,
                                 GtkTreeIter  *iter)
{
    if (!gtk_list_store_remove (list, iter)) {
        /* FIXME: Should set iter at the end again? */
        return;
    }
    if (!gtk_tree_model_iter_previous (GTK_TREE_MODEL (list), iter)) {
        gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list), iter);
    }
}

static void
config_layouts_treeview_set_list (GtkWidget    *treeview,
                                  GtkListStore *list,
                                  gboolean      is_sortable)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    if (is_sortable) {
        gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (list),
                                         LAYOUT_LAYOUT_DESC_COL,
                                         sort_layout_name,
                                         NULL, NULL);
        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (list),
                                              LAYOUT_LAYOUT_DESC_COL,
                                              GTK_SORT_ASCENDING);
    }
    gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (list));
    g_object_unref (G_OBJECT (list));

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Layout"), renderer,
                                                       "text", LAYOUT_LAYOUT_DESC_COL,
                                                       "visible", LAYOUT_VISIBLE_COL,
                                                       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
    gtk_widget_show (treeview);
}

static void
config_layouts_treeview_set (InputPadGtkWindow         *input_pad,
                             InputPadXKBLayoutList     *xkb_layout_list)
{
    InputPadXKBLayoutList *layouts = NULL;
    GtkTreeIter   iter;
    GtkListStore *add_list;
    GtkListStore *remove_list;
    gchar **group_layouts = input_pad->priv->group_layouts;
    gchar **group_variants = input_pad->priv->group_variants;
    int i;

    add_list = gtk_list_store_new (LAYOUT_N_COLS,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_BOOLEAN);
    remove_list = gtk_list_store_new (LAYOUT_N_COLS,
                                      G_TYPE_STRING,
                                      G_TYPE_STRING,
                                      G_TYPE_STRING,
                                      G_TYPE_STRING,
                                      G_TYPE_BOOLEAN);

    for (layouts = xkb_layout_list; layouts; layouts = layouts->next) {
        InputPadXKBVariantList *variants = layouts->variants;

        config_layouts_list_append_layout (add_list,
                                           layouts->layout,
                                           layouts->desc,
                                           NULL,
                                           NULL);
        while (variants) {
            config_layouts_list_append_layout (add_list,
                                               layouts->layout,
                                               layouts->desc,
                                               variants->variant,
                                               variants->desc);
            variants = variants->next;
        }
    }

    gtk_tree_model_get_iter_first (GTK_TREE_MODEL (add_list), &iter);
    do {
        gchar *layout_name = NULL;
        gchar *layout_desc = NULL;
        gchar *variant_name = NULL;
        gchar *variant_desc = NULL;

        gtk_tree_model_get (GTK_TREE_MODEL (add_list), &iter,
                            LAYOUT_LAYOUT_NAME_COL,
                            &layout_name,
                            LAYOUT_LAYOUT_DESC_COL,
                            &layout_desc,
                            LAYOUT_VARIANT_NAME_COL,
                            &variant_name,
                            LAYOUT_VARIANT_DESC_COL,
                            &variant_desc,
                            -1);
        for (i = 0; group_layouts[i]; i++) {
            if (!g_strcmp0 (group_layouts[i], layout_name)) {
                if (group_variants == NULL ||
                    i >= g_strv_length (group_variants) ||
                    group_variants[i] == NULL ||
                    *group_variants[i] == '\0') {
                    if (variant_name == NULL) {
                        config_layouts_list_remove_iter (add_list, &iter);
                        config_layouts_list_append_layout (remove_list,
                                                           layout_name,
                                                           layout_desc,
                                                           variant_name,
                                                           variant_desc);
                        break;
                    }
                } else {
                    if (!g_strcmp0 (group_variants[i], variant_name)) {
                        config_layouts_list_remove_iter (add_list, &iter);
                        config_layouts_list_append_layout (remove_list,
                                                           layout_name,
                                                           layout_desc,
                                                           variant_name,
                                                           variant_desc);
                        break;
                    }
                }
            }
        }
        g_free (layout_name);
        g_free (layout_desc);
        g_free (variant_name);
        g_free (variant_desc);
    } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (add_list), &iter));

    config_layouts_treeview_set_list (input_pad->priv->config_layouts_add_treeview,
                                      add_list, TRUE);
    config_layouts_treeview_set_list (input_pad->priv->config_layouts_remove_treeview,
                                      remove_list, FALSE);
}

static void
config_options_treeview_set (InputPadGtkWindow                 *input_pad,
                             InputPadXKBOptionGroupList        *xkb_group_list)
{
    InputPadXKBOptionGroupList *groups = NULL;
    GtkWidget *expander;
    GtkWidget *vbox;
    GtkWidget *checkbutton;
    GtkWidget *label;
    gchar **group_options = input_pad->priv->group_options;
    int i;

    for (groups = xkb_group_list ; groups; groups = groups->next) {
        int checked = 0;

        expander = gtk_expander_new (groups->desc);
        gtk_box_pack_start (GTK_BOX (input_pad->priv->config_options_vbox),
                            expander, TRUE, TRUE, 0);
        gtk_widget_show (expander);
        g_object_set_data (G_OBJECT (expander), "option_group",
                           (gpointer) groups->option_group);
        g_object_set_data (G_OBJECT (expander), "checked",
                           GINT_TO_POINTER (checked));
        vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
        gtk_widget_set_margin_start (vbox, 18);
        gtk_container_add (GTK_CONTAINER (expander), vbox);
        gtk_widget_show (vbox);

        InputPadXKBOptionList *options = groups->options;
        while (options) {
            gboolean has_option = FALSE;

            checkbutton = gtk_check_button_new_with_label (options->desc);
            g_object_set_data (G_OBJECT (checkbutton), "option",
                               (gpointer) options->option);
            for (i = 0; group_options[i]; i++) {
                if (!g_strcmp0 (group_options[i], options->option)) {
                    has_option = TRUE;
                    break;
                }
            }
            if (has_option) {
                gchar *text = g_strdup_printf ("<b>%s</b>", groups->desc);

                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton),
                                              TRUE);
                label = gtk_expander_get_label_widget (GTK_EXPANDER (expander));
                gtk_label_set_markup (GTK_LABEL (label), text);
                g_free (text);
                g_object_set_data (G_OBJECT (expander), "checked",
                                   GINT_TO_POINTER (++checked));
            }
            g_signal_connect (checkbutton, "toggled",
                              G_CALLBACK (on_checkbutton_config_options_option_clicked),
                              (gpointer) expander);
            gtk_box_pack_start (GTK_BOX (vbox), checkbutton, FALSE, TRUE, 0);
            gtk_widget_show (checkbutton);
            options = options->next;
        }
    }
}

static GtkTreeModel *
layout_model_new (InputPadGtkWindow            *input_pad,
                  InputPadXKBLayoutList        *xkb_layout_list)
{
    InputPadXKBLayoutList *layouts = NULL;
    GtkTreeStore *store;
    GtkTreeIter   iter;
    gchar **group_layouts = input_pad->priv->group_layouts;
    gchar **group_variants = input_pad->priv->group_variants;
    int i;

    store = gtk_tree_store_new (LAYOUT_N_COLS, G_TYPE_STRING, G_TYPE_STRING,
                                G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);
    for (i = 0; group_layouts[i]; i++) {
        const gchar *layout_name = NULL;
        const gchar *layout_desc = NULL;
        const gchar *variant_name = NULL;
        const gchar *variant_desc = NULL;
        gboolean has_layout = FALSE;

        for (layouts = xkb_layout_list ;layouts; layouts = layouts->next) {
            InputPadXKBVariantList *variants = layouts->variants;

            if (!g_strcmp0 (group_layouts[i], layouts->layout)) {
                layout_name = layouts->layout;
                layout_desc = layouts->desc;
                if (group_variants == NULL ||
                    i >= g_strv_length (group_variants) ||
                    group_variants[i] == NULL ||
                    *group_variants[i] == '\0') {
                    has_layout = TRUE;
                    break;
                }
                while (variants) {
                    if (!g_strcmp0 (group_variants[i], variants->variant)) {
                        has_layout = TRUE;
                        variant_name = variants->variant;
                        variant_desc = variants->desc;
                        break;
                    }
                    variants = variants->next;
                }
            }
            if (has_layout) {
                break;
            }
        }
        if (!has_layout) {
            continue;
        }
        gtk_tree_store_append (store, &iter, NULL);
        gtk_tree_store_set (store, &iter,
                            LAYOUT_LAYOUT_NAME_COL,
                            layout_name,
                            LAYOUT_LAYOUT_DESC_COL,
                            variant_desc ? variant_desc :
                                layout_desc,
                            LAYOUT_VARIANT_NAME_COL,
                            variant_name,
                            LAYOUT_VARIANT_DESC_COL, NULL,
                            LAYOUT_VISIBLE_COL, TRUE, -1);
    }

    /* Appended layouts should be last. */
#if 0
    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (store),
                                     LAYOUT_LAYOUT_DESC_COL,
                                     sort_layout_name,
                                     NULL, NULL);
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
                                          LAYOUT_LAYOUT_DESC_COL,
                                          GTK_SORT_ASCENDING);
#endif

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
    GtkWidget *button;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkCellRenderer *renderer;

    g_return_if_fail (xkb_config_reg != NULL);
    layouts = xkb_config_reg->layouts;
    g_return_if_fail (layouts != NULL);
    g_return_if_fail (xkb_config_reg->option_groups != NULL);

    config_layouts_treeview_set (window, layouts);
    config_options_treeview_set (window, xkb_config_reg->option_groups);
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    label = gtk_label_new_with_mnemonic (_("_Layout:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    combobox = gtk_combo_box_new ();
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), combobox);
    gtk_box_pack_start (GTK_BOX (hbox), combobox, FALSE, FALSE, 0);
    model = layout_model_new (window, layouts);
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
    window->priv->config_layouts_combobox = combobox;

    button = gtk_button_new_with_mnemonic (_("_Configure Layouts"));
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
    gtk_widget_show (button);

    g_signal_connect (button, "clicked",
                      G_CALLBACK (on_button_config_layouts_clicked),
                      (gpointer) window->priv->config_layouts_dialog);
}

static void
set_about (GtkWidget *widget)
{
    GtkAboutDialog *about_dlg = GTK_ABOUT_DIALOG (widget);
    gchar *license = NULL;
/* No longer used in gtk 2.20
    gchar *license1, *license2, *license3;
    license1 = _(""
"This library is free software; you can redistribute it and/or "
"modify it under the terms of the GNU Lesser General Public "
"License as published by the Free Software Foundation; either "
"version 2 of the License, or (at your option) any later version.");

    license2 = _(""
"This library is distributed in the hope that it will be useful, "
"but WITHOUT ANY WARRANTY; without even the implied warranty of "
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU "
"Lesser General Public License for more details.");

    license3 = _(""
"You should have received a copy of the GNU Lesser General Public "
"License along with this library; if not, write to the Free Software "
"Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, "
"MA  02110-1301  USA");

    license = g_strdup_printf ("%s\n\n%s\n\n%s",
                               license1, license2, license3);
*/

    /* This format has been used since gtk 2.20. */
    license = g_strdup_printf (_("This program comes with ABSOLUTELY NO WARRANTY; for details, visit %s"),
                               "http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html");
    gtk_about_dialog_set_license (about_dlg, license);
    gtk_about_dialog_set_version (about_dlg, VERSION);
    g_free (license);
}

static GtkTreeModel *
custom_char_group_model_new (InputPadGtkWindow *window)
{
    InputPadGroup *group;
    GtkTreeStore *store;
    GtkTreeIter   iter;
    int i;

    g_return_val_if_fail (INPUT_PAD_IS_GTK_WINDOW (window), NULL);
    g_return_val_if_fail (window->priv != NULL && window->priv->group != NULL, NULL);

    group = window->priv->group;

    store = gtk_tree_store_new (CHAR_BLOCK_N_COLS,
                                G_TYPE_STRING, G_TYPE_STRING,
                                G_TYPE_STRING,
                                G_TYPE_UINT, G_TYPE_UINT,
                                G_TYPE_BOOLEAN);
    for (i = 0; group; i++) {
        gtk_tree_store_append (store, &iter, NULL);
        gtk_tree_store_set (store, &iter,
                            CHAR_BLOCK_LABEL_COL,
                            group->name,
                            CHAR_BLOCK_UNICODE_COL, NULL,
                            CHAR_BLOCK_UTF8_COL, NULL,
                            CHAR_BLOCK_START_COL, i,
                            CHAR_BLOCK_END_COL, 0,
                            CHAR_BLOCK_VISIBLE_COL, TRUE,
                            -1);
        group = group->next;
    }
    return GTK_TREE_MODEL (store);
}

static GtkTreeModel *
custom_char_table_model_new (InputPadGtkWindow *window, InputPadTable *table)
{
    GtkTreeStore *store;
    GtkTreeIter   iter;
    int i;

    g_return_val_if_fail (INPUT_PAD_IS_GTK_WINDOW (window), NULL);
    g_return_val_if_fail (table != NULL, NULL);

    store = gtk_tree_store_new (CHAR_BLOCK_N_COLS,
                                G_TYPE_STRING, G_TYPE_STRING,
                                G_TYPE_STRING,
                                G_TYPE_UINT, G_TYPE_UINT,
                                G_TYPE_BOOLEAN);
    for (i = 0; table; i++) {
        gtk_tree_store_append (store, &iter, NULL);
        gtk_tree_store_set (store, &iter,
                            CHAR_BLOCK_LABEL_COL,
                            table->name,
                            CHAR_BLOCK_UNICODE_COL, NULL,
                            CHAR_BLOCK_UTF8_COL, NULL,
                            CHAR_BLOCK_START_COL, i,
                            CHAR_BLOCK_END_COL, 0,
                            CHAR_BLOCK_VISIBLE_COL, TRUE,
                            -1);
        table = table->next;
    }
    return GTK_TREE_MODEL (store);
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
all_char_table_model_new (void)
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

    about_dlg = GTK_WIDGET (gtk_builder_get_object (builder, "AboutDialog"));
    set_about (about_dlg);
}

static void
create_code_point_dialog_ui (GtkBuilder *builder, GtkWidget *window)
{
    GtkWidget *cp_dlg;
    GtkWidget *digit_hbox;
    GtkWidget *char_label;
    GtkWidget *encoding_hbox;
    GtkWidget *base_hbox;
    GtkWidget *down_button;
    GtkWidget *up_button;
    GtkWidget *send_button;
    GtkWidget *close_button;
    GList *orig_list;
    GList *list = NULL;
    static CodePointData cp_data = {NULL, NULL};

    cp_dlg = GTK_WIDGET (gtk_builder_get_object (builder, "CodePointDialog"));

    digit_hbox = GTK_WIDGET (gtk_builder_get_object (builder, "CodePointDigitHBox"));
    char_label = GTK_WIDGET (gtk_builder_get_object (builder, "CodePointCharLabel"));
    down_button = GTK_WIDGET (gtk_builder_get_object (builder, "CodePointDownButton"));
    up_button = GTK_WIDGET (gtk_builder_get_object (builder, "CodePointUpButton"));
    cp_data.signal_window = window;
    cp_data.digit_hbox = digit_hbox;
    cp_data.char_label = char_label;
    cp_data.down_button = down_button;
    cp_data.up_button = up_button;
    encoding_hbox = GTK_WIDGET (gtk_builder_get_object (builder, "EncodingHBox"));
    list = gtk_container_get_children (GTK_CONTAINER (encoding_hbox));
    orig_list = list;
    while (list) {
        if (GTK_IS_RADIO_BUTTON (list->data)) {
            g_signal_connect (G_OBJECT (list->data), "clicked",
                                        G_CALLBACK (on_button_encoding_clicked),
                                        (gpointer) &cp_data);
        }
        list = list->next;
    }
    g_list_free (orig_list);

    base_hbox = GTK_WIDGET (gtk_builder_get_object (builder, "BaseHBox"));
    list = gtk_container_get_children (GTK_CONTAINER (base_hbox));
    orig_list = list;
    while (list) {
        if (GTK_IS_RADIO_BUTTON (list->data)) {
            g_signal_connect (G_OBJECT (list->data), "clicked",
                                        G_CALLBACK (on_button_base_clicked),
                                        (gpointer) &cp_data);
        }
        list = list->next;
    }
    g_list_free (orig_list);

    init_spin_button (down_button, up_button, &cp_data);

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
    GtkWidget *contents_ok;

    contents_dlg = GTK_WIDGET (gtk_builder_get_object (builder, "ContentsDialog"));
    contents_ok = GTK_WIDGET (gtk_builder_get_object (builder, "ContentsOKButton"));
    g_signal_connect (G_OBJECT (contents_ok), "clicked",
                      G_CALLBACK (on_button_ok_clicked), (gpointer) contents_dlg);
}

static void
create_custom_char_views (GtkWidget *hbox, InputPadGtkWindow *window)
{
    GtkWidget *scrolled;
    GtkWidget *main_tv;
    GtkWidget *sub_tv;
    GtkTreeModel *model;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    static CharTreeViewData tv_data;

    g_return_if_fail (INPUT_PAD_IS_GTK_WINDOW (window));

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_size_request (scrolled, 100, 200);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_ALWAYS);
    gtk_box_pack_start (GTK_BOX (hbox), scrolled, FALSE, FALSE, 0);
    gtk_widget_show (scrolled);

    /* GtkViewport is not used because the header of GtkTreeviewColumn
     * is hidden with GtkViewport. */
    main_tv = gtk_tree_view_new ();
    gtk_container_add (GTK_CONTAINER (scrolled), main_tv);
    model = custom_char_group_model_new (INPUT_PAD_GTK_WINDOW (window));
    gtk_tree_view_set_model (GTK_TREE_VIEW (main_tv), model);
    g_object_unref (G_OBJECT (model));
    gtk_widget_show (main_tv);

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_size_request (scrolled, 100, 200);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_ALWAYS);
    gtk_box_pack_start (GTK_BOX (hbox), scrolled, FALSE, FALSE, 0);
    gtk_widget_show (scrolled);

    /* GtkViewport is not used because the header of GtkTreeviewColumn
     * is hidden with GtkViewport. */
    sub_tv = gtk_tree_view_new ();
    gtk_container_add (GTK_CONTAINER (scrolled), sub_tv);
    gtk_widget_show (sub_tv);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Group"), renderer,
                                                       "text", CHAR_BLOCK_LABEL_COL,
                                                       "visible", CHAR_BLOCK_VISIBLE_COL,
                                                       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (main_tv), column);
    gtk_tree_view_set_show_expanders (GTK_TREE_VIEW (main_tv), FALSE);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Subgroup"), renderer,
                                                       "text", CHAR_BLOCK_LABEL_COL,
                                                       "visible", CHAR_BLOCK_VISIBLE_COL,
                                                       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (sub_tv), column);
    gtk_tree_view_set_show_expanders (GTK_TREE_VIEW (sub_tv), FALSE);

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_size_request (scrolled, 350, 200);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_ALWAYS);
    gtk_box_pack_start (GTK_BOX (hbox), scrolled, FALSE, FALSE, 0);
    gtk_widget_show (scrolled);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (main_tv));
    tv_data.window = GTK_WIDGET (window);
    tv_data.sub_tv = sub_tv;
    g_signal_connect (G_OBJECT (selection), "changed",
                      G_CALLBACK (on_tree_view_select_custom_char_group),
                      &tv_data);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (sub_tv));
    tv_data.scrolled = scrolled;
    tv_data.window = GTK_WIDGET (window);
    tv_data.main_tv = main_tv;
    g_signal_connect (G_OBJECT (selection), "changed",
                      G_CALLBACK (on_tree_view_select_custom_char_table),
                      &tv_data);

    /* Ubuntu does not select the first iter when invoke input-pad */
    if (gtk_tree_model_get_iter_first (model, &iter)) {
        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (main_tv));
        gtk_tree_selection_select_iter (selection, &iter);
    }
}

static void
destroy_custom_char_views (GtkWidget *hbox, InputPadGtkWindow *window)
{
    GList *hbox_list = NULL;
    GList *scrolled_list;
    GList *viewport_list;
    GtkWidget *scrolled;
    GtkWidget *viewport;
    GtkWidget *tv;
    GtkTreeViewColumn *column;
    int i;

    g_return_if_fail (INPUT_PAD_IS_GTK_WINDOW (window));

    for (i = 0; i < 2; i++) {
        hbox_list = gtk_container_get_children (GTK_CONTAINER (hbox));
        g_return_if_fail (GTK_IS_SCROLLED_WINDOW (hbox_list->data));
        scrolled = GTK_WIDGET (hbox_list->data);
        g_list_free (hbox_list);

        scrolled_list = gtk_container_get_children (GTK_CONTAINER (scrolled));
        g_return_if_fail (GTK_IS_VIEWPORT (scrolled_list->data));
        viewport = GTK_WIDGET (scrolled_list->data);
        g_list_free (scrolled_list);

        viewport_list = gtk_container_get_children (GTK_CONTAINER (viewport));
        g_return_if_fail (GTK_IS_TREE_VIEW (viewport_list->data));
        tv = GTK_WIDGET (viewport_list->data);
        g_list_free (viewport_list);

        column = gtk_tree_view_get_column (GTK_TREE_VIEW (tv), 0);
        gtk_tree_view_remove_column (GTK_TREE_VIEW (tv), column);
        gtk_container_remove (GTK_CONTAINER (viewport), tv);
        gtk_container_remove (GTK_CONTAINER (scrolled), viewport);
        gtk_container_remove (GTK_CONTAINER (hbox), scrolled);
    }

    hbox_list = gtk_container_get_children (GTK_CONTAINER (hbox));
    g_return_if_fail (GTK_IS_SCROLLED_WINDOW (hbox_list->data));
    scrolled = GTK_WIDGET (hbox_list->data);
    g_list_free (hbox_list);

    scrolled_list = gtk_container_get_children (GTK_CONTAINER (scrolled));
    g_return_if_fail (GTK_IS_VIEWPORT (scrolled_list->data));
    viewport = GTK_WIDGET (scrolled_list->data);
    g_list_free (scrolled_list);

    destroy_custom_char_view_table (scrolled, window);
    gtk_container_remove (GTK_CONTAINER (scrolled), viewport);
    gtk_container_remove (GTK_CONTAINER (hbox), scrolled);
}

static void
create_custom_char_view_ui (GtkBuilder *builder,
                            GtkWidget  *window,
                            GActionMap *map)
{
    InputPadGtkWindowPrivate *priv;
    GtkWidget *hbox;
    GAction *action;

    g_return_if_fail (INPUT_PAD_IS_GTK_WINDOW (window));

    priv = input_pad_gtk_window_get_instance_private (INPUT_PAD_GTK_WINDOW (window));
    hbox = priv->top_custom_char_view_hbox;

    create_custom_char_views (hbox, INPUT_PAD_GTK_WINDOW (window));

    /* Should not call g_variant_unref() for g_action_change_state()
     * and g_simple_action_set_state() since the variant is floating? */
    action = g_action_map_lookup_action (map, "ShowCustomChars");
    if (set_show_table_type == 1) {
        g_action_change_state (action, g_variant_new_boolean (TRUE));
        gtk_widget_show (hbox);
    } else {
        g_action_change_state (action, g_variant_new_boolean (FALSE));
        gtk_widget_hide (hbox);
    }
    g_signal_connect (G_OBJECT (window), "group-changed",
                      G_CALLBACK (on_window_group_changed_custom_char_views),
                      (gpointer) hbox);
    g_signal_connect (G_OBJECT (window), "group-appended",
                      G_CALLBACK (on_window_group_appended_custom_char_views),
                      (gpointer) hbox);
}

static void
append_all_char_view_table (GtkWidget      *scrolled,
                            unsigned int    start,
                            unsigned int    end,
                            GtkWidget      *window)
{
    InputPadGtkWindow *input_pad;
    unsigned int table_code_min = start;
    unsigned int table_code_max = end;
    GtkWidget *table;

    g_return_if_fail (INPUT_PAD_IS_GTK_WINDOW (window));

    input_pad = INPUT_PAD_GTK_WINDOW (window);

    if ((end - start + 1) > INPUT_PAD_MAX_COLUMN * INPUT_PAD_MAX_ROW) {
#if 0
        g_warning ("Too many chars %ud > %ud",
                   end - start + 1,
                   INPUT_PAD_MAX_COLUMN * INPUT_PAD_MAX_ROW);
#endif
        end = start + INPUT_PAD_MAX_COLUMN * INPUT_PAD_MAX_WINDOW_ROW - 1;
    }

    table = gtk_grid_new ();
    g_object_set (table,
                  "halign", GTK_ALIGN_START,
                  "valign", GTK_ALIGN_START,
                  "margin", 0,
                  "expand", FALSE,
                  "row-spacing", 0,
                  "column-spacing", 0,
                  "row-homogeneous", TRUE,
                  "column-homogeneous", TRUE,
                  NULL);

    append_unicode_table (table, start, start, end, input_pad);

    if (end < table_code_max) {
        GtkWidget *viewport = input_pad_gtk_viewport_new ();
        gtk_container_add (GTK_CONTAINER (scrolled), viewport);
        gtk_container_add (GTK_CONTAINER (viewport), table);
        input_pad_gtk_viewport_table_configure (INPUT_PAD_GTK_VIEWPORT (viewport),
                                                table,
                                                table_code_min,
                                                table_code_max);
        gtk_widget_show (viewport);
    } else {
#if GTK_CHECK_VERSION (3, 8, 0)
        gtk_container_add (GTK_CONTAINER (scrolled), table);
#else
        gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled),
                                               table);
#endif
    }

    gtk_widget_show (table);
}

static void
destroy_all_char_view_table (GtkWidget *scrolled, InputPadGtkWindow *window)
{
    /* Currently same contents. */
    destroy_char_view_table_common (scrolled, window);
}

static void
create_all_char_view_ui (GtkBuilder    *builder,
                         GtkWidget     *window,
                         GActionMap    *map)
{
    InputPadGtkWindowPrivate *priv;
    GtkWidget *hbox;
    GtkWidget *scrolled;
    GtkWidget *scrollbar;
    GtkWidget *tv;
    GtkTreeModel *model;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    GtkCssProvider *css_provider;
    GtkStyleContext *style_context;
    static CharTreeViewData tv_data;
    GAction *action;
    GError *error = NULL;

    priv = input_pad_gtk_window_get_instance_private (INPUT_PAD_GTK_WINDOW (window));
    hbox = priv->top_char_view_hbox;

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_size_request (scrolled, 200, 200);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_ALWAYS);
    gtk_box_pack_start (GTK_BOX (hbox), scrolled, FALSE, FALSE, 0);
    gtk_widget_show (scrolled);

    /* GtkViewport is not used because the header of GtkTreeviewColumn
     * is hidden with GtkViewport. */
    tv = gtk_tree_view_new ();
    gtk_container_add (GTK_CONTAINER (scrolled), tv);
    model = all_char_table_model_new ();
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

    /* Disable multiple updated GtkAdjustment with value-changed signal. */
    g_object_set (gtk_widget_get_settings (scrolled),
                  "gtk-enable-animations", FALSE, NULL);

    scrollbar = gtk_scrolled_window_get_vscrollbar (GTK_SCROLLED_WINDOW (scrolled));
    css_provider = gtk_css_provider_new ();
    gtk_css_provider_load_from_data (css_provider,
                                     "GtkScrollbar { " \
                                     "    -GtkScrollbar-has-backward-stepper: true;" \
                                     "    -GtkScrollbar-has-forward-stepper: true;" \
                                     "              }",
                                     -1,
                                     &error);
    style_context = gtk_widget_get_style_context (scrollbar);
    gtk_style_context_add_provider (style_context,
                                    GTK_STYLE_PROVIDER (css_provider),
                                    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref (css_provider);

    gtk_widget_show (scrolled);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));
    tv_data.scrolled = scrolled;
    tv_data.window = window;
    g_signal_connect (G_OBJECT (selection), "changed",
                      G_CALLBACK (on_tree_view_select_all_char), &tv_data);

    /* Ubuntu does not select the first iter when invoke input-pad */
    if (gtk_tree_model_get_iter_first (model, &iter)) {
        gtk_tree_selection_select_iter (selection, &iter);
    }

    /* Should not call g_variant_unref() for g_action_change_state()
     * and g_simple_action_set_state() since the variant is floating? */
    action = g_action_map_lookup_action (map, "ShowAllChars");
    if (set_show_table_type == 2) {
        g_action_change_state (action, g_variant_new_boolean (TRUE));
        gtk_widget_show (hbox);
    } else {
        g_action_change_state (action, g_variant_new_boolean (FALSE));
        gtk_widget_hide (hbox);
    }

    action = g_action_map_lookup_action (map, "ShowNothing");
    if (set_show_table_type == 0) {
        g_action_change_state (action, g_variant_new_boolean (TRUE));
    } else {
        g_action_change_state (action, g_variant_new_boolean (FALSE));
    }
}

static void
create_keyboard_layout_ui (GtkBuilder  *builder,
                           GtkWidget   *window,
                           GActionMap  *map)
{
    InputPadGtkWindowPrivate *priv;
    GtkWidget *keyboard_vbox;
    GtkWidget *button_close;
    GtkWidget *button_add;
    GtkWidget *button_remove;
    GtkWidget *button_option;
    GtkWidget *button_option_close;
    GAction *action;
    InputPadGtkWindow *input_pad = INPUT_PAD_GTK_WINDOW (window);

    priv = input_pad_gtk_window_get_instance_private (INPUT_PAD_GTK_WINDOW (window));
    keyboard_vbox = priv->top_keyboard_layout_vbox;
    button_close = GTK_WIDGET (gtk_builder_get_object (builder, "ConfigLayoutsCloseButton"));
    button_add = GTK_WIDGET (gtk_builder_get_object (builder, "ConfigLayoutsAddButton"));
    button_remove = GTK_WIDGET (gtk_builder_get_object (builder, "ConfigLayoutsRemoveButton"));
    button_option = GTK_WIDGET (gtk_builder_get_object (builder, "ConfigLayoutsOptionButton"));
    button_option_close = GTK_WIDGET (gtk_builder_get_object (builder, "ConfigOptionsCloseButton"));
    input_pad->priv->config_layouts_dialog =
        GTK_WIDGET (gtk_builder_get_object (builder, "ConfigLayoutsDialog"));
    input_pad->priv->config_layouts_add_treeview =
        GTK_WIDGET (gtk_builder_get_object (builder, "ConfigLayoutsAddTreeView"));
    input_pad->priv->config_layouts_remove_treeview =
        GTK_WIDGET (gtk_builder_get_object (builder, "ConfigLayoutsRemoveTreeView"));
    input_pad->priv->config_options_dialog =
        GTK_WIDGET (gtk_builder_get_object (builder, "ConfigOptionsDialog"));
    input_pad->priv->config_options_vbox =
        GTK_WIDGET (gtk_builder_get_object (builder, "ConfigOptionsVBox"));

    g_signal_connect_after (G_OBJECT (window), "realize",
                            G_CALLBACK (on_window_realize),
                            (gpointer) keyboard_vbox);
    g_signal_connect (G_OBJECT (button_close), "clicked",
                      G_CALLBACK (on_button_config_layouts_close_clicked),
                      (gpointer) input_pad->priv->config_layouts_dialog);
    g_signal_connect (G_OBJECT (button_add), "clicked",
                      G_CALLBACK (on_button_config_layouts_add_clicked),
                      (gpointer) window);
    g_signal_connect (G_OBJECT (button_remove), "clicked",
                      G_CALLBACK (on_button_config_layouts_remove_clicked),
                      (gpointer) window);
    g_signal_connect (G_OBJECT (button_option), "clicked",
                      G_CALLBACK (on_button_config_layouts_clicked),
                      (gpointer) input_pad->priv->config_options_dialog);
    g_signal_connect (G_OBJECT (button_option_close), "clicked",
                      G_CALLBACK (on_button_config_options_close_clicked),
                      (gpointer) input_pad);

    /* Should not call g_variant_unref() for g_action_change_state()
     * and g_simple_action_set_state() since the variant is floating? */
    action = g_action_map_lookup_action (map, "ShowLayout");
    if (set_show_layout_type == 1) {
        g_action_change_state (action, g_variant_new_boolean (TRUE));
        gtk_widget_show (keyboard_vbox);
    } else {
        g_action_change_state (action, g_variant_new_boolean (FALSE));
        gtk_widget_hide (keyboard_vbox);
    }
}

static InputPadGtkWindow *
create_ui (InputPadGtkApplication *app, unsigned int child)
{
    GError *error = NULL;
    InputPadGtkWindow *window = NULL;
    GtkBuilder *builder = NULL;
    GActionMap *map = G_ACTION_MAP (app);

    builder = app->builder;

    window = INPUT_PAD_GTK_WINDOW (g_object_new (INPUT_PAD_TYPE_GTK_WINDOW,
                                                 "application", app,
                                                 NULL));

    window->child = child;
    gtk_window_set_icon_from_file (GTK_WINDOW (window),
                                   DATAROOTDIR "/pixmaps/input-pad.png",
                                   &error);
    error = NULL;
    gtk_window_set_default_icon_from_file (DATAROOTDIR "/pixmaps/input-pad.png",
                                           &error);

    create_code_point_dialog_ui (builder, GTK_WIDGET (window));
    create_about_dialog_ui (builder, GTK_WIDGET (window));
    create_contents_dialog_ui (builder, GTK_WIDGET (window));
    create_custom_char_view_ui (builder, GTK_WIDGET (window), map);
    create_all_char_view_ui (builder, GTK_WIDGET (window), map);
    create_keyboard_layout_ui (builder, GTK_WIDGET (window), map);

    gtk_builder_connect_signals (builder, NULL);

    return window;
}

#ifdef MODULE_XTEST_GDK_BASE

static GModule *
open_xtest_gmodule (gboolean check)
{
    gchar *filename;
    GModule *module = NULL;
    const gchar *error = NULL;

    g_return_val_if_fail (MODULE_XTEST_GDK_BASE != NULL, NULL);

    if (!g_module_supported ()) {
        error = g_module_error ();
        if (!check) {
            g_warning ("Module (%s) is not supported on your platform: %s",
                       MODULE_XTEST_GDK_BASE, error ? error : "");
        }
        return NULL;
    }

    filename = g_module_build_path (MODULE_XTEST_GDK_DIR, MODULE_XTEST_GDK_BASE);
    g_return_val_if_fail (filename != NULL, NULL);

    module = g_module_open (filename, G_MODULE_BIND_LAZY);
    if (module == NULL) {
        error = g_module_error ();
        if (!check) {
            g_warning ("Could not open %s: %s", filename, error ? error : "");
        }
        g_free (filename);
        return NULL;
    }
    g_free (filename);

    return module;
}

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
    if ((module = open_xtest_gmodule (FALSE)) == NULL) {
        return;
    }

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

#endif /* MODULE_XTEST_GDK_BASE */

static gboolean
check_module_filename (const gchar *filename)
{
    if (!g_str_has_prefix (filename, "lib") ||
        !g_str_has_prefix (filename + 3, MODULE_NAME_PREFIX)) {
        g_warning ("File prefix is not input-pad library: %s", filename);
        return FALSE;
    }
#ifdef G_MODULE_SUFFIX
    /* G_MODULE_SUFFIX is defined in glibconfig.h */
    g_assert (G_MODULE_SUFFIX != NULL);
    if (!g_str_has_suffix (filename, "." G_MODULE_SUFFIX)) {
        /* filename is ignored due to no suffix G_MODULE_SUFFIX. */
        return FALSE;
    }
#else
    g_debug ("Recommend to have G_MODULE_SUFFIX to avoid '.la' file.\n");
#endif
    return TRUE;
}

static GModule *
kbdui_module_open (const gchar *filepath)
{
#ifndef USE_GLOBAL_GMODULE
    return g_module_open (filepath, G_MODULE_BIND_LAZY);
#else
    GModule *module = NULL;

    g_return_val_if_fail (filepath != NULL, NULL);

    if (module_table == NULL) {
        module_table = g_hash_table_new_full (g_str_hash, g_str_equal,
                                              (GDestroyNotify) g_free,
                                              NULL);
        g_return_val_if_fail (module_table != NULL, NULL);
    }
    module = (GModule *) g_hash_table_lookup (module_table, filepath);
    if (module == NULL) {
        module = g_module_open (filepath, G_MODULE_BIND_LAZY);
        if (module) {
            g_hash_table_insert (module_table,
                                 (gpointer) g_strdup (filepath),
                                 (gpointer) module);
        }
    }
    return module;
#endif
}

static void
kbdui_module_close (GModule *module)
{
#ifndef USE_GLOBAL_GMODULE
    const gchar *error = NULL;
    const gchar *module_name;

    if (!g_module_close (module)) {
        error = g_module_error ();
        module_name = g_module_name (module);
        g_warning ("Cannot close %s: %s",
                   module_name ? module_name : "",
                   error ? error : "");
    }
#endif
}

static void
input_pad_gtk_window_kbdui_destroy (InputPadGtkWindow *window)
{
    g_return_if_fail (window != NULL &&
                      INPUT_PAD_IS_GTK_WINDOW (window));
    g_return_if_fail (window->priv != NULL);
    g_return_if_fail (window->priv->kbdui != NULL);

    if (window->priv->kbdui->module) {
        kbdui_module_close (window->priv->kbdui->module);
        window->priv->kbdui->module = NULL;
    }
    g_object_unref (window->priv->kbdui);
    window->priv->kbdui = NULL;
}

static gboolean
input_pad_gtk_window_kbdui_module_get_desc (InputPadWindowKbduiName  *list,
                                            GModule                  *module)
{
    const gchar *error = NULL;
    const gchar *symbol_name;
    const gchar *module_name;
    const gchar *desc;
    const gchar * (* get_description) (void);
    InputPadWindowType  (* get_type) (void);

    g_return_val_if_fail (module != NULL, FALSE);

    symbol_name = "input_pad_module_get_description";
    module_name = g_module_name (module);
    if (!g_module_symbol (module, symbol_name, (gpointer *) &get_description)) {
        error = g_module_error ();
        g_warning ("Could not find '%s' in %s: %s", symbol_name,
                   module_name ? module_name : "",
                   error ? error : "");
        return FALSE;
    }
    if (get_description == NULL) {
        g_warning ("Function '%s' is NULL in %s", symbol_name,
                   module_name ? module_name : "");
        return FALSE;
    }
    if ((desc = get_description ()) != NULL) {
        list->description = g_strdup (desc);
    }

    symbol_name = "input_pad_module_get_type";
    module_name = g_module_name (module);
    if (!g_module_symbol (module, symbol_name, (gpointer *) &get_type)) {
        error = g_module_error ();
        g_warning ("Could not find '%s' in %s: %s", symbol_name,
                   module_name ? module_name : "",
                   error ? error : "");
        return FALSE;
    }
    if (get_type == NULL) {
        g_warning ("Function '%s' is NULL in %s", symbol_name,
                   module_name ? module_name : "");
        return FALSE;
    }
    list->type = get_type ();

    return TRUE;
}

static gboolean
input_pad_gtk_window_kbdui_module_arg_init (int                     *argc,
                                            char                  ***argv,
                                            GModule                 *module,
                                            InputPadGtkKbduiContext *kbdui_context)
{
    const gchar *error = NULL;
    const gchar *symbol_name;
    const gchar *module_name;
    gboolean (* arg_init) (int *argc, char ***argv, InputPadGtkKbduiContext *kbdui_context);

    g_return_val_if_fail (kbdui_context != NULL, FALSE);
    g_return_val_if_fail (module != NULL, FALSE);

    symbol_name = "input_pad_module_arg_init";
    module_name = g_module_name (module);
    if (!g_module_symbol (module, symbol_name, (gpointer *) &arg_init)) {
        error = g_module_error ();
        g_warning ("Could not find '%s' in %s: %s", symbol_name,
                   module_name ? module_name : "",
                   error ? error : "");
        return FALSE;
    }
    if (arg_init == NULL) {
        g_warning ("Function '%s' is NULL in %s", symbol_name,
                   module_name ? module_name : "");
        return FALSE;
    }
    if (!arg_init (argc, argv, kbdui_context)) {
        g_warning ("Function '%s' failed to be run in %s", symbol_name,
                   module_name ? module_name : "");
        return FALSE;
    }

    return TRUE;
}

static gboolean
input_pad_gtk_window_kbdui_module_arg_init_post (int                     *argc,
                                                 char                  ***argv,
                                                 GModule                 *module,
                                                 InputPadGtkKbduiContext *kbdui_context)
{
    const gchar *error = NULL;
    const gchar *symbol_name;
    const gchar *module_name;
    gboolean (* arg_init_post) (int *argc, char ***argv, InputPadGtkKbduiContext *kbdui_context);

    g_return_val_if_fail (kbdui_context != NULL, FALSE);
    g_return_val_if_fail (module != NULL, FALSE);

    symbol_name = "input_pad_module_arg_init_post";
    module_name = g_module_name (module);
    if (!g_module_symbol (module, symbol_name, (gpointer *) &arg_init_post)) {
        error = g_module_error ();
        g_warning ("Could not find '%s' in %s: %s", symbol_name,
                   module_name ? module_name : "",
                   error ? error : "");
        return FALSE;
    }
    if (arg_init_post == NULL) {
        g_warning ("Function '%s' is NULL in %s", symbol_name,
                   module_name ? module_name : "");
        return FALSE;
    }
    if (!arg_init_post (argc, argv, kbdui_context)) {
        g_warning ("Function '%s' failed to be run in %s", symbol_name,
                   module_name ? module_name : "");
        return FALSE;
    }

    return TRUE;
}

static GModule *
input_pad_gtk_window_parse_kbdui_module_arg_init (int                          *argc,
                                                  char                       ***argv,
                                                  const gchar                  *kbdui_name,
                                                  InputPadGtkKbduiContext      *kbdui_context)
{
    gchar *name;
    gchar *filepath;
    const gchar *error = NULL;
    GModule *module;

    g_return_val_if_fail (MODULE_KBDUI_DIR != NULL, NULL);
    g_return_val_if_fail (kbdui_name != NULL, NULL);

    if (!g_module_supported ()) {
        error = g_module_error ();
        g_warning ("Module is not supported on your platform: %s",
                   error ? error : "");
        return NULL;
    }

    name = g_strdup_printf ("%s%s", MODULE_NAME_PREFIX, kbdui_name);
    filepath = g_module_build_path (MODULE_KBDUI_DIR, name);
    g_free (name);
    g_return_val_if_fail (filepath != NULL, NULL);

    module = kbdui_module_open (filepath);
    if (module == NULL) {
        error = g_module_error ();
        g_warning ("Could not open %s: %s", filepath, error ? error : "");
        g_free (filepath);
        return NULL;
    }
    g_free (filepath);

    if (!input_pad_gtk_window_kbdui_module_arg_init (argc, argv,
                                                     module, kbdui_context)) {
        kbdui_module_close (module);
        return NULL;
    }

    return module;
}

static GList *
input_pad_gtk_window_parse_kbdui_modules (int                          *argc,
                                          char                       ***argv,
                                          InputPadGtkKbduiContext      *kbdui_context)
{
    GList *list = NULL;
    GError *error = NULL;
    const gchar *err_message;
    const gchar *dirname = MODULE_KBDUI_DIR;
    const gchar *filename;
    gchar *filepath;
    GModule *module = NULL;
    GDir *dir;

    g_return_val_if_fail (MODULE_KBDUI_DIR != NULL, NULL);

    if (!g_module_supported ()) {
        err_message = g_module_error ();
        g_warning ("Module is not supported on your platform: %s",
                   err_message ? err_message : "");
        return NULL;
    }

    if (!g_file_test (dirname, G_FILE_TEST_IS_DIR)) {
        g_warning ("Directory Not Found: %s", dirname);
        return NULL;
    }

    if ((dir  = g_dir_open (dirname, 0, &error)) == NULL) {
        g_warning ("Cannot Open Directory: %s: %s", dirname,
                   error ? error->message ? error->message : "" : "");
        g_error_free (error);
        return NULL;
    }

    while ((filename = g_dir_read_name (dir)) != NULL) {
        if (!check_module_filename (filename)) {
            continue;
        }
        filepath = g_build_filename (dirname, filename, NULL);
        module = kbdui_module_open (filepath);
        if (module == NULL) {
            err_message = g_module_error ();
            g_warning ("Could not open %s: %s", filename, err_message ? err_message : "");
            g_free (filepath);
            continue;
        }
        g_free (filepath);

        if (input_pad_gtk_window_kbdui_module_arg_init (argc, argv,
                                                        module,
                                                        kbdui_context)) {
            list = g_list_append (list, module);
        } else {
            kbdui_module_close (module);
        }
    }
    g_dir_close (dir);
    return list;
}

static void
input_pad_gtk_window_kbdui_arg_init_post_list (int                          *argc,
                                               char                       ***argv,
                                               InputPadGtkKbduiContext      *kbdui_context,
                                               GList                        *list)
{
    const gchar *error = NULL;
    GList *p = list;
    GModule *module;

    if (!g_module_supported ()) {
        error = g_module_error ();
        g_warning ("Module is not supported on your platform: %s",
                   error ? error : "");
        return;
    }

    while (p) {
        module = (GModule *) p->data;
        input_pad_gtk_window_kbdui_module_arg_init_post (argc, argv,
                                                         module,
                                                         kbdui_context);
        kbdui_module_close (module);
        p->data = NULL;
        p = p->next;
    }
    g_list_free (list);
}

static void
input_pad_gtk_window_kbdui_init (InputPadGtkWindow *window)
{
    gchar *name;
    gchar *filename;
    const gchar *error = NULL;
    const gchar *symbol_name;
    const gchar *module_name;
    GModule *module = NULL;
    gboolean (* init) (InputPadGtkWindow *window);
    InputPadGtkKbdui * (* kbdui_new) (InputPadGtkWindow *window);
    InputPadGtkKbdui *kbdui;

    g_return_if_fail (window != NULL &&
                      INPUT_PAD_IS_GTK_WINDOW (window));
    g_return_if_fail (window->priv != NULL);
    g_return_if_fail (MODULE_KBDUI_DIR != NULL);

    if (window->priv->kbdui) {
        return;
    }

    g_return_if_fail (window->priv->kbdui_name != NULL);

    if (!g_module_supported ()) {
        error = g_module_error ();
        g_warning ("Module (%s) is not supported on your platform: %s",
                   window->priv->kbdui_name, error ? error : "");
        return;
    }

    name = g_strdup_printf ("%s%s", MODULE_NAME_PREFIX, window->priv->kbdui_name);
    filename = g_module_build_path (MODULE_KBDUI_DIR, name);
    g_free (name);
    g_return_if_fail (filename != NULL);

    module = kbdui_module_open (filename);
    if (module == NULL) {
        error = g_module_error ();
        g_warning ("Could not open %s: %s", filename, error ? error : "");
        g_free (filename);
        return;
    }
    g_free (filename);

    symbol_name = "input_pad_module_init";
    module_name = g_module_name (module);
    if (!g_module_symbol (module, symbol_name, (gpointer *)&init)) {
        error = g_module_error ();
        g_warning ("Could not find '%s' in %s: %s", symbol_name,
                   module_name ? module_name : "",
                   error ? error : "");
        kbdui_module_close (module);
        return;
    }
    if (init == NULL) {
        g_warning ("Function '%s' is NULL in %s", symbol_name,
                   module_name ? module_name : "");
        kbdui_module_close (module);
        return;
    }
    if (!init (window)) {
        g_warning ("Function '%s' failed to be run in %s", symbol_name,
                   module_name ? module_name : "");
        kbdui_module_close (module);
        return;
    }

    symbol_name = "input_pad_module_kbdui_new";
    module_name = g_module_name (module);
    if (!g_module_symbol (module, symbol_name, (gpointer *)&kbdui_new)) {
        error = g_module_error ();
        g_warning ("Could not find '%s' in %s: %s", symbol_name,
                   module_name ? module_name : "",
                   error ? error : "");
        kbdui_module_close (module);
        return;
    }
    if (kbdui_new == NULL) {
        g_warning ("Function '%s' is NULL in %s", symbol_name,
                   module_name ? module_name : "");
        kbdui_module_close (module);
        return;
    }
    if ((kbdui = kbdui_new (window)) == NULL) {
        g_warning ("Function '%s' failed to be run in %s", symbol_name,
                   module_name ? module_name : "");
        kbdui_module_close (module);
        return;
    }
    kbdui->module = module;
    window->priv->kbdui = kbdui;
}

static void
input_pad_gtk_window_set_priv (InputPadGtkWindow *window)
{
    InputPadGtkWindowPrivate *priv = input_pad_gtk_window_get_instance_private (window);

    if (priv->group == NULL) {
        priv->group = input_pad_group_parse_all_files (NULL, NULL);
    }
    priv->char_button_sensitive = TRUE;

    if (kbdui_name) {
        priv->kbdui_name = g_strdup (kbdui_name);
    }

    window->priv = priv;
}

static void
input_pad_gtk_window_init (InputPadGtkWindow *window)
{
    gtk_widget_init_template (GTK_WIDGET (window));
    input_pad_gtk_window_set_priv (window);
#ifdef MODULE_XTEST_GDK_BASE
    if (use_module_xtest) {
        input_pad_gtk_window_xtest_gdk_init (window);
    }
#endif
    if (window->priv->kbdui_name) {
        input_pad_gtk_window_kbdui_init (window);
    }
}

static void
input_pad_gtk_window_real_destroy (GtkWidget *widget)
{
    InputPadGtkWindow *window = INPUT_PAD_GTK_WINDOW (widget);

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
        if (window->priv->kbdui) {
            input_pad_gtk_window_kbdui_destroy (window);
        }
        g_free (window->priv->kbdui_name);
        window->priv->kbdui_name = NULL;
        window->priv = NULL;
    }
    GTK_WIDGET_CLASS (input_pad_gtk_window_parent_class)->destroy (widget);
}

static void
input_pad_gtk_window_real_realize (GtkWidget *window)
{
#ifdef MODULE_XTEST_GDK_BASE
    InputPadGtkWindow *input_pad = INPUT_PAD_GTK_WINDOW (window);

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
            send_key_event (gtk_widget_get_window (GTK_WIDGET (window)),
                            keysym, keycode, state);
        } else {
            g_print ("%s", str ? str : "");
        }
    } else if (type == INPUT_PAD_TABLE_TYPE_KEYSYMS) {
        send_key_event (gtk_widget_get_window (GTK_WIDGET (window)),
                        keysym, keycode, state);
    } else if (type == INPUT_PAD_TABLE_TYPE_STRINGS) {
            g_print ("%s", str ? str : "");
    } else if (type == INPUT_PAD_TABLE_TYPE_COMMANDS) {
            g_print ("%s", str ? str : "");
    } else {
        g_warning ("Currently your table type is not supported.");
    }
    return FALSE;
}

static void
input_pad_gtk_window_class_init (InputPadGtkWindowClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = (GtkWidgetClass *) klass;

    widget_class->destroy = input_pad_gtk_window_real_destroy;
    widget_class->realize = input_pad_gtk_window_real_realize;

    klass->button_pressed = input_pad_gtk_window_real_button_pressed;

    gtk_widget_class_set_template_from_resource (widget_class,
                                                 "/com/github/fujiwarat/input-pad/window-gtk.ui");

    gtk_widget_class_bind_template_child_private (widget_class,
                                                  InputPadGtkWindow,
                                                  top_custom_char_view_hbox);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  InputPadGtkWindow,
                                                  top_char_view_hbox);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  InputPadGtkWindow,
                                                  top_keyboard_layout_vbox);

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

    signals[GROUP_CHANGED] =
        g_signal_new (I_("group-changed"),
                      G_TYPE_FROM_CLASS (gobject_class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (InputPadGtkWindowClass, group_changed),
                      NULL, NULL,
                      INPUT_PAD_VOID__STRING_STRING,
                      G_TYPE_NONE,
                      2, G_TYPE_STRING, G_TYPE_STRING);

    signals[GROUP_APPENDED] =
        g_signal_new (I_("group-appended"),
                      G_TYPE_FROM_CLASS (gobject_class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (InputPadGtkWindowClass, group_appended),
                      NULL, NULL,
                      INPUT_PAD_VOID__STRING_STRING,
                      G_TYPE_NONE,
                      2, G_TYPE_STRING, G_TYPE_STRING);

    signals[CHAR_BUTTON_SENSITIVE] =
        g_signal_new (I_("char-button-sensitive"),
                      G_TYPE_FROM_CLASS (gobject_class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (InputPadGtkWindowClass, char_button_sensitive),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__BOOLEAN,
                      G_TYPE_NONE,
                      1, G_TYPE_BOOLEAN);

    signals[REORDER_BUTTON_PRESSED] =
        g_signal_new (I_("reorder-button-pressed"),
                      G_TYPE_FROM_CLASS (gobject_class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (InputPadGtkWindowClass, reorder_button_pressed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE,
                      0);
}

InputPadGtkWindow *
_input_pad_gtk_window_new_with_gtype (InputPadGtkApplication *app,
                                      GtkWindowType           type,
                                      unsigned int            child,
                                      gboolean                gtype)
{
    InputPadGtkWindow *window;

    g_return_val_if_fail (type >= GTK_WINDOW_TOPLEVEL && type <= GTK_WINDOW_POPUP, NULL);

    /* FIXME: Add INPUT_PAD_TYPE_GTK_WINDOW for a workaround.
     * python + gtkbuilder outputs an error:
     * ** ERROR **: ERROR: Invalid object type `InputPadGtkWindow'
     * swig bug with gtkbuilder? Ignored -export-dynamic
     */
    if (gtype) {
        INPUT_PAD_TYPE_GTK_WINDOW;
    }

    window = create_ui (app, child);

    /* setting GtkWindowType is no longer supported with gtkbuilder because
     * the constructor property is done here. */

    return window; 
}

InputPadGtkWindow *
input_pad_gtk_window_new_with_child (InputPadGtkApplication *app,
                                     GtkWindowType           type,
                                     unsigned int            child)
{
    return _input_pad_gtk_window_new_with_gtype (app,
                                                 type,
                                                 child,
                                                 FALSE);
}

InputPadGtkWindow *
input_pad_gtk_window_new (InputPadGtkApplication *app)
{
    return _input_pad_gtk_window_new_with_gtype (app,
                                                 GTK_WINDOW_TOPLEVEL,
                                                 0,
                                                 FALSE);
}

const char*
input_pad_get_version (void)
{
    return VERSION;
}

void
input_pad_gtk_window_set_paddir (InputPadGtkWindow *window,
                                 const gchar       *paddir,
                                 const gchar       *domain)
{
    g_return_if_fail (INPUT_PAD_IS_GTK_WINDOW (window));

    g_signal_emit (window, signals[GROUP_CHANGED], 0,
                   paddir, domain);
}

void
input_pad_gtk_window_append_padfile (InputPadGtkWindow *window,
                                     const gchar       *padfile,
                                     const gchar       *domain)
{
    g_return_if_fail (INPUT_PAD_IS_GTK_WINDOW (window));

    g_signal_emit (window, signals[GROUP_APPENDED], 0,
                   padfile, domain);
}

void
input_pad_gtk_window_set_char_button_sensitive (InputPadGtkWindow *window,
                                                gboolean           sensitive)
{
    g_return_if_fail (INPUT_PAD_IS_GTK_WINDOW (window));

    if (window->priv->char_button_sensitive == sensitive) {
        return;
    }
    g_signal_emit (window, signals[CHAR_BUTTON_SENSITIVE], 0,
                   sensitive);
    window->priv->char_button_sensitive = sensitive;
}

void
input_pad_gtk_window_reorder_button_pressed (InputPadGtkWindow *window)
{
    g_return_if_fail (INPUT_PAD_IS_GTK_WINDOW (window));

    g_signal_emit (window, signals[REORDER_BUTTON_PRESSED], 0);
}

guint
input_pad_gtk_window_get_keyboard_state (InputPadGtkWindow *window)
{
    g_return_val_if_fail (INPUT_PAD_IS_GTK_WINDOW (window), 0);

    return window->priv->keyboard_state;
}

void
input_pad_gtk_window_set_keyboard_state (InputPadGtkWindow *window,
                                         guint              keyboard_state)
{
    g_return_if_fail (INPUT_PAD_IS_GTK_WINDOW (window));

    window->priv->keyboard_state = keyboard_state;
}

void
input_pad_gtk_window_set_keyboard_state_with_keysym (InputPadGtkWindow *window,
                                                     guint              keysym)
{
    g_return_if_fail (INPUT_PAD_IS_GTK_WINDOW (window));

    switch (keysym) {
    case XK_Shift_L:
    case XK_Shift_R:
        xor_modifiers (window, ShiftMask);
        break;
    case XK_Control_L:
    case XK_Control_R:
        xor_modifiers (window, ControlMask);
        break;
    case XK_Alt_L:
    case XK_Alt_R:
        xor_modifiers (window, Mod1Mask);
        break;
    default:
        if (window->priv->keyboard_state & ControlMask) {
            window->priv->keyboard_state ^= ControlMask;
        }
        if (window->priv->keyboard_state & Mod1Mask) {
            window->priv->keyboard_state ^= Mod1Mask;
        }
    }
}

InputPadWindowKbduiName *
input_pad_gtk_window_get_kbdui_name_list (void)
{
    InputPadWindowKbduiName *list = NULL;
    GError *error = NULL;
    const gchar *err_message;
    const gchar *dirname = MODULE_KBDUI_DIR;
    const gchar *filename;
    gchar *filepath;
    GModule *module = NULL;
    GDir *dir;
    int len = 0;

    g_return_val_if_fail (MODULE_KBDUI_DIR != NULL, NULL);

    if (!g_module_supported ()) {
        err_message = g_module_error ();
        g_warning ("Module is not supported on your platform: %s",
                   err_message ? err_message : "");
        return NULL;
    }

    if (!g_file_test (dirname, G_FILE_TEST_IS_DIR)) {
        g_warning ("Directory Not Found: %s", dirname);
        return NULL;
    }

    if ((dir  = g_dir_open (dirname, 0, &error)) == NULL) {
        g_warning ("Cannot Open Directory: %s: %s", dirname,
                   error ? error->message ? error->message : "" : "");
        g_error_free (error);
        return NULL;
    }

    while ((filename = g_dir_read_name (dir)) != NULL) {
        const gchar *subname = NULL;
        gchar *name = NULL;

        if (!check_module_filename (filename)) {
            continue;
        }
        filepath = g_build_filename (dirname, filename, NULL);
        module = g_module_open (filepath, G_MODULE_BIND_LAZY);
        if (module == NULL) {
            err_message = g_module_error ();
            g_warning ("Could not open %s: %s", filename, err_message ? err_message : "");
            g_free (filepath);
            continue;
        }
        g_free (filepath);

        subname = filename + 3 + strlen (MODULE_NAME_PREFIX);
        if (subname == NULL || *subname == '\0') {
            g_warning ("Filename is invalid %s", filename);
            continue;
        }
#ifdef G_MODULE_SUFFIX
        name = g_strndup (subname, strlen (subname) - strlen (G_MODULE_SUFFIX) - 1);
#else
        name = g_strdup (subname);
#endif
        if (name == NULL || *name == '\0') {
            g_warning ("Filename is invalid %s", filename);
            g_free (name);
            continue;
        }
        if (list == NULL) {
            len = 2;
            list = g_new0 (InputPadWindowKbduiName, len);
        } else {
            len++;
            list = g_renew (InputPadWindowKbduiName, list, len);
        }
        list[len - 1].name = list[len - 1].description = NULL;
        list[len - 1].type = INPUT_PAD_WINDOW_TYPE_GTK;
        list[len - 2].name = g_strdup (name);
        g_free (name);
        input_pad_gtk_window_kbdui_module_get_desc (&list[len -2], module);
        kbdui_module_close (module);
    }
    g_dir_close (dir);
    return list;
}

void
input_pad_gtk_window_get_kbdui_name_list_free (InputPadWindowKbduiName *list)
{
    int i;

    for (i = 0; list[i].name != NULL; i++) {
        g_free (list[i].name);
        g_free (list[i].description);
        list[i].name = NULL;
        list[i].description = NULL;
    }
    g_free (list);
}

int
input_pad_gtk_window_get_kbdui_name_list_length (void)
{
    InputPadWindowKbduiName *list;
    int length = 0;

    list = input_pad_gtk_window_get_kbdui_name_list ();

    if (list == NULL) {
        return 0;
    }

    for (length = 0; list[length].name != NULL; length++);
    input_pad_gtk_window_get_kbdui_name_list_free (list);

    return length;
}

gchar *
input_pad_gtk_window_get_kbdui_name_get_name_with_index (int index)
{
    InputPadWindowKbduiName *list;
    gchar *name = NULL;

    list = input_pad_gtk_window_get_kbdui_name_list ();

    if (list == NULL) {
        return NULL;
    }

    name = g_strdup (list[index].name);
    input_pad_gtk_window_get_kbdui_name_list_free (list);

    return name;
}

gchar *
input_pad_gtk_window_get_kbdui_name_get_description_with_index (int index)
{
    InputPadWindowKbduiName *list;
    gchar *description = NULL;

    list = input_pad_gtk_window_get_kbdui_name_list ();

    if (list == NULL) {
        return NULL;
    }

    description = g_strdup (list[index].description);
    input_pad_gtk_window_get_kbdui_name_list_free (list);

    return description;
}

InputPadWindowType
input_pad_gtk_window_get_kbdui_name_get_type_with_index (int index)
{
    InputPadWindowKbduiName *list;
    InputPadWindowType type;

    list = input_pad_gtk_window_get_kbdui_name_list ();

    if (list == NULL) {
        return INPUT_PAD_WINDOW_TYPE_GTK;
    }

    type = list[index].type;
    input_pad_gtk_window_get_kbdui_name_list_free (list);

    return type;
}

void
input_pad_gtk_window_set_kbdui_name (InputPadGtkWindow *window,
                                     const gchar       *name)
{
    GModule *module;
    InputPadGtkKbduiContext *context;

    if (!g_strcmp0 (name, window->priv->kbdui_name)) {
        return;
    }
    if (window->priv->kbdui) {
        input_pad_gtk_window_kbdui_destroy (window);
    }
    g_free (window->priv->kbdui_name);
    window->priv->kbdui_name = NULL;

    if (name == NULL) {
        window->priv->kbdui_name = NULL;
        return;
    }
    if (g_strcmp0 (name, "default") == 0) {
        window->priv->kbdui_name = NULL;
        return;
    }
    window->priv->kbdui_name = g_strdup (name);
    if (window->priv->kbdui_name) {
        context = input_pad_gtk_kbdui_context_new ();
        module = input_pad_gtk_window_parse_kbdui_module_arg_init (NULL, NULL,
                                                                   window->priv->kbdui_name,
                                                                   context);
        if (!module) {
            input_pad_gtk_kbdui_context_destroy (context);
            return;
        }
        input_pad_gtk_window_kbdui_module_arg_init_post (NULL, NULL,
                                                         module,
                                                         context);
        kbdui_module_close (module);
        input_pad_gtk_kbdui_context_destroy (context);
        input_pad_gtk_window_kbdui_init (window);
    }
}

void
input_pad_gtk_window_set_show_table (InputPadGtkWindow *window,
                                     InputPadWindowShowTableType type)
{
    GActionMap *map = NULL;
    GAction *action;
    GVariant *state;

    g_return_if_fail (window && INPUT_PAD_IS_GTK_WINDOW (window));
    g_return_if_fail (window->priv != NULL);

    g_object_get (window, "application", &map, NULL);

    switch (type) {
    case INPUT_PAD_WINDOW_SHOW_TABLE_TYPE_NOTHING:
        action = g_action_map_lookup_action (map, "ShowNothing");
        //g_action_change_state (action, g_variant_new_boolean (TRUE));
        state = g_action_get_state (G_ACTION (action));
        on_toggle_action (G_SIMPLE_ACTION (action), state, map);
        g_variant_unref (state);
        break;
    case INPUT_PAD_WINDOW_SHOW_TABLE_TYPE_CUSTOM:
        action = g_action_map_lookup_action (map, "ShowCustomChars");
        //g_action_change_state (action, g_variant_new_boolean (TRUE));
        state = g_action_get_state (G_ACTION (action));
        on_toggle_action (G_SIMPLE_ACTION (action), state, map);
        g_variant_unref (state);
        break;
    case INPUT_PAD_WINDOW_SHOW_TABLE_TYPE_ALL:
        action = g_action_map_lookup_action (map, "ShowAllChars");
        //g_action_change_state (action, g_variant_new_boolean (TRUE));
        state = g_action_get_state (G_ACTION (action));
        on_toggle_action (G_SIMPLE_ACTION (action), state, map);
        g_variant_unref (state);
        break;
    default:;
    }
}

void
input_pad_gtk_window_set_show_layout (InputPadGtkWindow *window,
                                      InputPadWindowShowLayoutType type)
{
    GActionMap *map = NULL;
    GAction *action;

    g_return_if_fail (window && INPUT_PAD_IS_GTK_WINDOW (window));
    g_return_if_fail (window->priv != NULL);

    g_object_get (window, "application", &map, NULL);

    /* Should not call g_variant_unref() for g_action_change_state()
     * and g_simple_action_set_state() since the variant is floating? */
    switch (type) {
    case INPUT_PAD_WINDOW_SHOW_LAYOUT_TYPE_NOTHING:
        action = g_action_map_lookup_action (map, "ShowLayout");
        g_action_change_state (action, g_variant_new_boolean (TRUE));
        on_check_action (G_SIMPLE_ACTION (action), NULL, map);
        //gtk_widget_hide (priv->top_keyboard_layout_vbox);
        //resize_toplevel_window_with_hide_widget (priv->top_keyboard_layout_vbox);
        break;
    case INPUT_PAD_WINDOW_SHOW_LAYOUT_TYPE_DEFAULT:
        action = g_action_map_lookup_action (map, "ShowLayout");
        g_action_change_state (action, g_variant_new_boolean (FALSE));
        //gtk_widget_show (priv->top_keyboard_layout_vbox);
        on_check_action (G_SIMPLE_ACTION (action), NULL, map);
        break;
    default:;
    }
}

static void
input_pad_gtk_application_init (InputPadGtkApplication *app)
{
    app->show = TRUE;
}

int
input_pad_window_init (int                     *argc,
                       char                  ***argv,
                       InputPadWindowType       type,
                       int                     *do_exit)
{
    GOptionContext *context;
#ifdef MODULE_XTEST_GDK_BASE
    GModule *module = NULL;
    gboolean has_xtest_module = FALSE;
#endif
    InputPadGtkKbduiContext *kbdui_context;
    GError *error = NULL;
    GList *list = NULL;
    const gchar *name;

    if (do_exit) {
        *do_exit = FALSE;
    }

#ifdef ENABLE_NLS
    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

    setlocale (LC_ALL, "");
#endif

    if (type != INPUT_PAD_WINDOW_TYPE_GTK) {
        g_warning ("Currently GTK type only is supported. Ignoring...");
    }

    g_set_application_name (_("Input Pad"));
    context = g_option_context_new (N_("Input Pad"));
    g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);
    g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);

#ifdef MODULE_XTEST_GDK_BASE
    if ((module = open_xtest_gmodule (TRUE)) != NULL) {
        g_module_close (module);
        has_xtest_module = TRUE;
        g_option_context_add_main_entries (context,
                                           disable_xtest_entry,
                                           GETTEXT_PACKAGE);
    } else {
        g_option_context_add_main_entries (context,
                                           enable_xtest_entry,
                                           GETTEXT_PACKAGE);
    }
#endif

    g_option_context_add_group (context, gtk_get_option_group (TRUE));

    kbdui_context = input_pad_gtk_kbdui_context_new ();
    kbdui_context->context = context;
    list = input_pad_gtk_window_parse_kbdui_modules (argc, argv, kbdui_context);

    gdk_set_allowed_backends ("x11");
    g_option_context_parse (context, argc, argv, &error);
    g_option_context_free (context);
    kbdui_context->context = NULL;

    if (ask_version) {
        g_print ("%s %s version %s\n", g_get_prgname (),
                                       g_get_application_name (),
                                       input_pad_get_version ());
        if (do_exit) {
            *do_exit = TRUE;
        }
        return 0;
    }

#ifdef MODULE_XTEST_GDK_BASE
    if (has_xtest_module) {
        use_module_xtest = !use_module_xtest;
    }
#endif

    gtk_init (argc, argv);
    input_pad_gtk_window_kbdui_arg_init_post_list (argc, argv, kbdui_context, list);
    name = input_pad_gtk_kbdui_context_get_kbdui_name (kbdui_context);
    if (name) {
        g_free (kbdui_name);
        if (g_strcmp0 (name, "default") == 0) {
            kbdui_name = NULL;
        } else {
            kbdui_name = g_strdup (name);
        }
    }
    input_pad_gtk_kbdui_context_destroy (kbdui_context);

    return 0;
}

static void
on_close_activate (GSimpleAction *action,
                   GVariant      *parameter,
                   gpointer       data)
{
    InputPadGtkApplication *app = INPUT_PAD_GTK_APPLICATION (data);
    gtk_widget_destroy (GTK_WIDGET (app->window));
}

static void
on_code_point_activate (GSimpleAction *action,
                        GVariant      *parameter,
                        gpointer       data)
{
    InputPadGtkApplication *app = INPUT_PAD_GTK_APPLICATION (data);
    GtkBuilder *builder = app->builder;
    GtkWidget *dlg = GTK_WIDGET (gtk_builder_get_object (builder,
                                                         "CodePointDialog"));
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (app->window));

    gtk_dialog_run (GTK_DIALOG (dlg));
    gtk_widget_hide (dlg);
}

static void
on_toggle_action (GSimpleAction *action,
                  GVariant      *parameter,
                  gpointer       data)
{
    InputPadGtkApplication *app = INPUT_PAD_GTK_APPLICATION (data);
    InputPadGtkWindow *window;
    InputPadGtkWindowPrivate *priv;
    GActionMap *map = G_ACTION_MAP (data);
    GVariant *state = g_action_get_state (G_ACTION (action));
    static gchar *names[] = { "ShowAllChars",
                              "ShowCustomChars",
                              "ShowNothing",
                              NULL };
    static gchar *false_name = NULL;
    const gchar *true_name;
    int i;

    if (g_variant_get_boolean (state) == TRUE)
        return;

    /* Should not call g_variant_unref() for g_action_change_state()
     * and g_simple_action_set_state() since the variant is floating? */
    for (i = 0; names[i]; i++) {
        GAction *false_action = g_action_map_lookup_action (map, names[i]);
        state = g_action_get_state (false_action);
        if (g_variant_get_boolean (state) == TRUE) {
            g_action_change_state (false_action, g_variant_new_boolean (FALSE));
            false_name = names[i];
        }
    }
    g_variant_unref (state);
    g_assert (false_name != NULL);

    g_action_change_state (G_ACTION (action), g_variant_new_boolean (TRUE));

    if (app->window == NULL)
        return;

    window = app->window;
    priv = input_pad_gtk_window_get_instance_private (window);

    true_name = g_action_get_name (G_ACTION (action));
    if (g_strcmp0 (true_name, "ShowAllChars") == 0) {
        gtk_widget_show (priv->top_char_view_hbox);
        gtk_widget_hide (priv->top_custom_char_view_hbox);
    }
    if (g_strcmp0 (true_name, "ShowCustomChars") == 0) {
        gtk_widget_hide (priv->top_char_view_hbox);
        gtk_widget_show (priv->top_custom_char_view_hbox);
    }
    if (g_strcmp0 (true_name, "ShowNothing") == 0) {
        GtkWidget *hide_widget = NULL;

        gtk_widget_hide (priv->top_char_view_hbox);
        gtk_widget_hide (priv->top_custom_char_view_hbox);

        if (g_strcmp0 (false_name, "ShowAllChars") == 0)
            hide_widget = priv->top_char_view_hbox;
        if (g_strcmp0 (false_name, "ShowCustomChars") == 0)
            hide_widget = priv->top_custom_char_view_hbox;

        if (hide_widget != NULL)
            resize_toplevel_window_with_hide_widget (hide_widget);
    }
}

static void
on_check_action (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       data)
{
    InputPadGtkApplication *app = INPUT_PAD_GTK_APPLICATION (data);
    InputPadGtkWindow *window;
    InputPadGtkWindowPrivate *priv;
    GVariant *state = g_action_get_state (G_ACTION (action));
    gboolean do_check = !g_variant_get_boolean (state);

    g_variant_unref (state);

    /* Should not call g_variant_unref() for g_action_change_state()
     * and g_simple_action_set_state() since the variant is floating? */
    g_action_change_state (G_ACTION (action), g_variant_new_boolean (do_check));

    if (app->window == NULL)
        return;

    window = app->window;
    priv = input_pad_gtk_window_get_instance_private (window);

    if (do_check)
        gtk_widget_show (priv->top_keyboard_layout_vbox);
    else {
        gtk_widget_hide (priv->top_keyboard_layout_vbox);
        resize_toplevel_window_with_hide_widget (priv->top_keyboard_layout_vbox);
    }
}

static void
on_contents_activate (GSimpleAction *action,
                      GVariant      *parameter,
                      gpointer       data)
{
    InputPadGtkApplication *app = INPUT_PAD_GTK_APPLICATION (data);
    GtkBuilder *builder = app->builder;
    GtkWidget *dlg = GTK_WIDGET (gtk_builder_get_object (builder,
                                                         "ContentsDialog"));
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (app->window));

    gtk_dialog_run (GTK_DIALOG (dlg));
    gtk_widget_hide (dlg);
}

static void
on_about_activate (GSimpleAction *action,
                   GVariant      *parameter,
                   gpointer       data)
{
    InputPadGtkApplication *app = INPUT_PAD_GTK_APPLICATION (data);
    GtkBuilder *builder = app->builder;
    GtkWidget *dlg = GTK_WIDGET (gtk_builder_get_object (builder,
                                                         "AboutDialog"));
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (app->window));

    /* Do not use GtkBuilder because gtk_about_dialog_set_logo_icon_name()
     * sets the best_size as 0 for pixmaps dir.
     * Set NULL here so that the default icon is set. */
    gtk_about_dialog_set_logo_icon_name (GTK_ABOUT_DIALOG (dlg), NULL);

    gtk_dialog_run (GTK_DIALOG (dlg));
    gtk_widget_hide (dlg);
}

static void
change_radio_state (GSimpleAction *action,
                    GVariant      *value,
                    gpointer       data)
{
    g_simple_action_set_state (action, value);
}

static GActionEntry app_entries[] =
{
    { "Close",           on_close_activate, NULL, NULL, NULL },
    { "CodePoint",       on_code_point_activate, NULL, NULL, NULL },
    { "ShowAllChars",    on_toggle_action, NULL, "true",  change_radio_state },
    { "ShowCustomChars", on_toggle_action, NULL, "false", change_radio_state },
    { "ShowNothing",     on_toggle_action, NULL, "false", change_radio_state },
    { "ShowLayout",      on_check_action, NULL, "true", NULL },
    { "Contents",        on_contents_activate, NULL, NULL, NULL },
    { "About",           on_about_activate, NULL, NULL, NULL }
};

static void
input_pad_gtk_application_startup (GApplication *app)
{
    GtkBuilder *builder;
    GMenuModel *menubar;

    G_APPLICATION_CLASS (input_pad_gtk_application_parent_class)->startup (app);

    g_action_map_add_action_entries (G_ACTION_MAP (app),
                                     app_entries, G_N_ELEMENTS (app_entries),
                                     app);

    builder = gtk_builder_new_from_resource (
            "/com/github/fujiwarat/input-pad/app-menu.ui");
    gtk_builder_set_translation_domain (builder, GETTEXT_PACKAGE);
    menubar = G_MENU_MODEL (gtk_builder_get_object (builder, "menubar"));
    gtk_application_set_menubar (GTK_APPLICATION (app), menubar);
    g_object_unref (builder);

    builder = gtk_builder_new_from_resource (
            "/com/github/fujiwarat/input-pad/dialog.ui");
    gtk_builder_set_translation_domain (builder, GETTEXT_PACKAGE);
    INPUT_PAD_GTK_APPLICATION (app)->builder = builder;
}

static void
input_pad_gtk_application_activate (GApplication *app)
{
    InputPadGtkWindow *win;

    win = input_pad_gtk_window_new (INPUT_PAD_GTK_APPLICATION (app));

    if (INPUT_PAD_GTK_APPLICATION (app)->show)
        gtk_window_present (GTK_WINDOW (win));

    INPUT_PAD_GTK_APPLICATION (app)->window = win;

    g_signal_emit (app, app_signals[APP_ACTIVATED], 0);
}

static void
input_pad_gtk_application_class_init (InputPadGtkApplicationClass *class)
{
    GApplicationClass *gclass = G_APPLICATION_CLASS (class);

    gclass->startup = input_pad_gtk_application_startup;
    gclass->activate = input_pad_gtk_application_activate;

    app_signals[APP_ACTIVATED] =
        g_signal_new (I_("activated"),
                      G_TYPE_FROM_CLASS (gclass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (InputPadGtkApplicationClass, activated),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE,
                      0);
}

void
on_is_registered (GObject *object, gpointer data)
{
    if (g_application_get_is_remote (G_APPLICATION (object))) {
        g_warning ("Another application executes input-pad.");
    }
}

InputPadGtkApplication *
input_pad_gtk_application_new (void)
{
    GObject *object = g_object_new (INPUT_PAD_TYPE_GTK_APPLICATION,
                                    "application-id", "com.github.fujiwarat.input-pad",
                                    "flags", G_APPLICATION_NON_UNIQUE,
                                    NULL);

   g_signal_connect (object, "notify::is-registered", G_CALLBACK (on_is_registered), NULL);

  return INPUT_PAD_GTK_APPLICATION (object);
}

InputPadGtkWindow *
input_pad_gtk_application_get_window (InputPadGtkApplication *app)
{
    g_return_val_if_fail (INPUT_PAD_IS_GTK_APPLICATION (app), NULL);

    return app->window;
}

void *
input_pad_window_new (void)
{
    return input_pad_gtk_application_new ();
}

void
input_pad_window_show (void *window_data)
{
    InputPadGtkApplication *app;
    InputPadGtkWindow *input_pad_window;
    GtkWidget *window;

    g_return_if_fail (window_data != NULL &&
                      INPUT_PAD_IS_GTK_APPLICATION (window_data));

    app = INPUT_PAD_GTK_APPLICATION (window_data);

    g_return_if_fail (app->window != NULL);

    input_pad_window = app->window;
    window = GTK_WIDGET (input_pad_window);

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
    InputPadGtkApplication *app;

    g_return_if_fail (window_data != NULL &&
                      INPUT_PAD_IS_GTK_APPLICATION (window_data));

    app = INPUT_PAD_GTK_APPLICATION (window_data);

    if (app->window == NULL)
        app->show = FALSE;
    else
        gtk_widget_hide (GTK_WIDGET (app->window));
}

unsigned int
input_pad_window_get_visible (void *window_data)
{
    InputPadGtkApplication *app;

    g_return_val_if_fail (window_data != NULL &&
                          INPUT_PAD_IS_GTK_APPLICATION (window_data), 0);

    app = INPUT_PAD_GTK_APPLICATION (window_data);

    g_return_val_if_fail (app->window != NULL, 0);

    return gtk_widget_get_visible (GTK_WIDGET (app->window));
}

void
input_pad_window_set_paddir (void       *window_data,
                             const char *paddir,
                             const char *domain)
{
    InputPadGtkApplication *app;
    InputPadGtkWindow *window;

    g_return_if_fail (window_data != NULL &&
                      INPUT_PAD_IS_GTK_APPLICATION (window_data));

    app = INPUT_PAD_GTK_APPLICATION (window_data);

    g_return_if_fail (app->window != NULL);

    window = app->window;
    input_pad_gtk_window_set_paddir (window,
                                     (const gchar *) paddir,
                                     (const gchar *) domain);
}

void
input_pad_window_append_padfile (void       *window_data,
                                 const char *padfile,
                                 const char *domain)
{
    InputPadGtkApplication *app;
    InputPadGtkWindow *window;

    g_return_if_fail (window_data != NULL &&
                      INPUT_PAD_IS_GTK_APPLICATION (window_data));

    app = INPUT_PAD_GTK_APPLICATION (window_data);

    g_return_if_fail (app->window != NULL);

    window = app->window;
    input_pad_gtk_window_append_padfile (window,
                                        (const gchar *) padfile,
                                        (const gchar *) domain);
}

void
input_pad_window_set_char_button_sensitive (void        *window_data,
                                            unsigned int sensitive)
{
    InputPadGtkApplication *app;
    InputPadGtkWindow *window;

    g_return_if_fail (window_data != NULL &&
                      INPUT_PAD_IS_GTK_APPLICATION (window_data));

    app = INPUT_PAD_GTK_APPLICATION (window_data);

    g_return_if_fail (app->window != NULL);

    window = app->window;
    input_pad_gtk_window_set_char_button_sensitive (window,
                                                    sensitive);
}

void
input_pad_window_reorder_button_pressed (void *window_data)
{
    InputPadGtkApplication *app;
    InputPadGtkWindow *window;

    g_return_if_fail (window_data != NULL &&
                      INPUT_PAD_IS_GTK_APPLICATION (window_data));

    app = INPUT_PAD_GTK_APPLICATION (window_data);

    g_return_if_fail (app->window != NULL);

    window = app->window;
    input_pad_gtk_window_reorder_button_pressed (window);
}

InputPadWindowKbduiName *
input_pad_window_get_kbdui_name_list (void)
{
    return input_pad_gtk_window_get_kbdui_name_list ();
}

int
input_pad_window_get_kbdui_name_list_length (void)
{
    return input_pad_gtk_window_get_kbdui_name_list_length ();
}

char *
input_pad_window_get_kbdui_name_get_name_with_index (int index)
{
    return input_pad_gtk_window_get_kbdui_name_get_name_with_index (index);
}

char *
input_pad_window_get_kbdui_name_get_description_with_index (int index)
{
    return input_pad_gtk_window_get_kbdui_name_get_description_with_index (index);
}

InputPadWindowType
input_pad_window_get_kbdui_name_get_type_with_index (int index)
{
    return input_pad_gtk_window_get_kbdui_name_get_type_with_index (index);
}

void
input_pad_window_set_kbdui_name (void *window_data, const char *name)
{
    InputPadGtkApplication *app;
    InputPadGtkWindow *window;

    g_return_if_fail (window_data != NULL &&
                      INPUT_PAD_IS_GTK_APPLICATION (window_data));

    app = INPUT_PAD_GTK_APPLICATION (window_data);

    g_return_if_fail (app->window != NULL);

    window = app->window;
    input_pad_gtk_window_set_kbdui_name (window,
                                         (const gchar *) name);
}

void
input_pad_window_set_show_table (void                          *window_data,
                                 InputPadWindowShowTableType    type)
{
    InputPadGtkApplication *app;
    InputPadGtkWindow *window;

    g_return_if_fail (window_data != NULL &&
                      INPUT_PAD_IS_GTK_APPLICATION (window_data));

    app = INPUT_PAD_GTK_APPLICATION (window_data);

    g_return_if_fail (app->window != NULL);

    window = app->window;
    input_pad_gtk_window_set_show_table (window, type);
}

void
input_pad_window_set_show_layout (void                          *window_data,
                                  InputPadWindowShowLayoutType   type)
{
    InputPadGtkApplication *app;
    InputPadGtkWindow *window;

    g_return_if_fail (window_data != NULL &&
                      INPUT_PAD_IS_GTK_APPLICATION (window_data));

    app = INPUT_PAD_GTK_APPLICATION (window_data);

    g_return_if_fail (app->window != NULL);

    window = app->window;
    input_pad_gtk_window_set_show_layout (window, type);
}

void *
input_pad_window_get_window (void *window_data)
{
    InputPadGtkApplication *app;

    g_return_val_if_fail (window_data != NULL &&
                          INPUT_PAD_IS_GTK_APPLICATION (window_data),
                          NULL);

    app = INPUT_PAD_GTK_APPLICATION (window_data);

    return input_pad_gtk_application_get_window (app);
}

void
input_pad_window_activate (void *window_data)
{
    InputPadGtkApplication *app;

    g_return_if_fail (window_data != NULL &&
                      INPUT_PAD_IS_GTK_APPLICATION (window_data));

    app = INPUT_PAD_GTK_APPLICATION (window_data);

    g_application_activate (G_APPLICATION (app));
}

int
input_pad_window_main (void *window_data)
{
    InputPadGtkApplication *app;

    g_return_val_if_fail (window_data != NULL &&
                          INPUT_PAD_IS_GTK_APPLICATION (window_data),
                          -1);

    app = INPUT_PAD_GTK_APPLICATION (window_data);

    return g_application_run (G_APPLICATION (app), 0, NULL);
}

void
input_pad_window_destroy (void *window_data)
{
    InputPadGtkApplication *app;
    InputPadGtkWindow *window;

    g_return_if_fail (window_data != NULL &&
                      INPUT_PAD_IS_GTK_APPLICATION (window_data));

    app = INPUT_PAD_GTK_APPLICATION (window_data);

    g_return_if_fail (app->window != NULL);

    window = app->window;
    gtk_widget_destroy (GTK_WIDGET (window));
}
