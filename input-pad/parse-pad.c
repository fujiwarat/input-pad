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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <unistd.h> /* getuid */
#include <pwd.h> /* getpwuid */

#include "i18n.h"
#include "input-pad-group.h"
#include "input-pad-private.h"

static const gchar *xml_file;
static const gchar *translation_domain;

static int
cmp_filepath (gconstpointer a, gconstpointer b)
{
    const gchar *file1 = a;
    const gchar *file2 = b;
    gboolean file1_content;
    gboolean file2_content;

    file1_content = (file1 && *file1);
    file2_content = (file2 && *file2);

    if (file1_content && !file2_content) {
        return 1;
    }
    if (!file1_content && file2_content) {
        return -1;
    }

    if (g_str_has_prefix (file1, INPUT_PAD_PAD_SYSTEM_DIR) &&
        !g_str_has_prefix (file2, INPUT_PAD_PAD_SYSTEM_DIR)) {
        return -1;
    }
    if (!g_str_has_prefix (file1, INPUT_PAD_PAD_SYSTEM_DIR) &&
        g_str_has_prefix (file2, INPUT_PAD_PAD_SYSTEM_DIR)) {
        return 1;
    }
    return g_strcmp0 (file1, file2);
}

static void
get_content (xmlNodePtr node, char **content, gboolean i18n)
{
    xmlNodePtr current;
    gboolean has_content = FALSE;

    for (current = node; current; current = current->next) {
        if (current->type == XML_TEXT_NODE) {
            if (current->content) {
                if (i18n) {
                    if (translation_domain) {
                        *content = g_strdup (D_(translation_domain,
                                                (char *) current->content));
                    } else {
                        *content = g_strdup (_((char *) current->content));
                    }
                } else {
                    *content = g_strdup ((char *) current->content);
                }
#ifdef DEBUG
                g_print ("content %s\n", (char *) *content);
#endif
                has_content = TRUE;
                break;
            } else {
                g_error ("tag does not have content in the file %s",
                         xml_file);
            }
        }
    }
    if (!has_content) {
        g_error ("tag does not have content in the file %s",
                 xml_file);
    }
}

static void
get_int (xmlNodePtr node, int *retval, int base)
{
    xmlNodePtr current;
    gboolean has_content = FALSE;

    for (current = node; current; current = current->next) {
        if (current->type == XML_TEXT_NODE) {
            if (current->content) {
                *retval = (int) g_ascii_strtoll ((const gchar *) current->content,
                                                 NULL, base);
                has_content = TRUE;
                break;
            } else {
                g_error ("tag does not have content in the file %s",
                         xml_file);
            }
        }
    }
    if (!has_content) {
        g_error ("tag does not have content in the file %s",
                 xml_file);
    }
}

static void
parse_keys (xmlNodePtr node, InputPadTable **ptable)
{
    xmlNodePtr current;
    gboolean has_keys = FALSE;

    for (current = node; current; current = current->next) {
        if (current->type == XML_ELEMENT_NODE) {
            if (!g_strcmp0 ((char *) current->name, "keysyms")) {
                (*ptable)->type = INPUT_PAD_TABLE_TYPE_KEYSYMS;
                if (current->children) {
                    get_content (current->children, &(*ptable)->data.keysyms, FALSE);
                    has_keys = TRUE;
                } else {
                    g_error ("tag %s does not have child tags in the file %s",
                             (char *) current->name,
                             xml_file);
                }
            }
        }
    }
    if (!has_keys) {
        g_error ("tag %s does not find \"keysyms\" tag in file %s",
                 node->parent ? node->parent->name ? (char *) node->parent->name : "(null)" : "(null)",
                 xml_file);
    }
}

