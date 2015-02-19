#!/bin/sh
# Hopefully you checked out with git clone git@github.com:simsong/hashdb.git

# have automake do an initial population iff necessary
if [ ! -e config.guess -o ! -e config.sub -o ! -e install-sh -o ! -e missing ]; then
    autoheader -f
    touch NEWS README AUTHORS ChangeLog
    touch stamp-h
    aclocal -I m4
    autoconf -f
    #libtoolize || glibtoolize
    automake --add-missing --copy
else
    automake --add-missing --copy
    autoreconf -f
fi
echo be sure to run ./configure
