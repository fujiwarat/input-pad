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

#ifndef __INPUT_PAD_I18N_H__
#define __INPUT_PAD_I18N_H__

#ifdef ENABLE_NLS

#include <libintl.h>

#ifndef GETTEXT_PACKAGE
#error Need to inlcude config.h
#endif

#define _(str)      ((char *) g_dgettext (GETTEXT_PACKAGE, (str)))
#define N_(str)     (str)

#else

#define _(str)  (str)
#define N_(str) (str)

#endif

#define I_(str) g_intern_static_string (str)

#endif