static void
parse_string (xmlNodePtr node, InputPadTableStr *str)
{
    xmlNodePtr current;
    gboolean has_label = FALSE;

    for (current = node; current; current = current->next) {
        if (current->type == XML_ELEMENT_NODE) {
            if (!g_strcmp0 ((char *) current->name, "label")) {
                if (current->children) {
                    get_content (current->children, &str->label, FALSE);
                    has_label = TRUE;
                } else {
                    g_error ("tag %s does not have child tags in the file %s",
                             (char *) current->name,
                             xml_file);
                }
            }
            if (!g_strcmp0 ((char *) current->name, "comment")) {
                if (current->children) {
                    get_content (current->children, &str->comment, TRUE);
                } else {
                    g_error ("tag %s does not have child tags in the file %s",
                             (char *) current->name,
                             xml_file);
                }
            }
            if (!g_strcmp0 ((char *) current->name, "rawtext")) {
                if (current->children) {
                    get_content (current->children, &str->rawtext, TRUE);
                } else {
                    g_error ("tag %s does not have child tags in the file %s",
                             (char *) current->name,
                             xml_file);
                }
            }
        }
    }
    if (!has_label) {
        g_error ("tag %s does not find \"label\" tag in file %s",
                 node->parent ? node->parent->name ? (char *) node->parent->name : "(null)" : "(null)",
                 xml_file);
    }
}

static void
parse_command (xmlNodePtr node, InputPadTableCmd *cmd)
{
    xmlNodePtr current;
    gboolean has_execl = FALSE;

    for (current = node; current; current = current->next) {
        if (current->type == XML_ELEMENT_NODE) {
            if (!g_strcmp0 ((char *) current->name, "label")) {
                if (current->children) {
                    get_content (current->children, &cmd->label, TRUE);
                } else {
                    g_error ("tag %s does not have child tags in the file %s",
                             (char *) current->name,
                             xml_file);
                }
            }
            if (!g_strcmp0 ((char *) current->name, "execl")) {
                if (current->children) {
                    get_content (current->children, &cmd->execl, FALSE);
                    has_execl = TRUE;
                } else {
                    g_error ("tag %s does not have child tags in the file %s",
                             (char *) current->name,
                             xml_file);
                }
            }
        }
    }
    if (!has_execl) {
        g_error ("tag %s does not find \"execl\" tag in file %s",
                 node->parent ? node->parent->name ? (char *) node->parent->name : "(null)" : "(null)",
                 xml_file);
    }
}

#define GET_TABLE_SUB_ARRAY_LEN(type_name, TypeName, label)             \
static int                                                              \
get_##type_name##_array_len (InputPadTable##TypeName *instance)         \
{                                                                       \
    int i = 0;                                                          \
                                                                        \
    if (instance == NULL) {                                             \
        return 0;                                                       \
    }                                                                   \
    while (instance[i].label) {                                         \
        i++;                                                            \
    }                                                                   \
    return i;                                                           \
}

GET_TABLE_SUB_ARRAY_LEN (string, Str, label)
GET_TABLE_SUB_ARRAY_LEN (command, Cmd, execl)
#undef GET_TABLE_SUB_ARRAY_LEN

static void
free_string_array (InputPadTableStr *strs)
{
    int i = 0;
    if (strs == NULL) {
        return;
    }
    for (i = 0; strs[i].label; i++) {
        g_free (strs[i].label);
        g_free (strs[i].comment);
        g_free (strs[i].rawtext);
        strs[i].label = NULL;
        strs[i].comment = NULL;
        strs[i].rawtext = NULL;
    }
    g_free (strs);
}

static void
free_command_array (InputPadTableCmd *cmds)
{
    int i = 0;
    if (cmds == NULL) {
        return;
    }
    for (i = 0; cmds[i].execl; i++) {
        g_free (cmds[i].execl);
        g_free (cmds[i].label);
        cmds[i].label = NULL;
        cmds[i].execl = NULL;
    }
    g_free (cmds);
}

static void
parse_table_sub_string (xmlNodePtr node, InputPadTable **ptable)
{
    int len;

    len = get_string_array_len ((*ptable)->data.strs);
    if (len == 0) {
        (*ptable)->data.strs = g_new0 (InputPadTableStr, 2);
    } else {
        (*ptable)->data.strs = g_renew (InputPadTableStr,
                                        (*ptable)->data.strs,
                                        len + 2);
        (*ptable)->data.strs[len + 1].label= NULL;
        (*ptable)->data.strs[len + 1].comment = NULL;
        (*ptable)->data.strs[len + 1].rawtext = NULL;
    }
    parse_string (node, &(*ptable)->data.strs[len]);
}

