#!/bin/sh
set -e
set -x

libtoolize --automake --copy --force || exit 1
mkdir -p m4
intltoolize --copy --force || exit 1
aclocal --force || exit 1
autoheader --force || exit 1
automake --add-missing --copy --force || exit 1
autoconf --force || exit 1

if `echo "$@" | grep -q CFLAGS > /dev/null` ; then
    ./configure "$@" || exit 1
else
    CFLAGS=${CFLAGS-"-Wall -Wformat -Werror=format-security"}
    CXXFLAGS="$CFLAGS"
    ./configure "$@" CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS" || exit 1
fi
