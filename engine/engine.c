/* vim:set et sts=4: */
/* ibus-input-pad - The input pad for IBus
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

#include <ibus.h>
#include <glib.h>

#include "engine.h"
#include <input-pad.h>
#include <input-pad-group.h>

typedef struct _IBusInputPadEngine IBusInputPadEngine;
typedef struct _IBusInputPadEngineClass IBusInputPadEngineClass;

struct _IBusInputPadEngine {
    IBusEngine      parent;

    /* members */
    IBusPropList   *prop_list;
    IBusProperty   *property;
    void           *window_data;
    GSList         *str_list;
};

struct _IBusInputPadEngineClass {
    IBusEngineClass parent;
};

static GObject* ibus_input_pad_engine_constructor
                                            (GType                   type,
                                             guint                   n_construct_params,
                                             GObjectConstructParam  *construct_params);
static void ibus_input_pad_engine_destroy   (IBusObject       *object);
static void ibus_input_pad_engine_commit_str
                                            (IBusEngine	       *engine,
                                             const gchar       *str);
static gboolean ibus_input_pad_engine_process_key_event
                                             (IBusEngine       *engine,
                                              guint             keyval,
                                              guint             keycode,
                                              guint             modifiers);
static void ibus_input_pad_engine_enable    (IBusEngine        *engine);
static void ibus_input_pad_engine_disable   (IBusEngine        *engine);
static void ibus_input_pad_engine_focus_in  (IBusEngine        *engine);
static void ibus_input_pad_engine_focus_out (IBusEngine        *engine);


static IBusEngineClass *parent_class = NULL;

static unsigned int
on_window_button_pressed (gpointer window_data,
                          gchar   *str,
                          guint    type,
                          guint    keysym,
                          guint    keycode,
                          guint    state,
                          gpointer data)
{
    IBusInputPadEngine *engine = (IBusInputPadEngine *) data;

    if (str == NULL ||
        (InputPadTableType) type != INPUT_PAD_TABLE_TYPE_CHARS) {
        return FALSE;
    }
    if (keysym > 0) {
        return FALSE;
    }

    ibus_input_pad_engine_commit_str (IBUS_ENGINE (engine), str);
    return TRUE;
}

static void
ibus_input_pad_engine_commit_str (IBusEngine *engine, const gchar *str)
{
    IBusText *text;

    text = ibus_text_new_from_string (str);
    ibus_engine_commit_text (engine, text);
}

static void
ibus_input_pad_engine_class_init (IBusInputPadEngineClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS (klass);
    IBusEngineClass *engine_class = IBUS_ENGINE_CLASS (klass);

    parent_class = (IBusEngineClass *) g_type_class_peek_parent (klass);
    object_class->constructor = ibus_input_pad_engine_constructor;
    ibus_object_class->destroy = (IBusObjectDestroyFunc) ibus_input_pad_engine_destroy;
    engine_class->process_key_event = ibus_input_pad_engine_process_key_event;
    engine_class->enable = ibus_input_pad_engine_enable;
    engine_class->disable = ibus_input_pad_engine_disable;
    engine_class->focus_in = ibus_input_pad_engine_focus_in;
    engine_class->focus_out = ibus_input_pad_engine_focus_out;
}

static void
ibus_input_pad_engine_init (IBusInputPadEngine *engine)
{
    if (engine->window_data == NULL)
        engine->window_data = input_pad_window_new (TRUE);
    g_signal_connect (G_OBJECT (engine->window_data),
                      "button-pressed",
                      G_CALLBACK (on_window_button_pressed), (gpointer) engine);
    input_pad_window_show (engine->window_data);
}

static GObject*
ibus_input_pad_engine_constructor (GType                   type,
                                   guint                   n_construct_params,
                                   GObjectConstructParam  *construct_params)
{
    IBusInputPadEngine *engine;

    engine = (IBusInputPadEngine *) G_OBJECT_CLASS (parent_class)->constructor (type,
                                                    n_construct_params,
                                                    construct_params);

    return (GObject *) engine;
}

static void
free_str_list (GSList *str_list)
{
    GSList *head = str_list;
    gchar *str;

    g_return_if_fail (str_list != NULL);

    while (str_list) {
        str = (gchar *) str_list->data;
        g_free (str);
        str_list->data = NULL;
        str_list = str_list->next;
    }
    g_slist_free (head);
}

static void
ibus_input_pad_engine_destroy (IBusObject *object)
{
    IBusInputPadEngine *engine = (IBusInputPadEngine *) object;

    if (engine->str_list) {
        free_str_list (engine->str_list);
        engine->str_list = NULL;
    }
    if (engine->window_data) {
        input_pad_window_destroy (engine->window_data);
        engine->window_data = NULL;
    }
    IBUS_OBJECT_CLASS (parent_class)->destroy (object);
}

static gboolean
ibus_input_pad_engine_process_key_event (IBusEngine    *engine,
                                         guint          keyval,
                                         guint          keycode,
                                         guint          modifiers)
{
    return parent_class->process_key_event (engine, keyval, keycode, modifiers);
}

static void
ibus_input_pad_engine_enable (IBusEngine *engine)
{
    input_pad_window_show (((IBusInputPadEngine *) engine)->window_data);
    parent_class->enable (engine);
}

static void
ibus_input_pad_engine_disable (IBusEngine *engine)
{
    input_pad_window_hide (((IBusInputPadEngine *) engine)->window_data);
    parent_class->disable (engine);
}

static void
ibus_input_pad_engine_focus_in (IBusEngine *engine)
{
    IBusInputPadEngine *input_pad = (IBusInputPadEngine *) engine;
    gchar *str;
    GSList *str_list;

    parent_class->focus_in (engine);
    str_list = input_pad->str_list;
    while (str_list) {
        str = (gchar *) str_list->data;
        ibus_input_pad_engine_commit_str (engine, str);
        g_free (str);
        str_list->data = NULL;
        str_list = str_list->next;
    }
    if (input_pad->str_list) {
        g_slist_free (input_pad->str_list);
        input_pad->str_list = NULL;
    }
}

static void
ibus_input_pad_engine_focus_out (IBusEngine *engine)
{
    parent_class->focus_out (engine);
}

GType
ibus_input_pad_engine_get_type (void)
{
    static GType type = 0;

    static const GTypeInfo type_info = {
        sizeof (IBusInputPadEngineClass),
        (GBaseInitFunc)     NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc)    ibus_input_pad_engine_class_init,
        NULL,
        NULL,
        sizeof (IBusInputPadEngine),
        0,
        (GInstanceInitFunc) ibus_input_pad_engine_init,
    };

    if (type == 0) {
            type = g_type_register_static (IBUS_TYPE_ENGINE,
                                           "IBusInputPadEngine",
                                           &type_info,
                                           (GTypeFlags) 0);
    }

    return type;
}

void
ibus_input_pad_init (int *argc, char ***argv, IBusBus *bus)
{
    input_pad_window_init (argc, argv, 0);
}

void
ibus_input_pad_exit (void)
{
}
