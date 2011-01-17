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

#define KEYSYM_PREFIX "XK_"
#define ARRAY_START "" \
"#ifndef __INPUT_PAD_KEYSYM_STR2VAL_H__\n"                              \
"#define __INPUT_PAD_KEYSYM_STR2VAL_H__\n"                              \
"\n"                                                                    \
"#include <X11/Xlib.h>\n"                                               \
"#include <%s>\n"                                                       \
"\n"                                                                    \
"typedef struct _InputPadKeysymTable InputPadKeysymTable;\n"            \
"\n"                                                                    \
"struct _InputPadKeysymTable {\n"                                       \
"  const char *str;\n"                                                  \
"  KeySym keysym;\n"                                                    \
"};\n"                                                                  \
"\n"                                                                    \
"static const InputPadKeysymTable input_pad_keysym_table[] = {\n"

#define ARRAY_END "" \
"  {NULL, 0x0},\n"                                                      \
"};\n"                                                                  \
"#endif"

static gchar *progname;

static void
usage (void) {
    g_print ("usage: %s FILENAME\n", progname ? progname : "");
    g_print ("FILENAME                  file path of X11/keysymdef.h\n");
}

gchar *
get_include (const gchar *filename)
{
    gchar *basename, *p, *f;

    g_return_val_if_fail (filename != NULL, NULL);
    basename = g_path_get_basename (filename);

    f = (gchar *) filename;
    p = g_strrstr (f, "/");
    if (p == NULL || p  - 1 <= f) {
        g_free (basename);
        return g_strdup (f);
    }
    f = g_strndup (filename, p - filename);
    p = g_strrstr (f, "/");
    if (p == NULL) {
        p = g_strdup_printf ("%s/%s", p, basename);
        g_free (basename);
        g_free (f);
        return p;
    }
    p = g_strdup_printf ("%s/%s", p + 1, basename);
    g_free (basename);
    g_free (f);
    return p;
}

gchar *
gen_array (const gchar *filename)
{
    gchar *contents = NULL;
    gchar *include = NULL;
    gchar *p;
    gchar **words;
    gchar *keysym;
    gchar *array_line;
    GError *error = NULL;
    GString *line;
    GString *array;

    if (!filename ||
        !g_file_test (filename, G_FILE_TEST_EXISTS)) {
        g_error ("ERROR: File Not Found: %s\n", filename ? filename : "(null)");
        return NULL;
    }

    if (!g_file_get_contents (filename, &contents, NULL, &error)) {
        g_error ("ERROR: Failed to get contents: %s\n",
                 error ? error->message ? error->message : "" : "");
        g_error_free (error);
        return NULL;
    }

    include = get_include (filename);
    g_return_val_if_fail (include != NULL && *include != '\0', NULL);
    array_line = g_strdup_printf (ARRAY_START, include);
    array = g_string_new (array_line);
    g_free (array_line);

    line = g_string_new ("");
    for (p = contents; p && *p; p = g_utf8_find_next_char (p, NULL)) {
        gunichar ch;

        ch = g_utf8_get_char (p);
        if ((ch != '\n') && (ch != '\0')) {
            g_string_append_unichar (line, ch);
            continue;
        }
        if (g_str_has_prefix (line->str, "#ifdef") ||
            g_str_has_prefix (line->str, "#ifndef") ||
            g_str_has_prefix (line->str, "#endif")) {
            g_string_append (array, line->str);
            g_string_append_unichar (array, '\n');
            goto next_line;
        }
        if (!g_str_has_prefix (line->str, "#define")) {
            goto next_line;
        }
        words = g_strsplit_set (line->str, " \t", -1);
        keysym = g_strdup (words[1]);
        g_strfreev (words);
        if (*keysym == '\0') {
            g_free (keysym);
            goto next_line;
        }
        if (!g_str_has_prefix (keysym, KEYSYM_PREFIX)) {
            g_free (keysym);
            g_string_append (array, line->str);
            g_string_append_unichar (array, '\n');
            goto next_line;
        }

        array_line = g_strdup_printf ("  {\"%s\", %s},\n",
                                      keysym + g_utf8_strlen (KEYSYM_PREFIX, -1),
                                      keysym);
        g_free (keysym);
        g_string_append (array, array_line);
        g_free (array_line);

next_line:
        g_string_set_size (line, 0);
    }
    g_string_free (line, TRUE);
    g_free (contents);
    g_string_append (array, ARRAY_END);
    return g_string_free (array, FALSE);
}

int
main (int argc, char *argv[])
{
    char *d;
    char *array;
    char *output_filename;
    GError *error = NULL;

    progname = g_path_get_basename (argv[0]);
    if (argc == 1) {
        usage ();
        g_free (progname);
        return -1;
    }

    array = gen_array (argv[1]);
    output_filename = g_strdup_printf ("%s.h", progname);
    if (!g_file_set_contents (output_filename, array, -1, &error)) {
        g_error ("ERROR: Failed to save %s: %s\n",
                 output_filename,
                 error ? error->message ? error->message : "" : "");
        g_error_free (error);
    }
    g_free (array);
    g_free (progname);

    return 0;
}
