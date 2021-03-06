/* vim:set et sts=4: */
/* input-pad - The input pad
 * Copyright (C) 2010-2014 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2010-2014 Red Hat, Inc.
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

#include "input-pad.h"
#include <stdlib.h> /* exit */

int
main (int argc, char *argv[])
{
    void *data;
    int do_exit = 0;
    int retval;

    retval = input_pad_window_init (&argc, &argv, 0, &do_exit);

    if (do_exit) {
        exit (retval);
    }

    data = input_pad_window_new ();

    return input_pad_window_main (data);
}
