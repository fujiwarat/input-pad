#!/bin/sh

# Weblate warns Not Found data/alphabet.xml.in.in.h in input-pad.pot
# since intltool converts XML files to H files internally.
# This script trims .h in the comments in input-pad.pot until the intltool
# method is replaced with gettext one.

: ${ECHO:='/usr/bin/echo'}

PROGNAME=`basename $0`

usage()
{
    $ECHO -e \
"This script trims .h in the comments in input-pad.pot\n"                      \
"$PROGNAME [OPTIONS…] POTFILE\n"                                               \
"\n"                                                                           \
"OPTIONS:\n"                                                                   \
"-h, --help                       This help\n"                                 \
"\n"                                                                           \
"POTFILE:                         input-pad.pot\n"                             \
""
}

if [ "x$1" = "x--help" ] || [ "x$1" = "x-h" ] ; then
    usage
    exit 0
fi

POTFILE="$1"

if [ ! -f "$POTFILE" ] ; then
   $ECHO -e "Not Found $POTFILE\n"
    exit 1
fi

sed -e "/^#/s/\([^ ]*\)\.in\.h:\([0-9]*\)/\1.in:\2/" "$POTFILE"