static void
parse_table_sub_command (xmlNodePtr node, InputPadTable **ptable)
{
    int len;

    len = get_command_array_len ((*ptable)->data.cmds);
    if (len == 0) {
        (*ptable)->data.cmds = g_new0 (InputPadTableCmd, 2);
    } else {
        (*ptable)->data.cmds = g_renew (InputPadTableCmd,
                                       (*ptable)->data.cmds,
                                       len + 2);
        (*ptable)->data.cmds[len + 1].label= NULL;
        (*ptable)->data.cmds[len + 1].execl = NULL;
    }
    parse_command (node, &(*ptable)->data.cmds[len]);
}

static void
parse_table (xmlNodePtr node, InputPadTable **ptable)
{
    xmlNodePtr current;
    gboolean has_name = FALSE;
    gboolean has_chars = FALSE;

    for (current = node; current; current = current->next) {
        if (current->type == XML_ELEMENT_NODE) {
            if (!g_strcmp0 ((char *) current->name, "name")) {
                if (current->children) {
                    get_content (current->children, &(*ptable)->name, TRUE);
                    has_name = TRUE;
                } else {
                    g_error ("tag %s does not have child tags in the file %s",
                             (char *) current->name,
                             xml_file);
                }
            }
            if (!g_strcmp0 ((char *) current->name, "column")) {
                if (current->children) {
                    get_int (current->children, &(*ptable)->column, 10);
                } else {
                    g_error ("tag %s does not have child tags in the file %s",
                             (char *) current->name,
                             xml_file);
                }
            }
            if (!g_strcmp0 ((char *) current->name, "chars")) {
                (*ptable)->type = INPUT_PAD_TABLE_TYPE_CHARS;
                if (current->children) {
                    get_content (current->children, &(*ptable)->data.chars, FALSE);
                    has_chars = TRUE;
                } else {
                    g_error ("tag %s does not have child tags in the file %s",
                             (char *) current->name,
                             xml_file);
                }
            }
            if (!g_strcmp0 ((char *) current->name, "keys")) {
                (*ptable)->type = INPUT_PAD_TABLE_TYPE_KEYSYMS;
                if (current->children) {
                    parse_keys (current->children, ptable);
                    has_chars = TRUE;
                } else {
                    g_error ("tag %s does not have child tags in the file %s",
                             (char *) current->name,
                             xml_file);
                }
            }
            if (!g_strcmp0 ((char *) current->name, "string")) {
                (*ptable)->type = INPUT_PAD_TABLE_TYPE_STRINGS;
                if (current->children) {
                    parse_table_sub_string (current->children, ptable);
                    has_chars = TRUE;
                } else {
                    g_error ("tag %s does not have child tags in the file %s",
                             (char *) current->name,
                             xml_file);
                }
            }
            if (!g_strcmp0 ((char *) current->name, "command")) {
                (*ptable)->type = INPUT_PAD_TABLE_TYPE_COMMANDS;
                if (current->children) {
                    parse_table_sub_command(current->children, ptable);
                    has_chars = TRUE;
                } else {
                    g_error ("tag %s does not have child tags in the file %s",
                             (char *) current->name,
                             xml_file);
                }
            }
        }
    }
    if (!has_name || !has_chars) {
        g_error ("tag %s does not find \"name\" or \"chars\" tag in file %s",
                 node->parent ? node->parent->name ? (char *) node->parent->name : "(null)" : "(null)",
                 xml_file);
    }
}

