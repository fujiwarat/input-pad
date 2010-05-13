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
#include <locale.h>

#include "i18n.h"
#include "engine.h"

static IBusBus *bus = NULL;
static IBusFactory *factory = NULL;

static void
ibus_disconnected_cb (IBusBus  *bus,
                      gpointer  user_data)
{
    g_debug ("bus disconnected");
    ibus_quit ();
}

static void
start_component (int argc, char **argv)
{
    IBusComponent *component;

    ibus_init ();

    bus = ibus_bus_new ();
    g_signal_connect (bus, "disconnected", G_CALLBACK (ibus_disconnected_cb), NULL);

    ibus_input_pad_init (&argc, &argv, bus);

    component = ibus_component_new ("org.freedesktop.IBus.InputPad",
                                    N_("Input Pad"),
                                    "0.1.0",
                                    "GPL",
                                    "Takao Fujiwara <takao.fujiwara1@gmail.com>",
                                    "http://code.google.com/p/ibus/",
                                    "",
                                    "ibus-input-pad");
    ibus_component_add_engine (component,
                               ibus_engine_desc_new ("input-pad",
                                                     N_("Input Pad"),
                                                     N_("Input Pad"),
                                                     "xx",
                                                     "GPL",
                                                     "Takao Fujiwara <takao.fujiwara1@gmail.com>",
                                                     DATAROOTDIR "/pixmaps/input-pad.png",
                                                     "us"));

    factory = ibus_factory_new (ibus_bus_get_connection (bus));

    ibus_factory_add_engine (factory, "input-pad", IBUS_TYPE_INPUT_PAD_ENGINE);

    //if (ibus) {
    if (TRUE) {
        ibus_bus_request_name (bus, "org.freedesktop.IBus.InputPad", 0);
    }
    else {
        ibus_bus_register_component (bus, component);
    }

    g_object_unref (component);

    ibus_main ();

    ibus_input_pad_exit ();
}

int
main (int argc, char **argv)
{
    GError *error = NULL;
    GOptionContext *context;

    setlocale (LC_ALL, "");

    bindtextdomain(GETTEXT_PACKAGE, IBUS_LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    start_component (argc, argv);
}
