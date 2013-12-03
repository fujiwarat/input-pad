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
export CFLAGS=${CFLAGS:-"-Wall -g -O0 -Wl,--no-undefined"}
export CXXFLAGS="$CFLAGS"
./configure "$@" || exit 1