static void
parse_group (xmlNodePtr node, InputPadGroup **pgroup)
{
    xmlNodePtr current;
    gboolean has_name = FALSE;
    gboolean has_table = FALSE;
    InputPadTable **ptable = &((*pgroup)->table);

    for (current = node; current; current = current->next) {
        if (current->type == XML_ELEMENT_NODE) {
            if (!g_strcmp0 ((char *) current->name, "name")) {
                if (current->children) {
                    get_content (current->children, &(*pgroup)->name, TRUE);
                    has_name = TRUE;
                } else {
                    g_error ("tag %s does not have child tags in the file %s",
                             (char *) current->name,
                             xml_file);
                }
            }
            if (!g_strcmp0 ((char *) current->name, "table")) {
                if (current->children) {
                    *ptable = g_new0 (InputPadTable, 1);
                    (*ptable)->priv = g_new0 (InputPadTablePrivate, 1);
                    (*ptable)->column = 15;
                    parse_table (current->children, ptable);
                    ptable = &((*ptable)->next);
                    has_table = TRUE;
                } else {
                    g_error ("tag %s does not have child tags in the file %s",
                             (char *) current->name,
                             xml_file);
                }
            }
        }
    }
    if (!has_name || !has_table ) {
        g_error ("tag %s does not find \"name\" or \"table\" tag in file %s",
                 node->parent ? node->parent->name ? (char *) node->parent->name : "(null)" : "(null)",
                 xml_file);
    }
}

static void
parse_pad (xmlNodePtr node, InputPadGroup **pgroup)
{
    xmlNodePtr current;
    gboolean has_pad = FALSE;

    for (current = node; current; current = current->next) {
        if (current->type == XML_ELEMENT_NODE &&
            !g_strcmp0 ((char *) current->name, "group")) {
            if (current->children) {
                *pgroup = g_new0 (InputPadGroup, 1);
                (*pgroup)->priv = g_new0 (InputPadGroupPrivate, 1);
                parse_group (current->children, pgroup);
                has_pad = TRUE;
                pgroup = &((*pgroup)->next);
            } else {
                g_error ("tag %s does not have child tags in the file %s",
                         (char *) current->name,
                         xml_file);
            }
        }
    }

    if (!has_pad) {
        g_error ("tag %s does not find \"group\" tag in file %s",
                 node->parent ? node->parent->name ? (char *) node->parent->name : "(null)" : "(null)",
                 xml_file);
    }
}

static void
parse_input_pad (xmlNodePtr node, InputPadGroup **pgroup)
{
    xmlNodePtr current;
    gboolean has_pad_child = FALSE;

    for (current = node; current; current = current->next) {
        if (current->type == XML_ELEMENT_NODE &&
            !g_strcmp0 ((char *) current->name, "pad")) {
            if (current->children) {
                parse_pad (current->children, pgroup);
                has_pad_child = TRUE;
                break;
            } else {
                g_error ("tag %s does not have child tags in the file %s",
                         (char *) current->name,
                         xml_file);
            }
        }
    }

    if (!has_pad_child) {
        g_error ("tag %s does not find \"pad\" tag in file %s",
                 node->parent ? node->parent->name ? (char *) node->parent->name : "(null)" : "(null)",
                 xml_file);
    }
}

static gchar *
get_user_pad_dir (void)
{
    gchar *home_dir = NULL;
    gchar *config_dir;
    struct passwd *pw;

    if (g_getenv ("HOME")) {
        home_dir = (gchar *) g_getenv ("HOME");
    } else {
        pw = getpwuid (getuid ());
        home_dir = pw->pw_dir;
    }
    if (home_dir == NULL) {
        home_dir = "/";
    }
    config_dir = g_strdup_printf ("%s/.config/input-pad/pad", home_dir);
    return config_dir;
}

InputPadGroup *
input_pad_group_append_from_file (InputPadGroup        *group,
                                  const gchar          *file,
                                  const gchar          *domain)
{
    InputPadGroup **pgroup = &group;
    xmlDocPtr doc;
    xmlNodePtr node;

    xmlInitParser ();
    xmlLoadExtDtdDefaultValue = XML_DETECT_IDS | XML_COMPLETE_ATTRS;
    xmlSubstituteEntitiesDefault (1);

    xml_file = file;
    translation_domain = domain;
    doc = xmlParseFile (xml_file);
    if (doc == NULL || xmlDocGetRootElement (doc) == NULL) {
        g_error ("Unable to parse file: %s", xml_file);
    }

    node = xmlDocGetRootElement (doc);
    if (node == NULL) {
        g_error ("Top node not found: %s", xml_file);
    }

    if (g_strcmp0 ((gchar *) node->name, "input-pad")) {
        g_error ("The first tag should be <input-pad>: %s", xml_file);
    }
    if (node->children == NULL) {
        g_error ("tag %s does not have child tags in the file %s",
                 (char *) node->name, xml_file);
    }

    while (pgroup && *pgroup) {
        pgroup = &((*pgroup)->next);
    }
    parse_input_pad (node->children, pgroup);

    xmlFreeDoc (doc);
    xmlCleanupParser ();

    xml_file = NULL;
    translation_domain = NULL;

    return group;
}

InputPadGroup *
input_pad_group_parse_all_files (const char *custom_dirname, const char *domain)
{
    const gchar *dirname = INPUT_PAD_PAD_SYSTEM_DIR;
    const gchar *filename;
    gchar *filepath;
    gchar *config_dir;
    GDir *dir;
    GError *error = NULL;
    InputPadGroup *group = NULL;
    GSList *file_list = NULL;
    GSList *list;

    if (custom_dirname != NULL) {
        dirname = (const gchar *) custom_dirname;
    }

    if (!dirname ||
        !g_file_test (dirname, G_FILE_TEST_IS_DIR)) {
        g_warning ("Directory Not Found: %s", dirname ? dirname : "(null)");
        return NULL;
    }

    if ((dir  = g_dir_open (dirname, 0, &error)) == NULL) {
        g_warning ("Cannot Open Directory: %s: %s", dirname,
                   error ? error->message ? error->message : "" : "");
        g_error_free (error);
        return NULL;
    }

    while ((filename = g_dir_read_name (dir)) != NULL) {
        if (!g_str_has_suffix (filename, ".xml")) {
            g_warning ("File extension is not xml: %s", filename);
            continue;
        }
        filepath = g_build_filename (dirname, filename, NULL);
        file_list = g_slist_append (file_list, (gpointer) filepath);
    }
    g_dir_close (dir);

    dir = NULL;
    config_dir = get_user_pad_dir ();
    if (config_dir &&
        g_file_test (config_dir, G_FILE_TEST_IS_DIR)) {
        dir  = g_dir_open (config_dir, 0, NULL);
    }
    while (dir && (filename = g_dir_read_name (dir)) != NULL) {
        if (!g_str_has_suffix (filename, ".xml")) {
            g_warning ("File extension is not xml: %s", filename);
            continue;
        }
        filepath = g_build_filename (config_dir, filename, NULL);
        file_list = g_slist_append (file_list, (gpointer) filepath);
    }
    g_free (config_dir);
    if (dir) {
        g_dir_close (dir);
    }

    if (!file_list) {
        return NULL;
    }

    list = file_list = g_slist_sort (file_list, cmp_filepath);
    while (list) {
        filepath = (gchar *) list->data;
        group = input_pad_group_append_from_file (group, filepath, domain);
        g_free (filepath);
        list = g_slist_next (list);
    }
    g_slist_free (file_list);

    return group;
}

void
input_pad_group_destroy (InputPadGroup *group_data)
{
    InputPadGroup *group, *prev_group;
    InputPadTable *table, *prev_table;

    group = group_data;
    while (group) {
        table = group->table;
        while (table) {
            g_free (table->name);
            table->name = NULL;
            if (table->type == INPUT_PAD_TABLE_TYPE_CHARS) {
                g_free (table->data.chars);
                table->data.chars = NULL;
            } else if (table->type == INPUT_PAD_TABLE_TYPE_KEYSYMS) {
                g_free (table->data.keysyms);
                table->data.keysyms = NULL;
            } else if (table->type == INPUT_PAD_TABLE_TYPE_STRINGS) {
                free_string_array (table->data.strs);
                table->data.strs = NULL;
            } else if (table->type == INPUT_PAD_TABLE_TYPE_COMMANDS) {
                free_command_array (table->data.cmds);
                table->data.cmds = NULL;
            } else {
                g_warning ("Free is not defined in type %d", table->type);
            }
            prev_table = table;
            table = table->next;
            prev_table->next = NULL;
            g_free (prev_table);
        }
        group->table = NULL;
        g_free (group->name);
        group->name = NULL;
        prev_group = group;
        group = group->next;
        prev_group->next = NULL;
        g_free (prev_group);
    }
}
