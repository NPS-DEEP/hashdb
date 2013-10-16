#!/bin/sh

# validate that requisite submodules are present
if [ ! -r src/dfxml/.git ] ;
then
  echo bringing in submodules
  echo next time check out with git clone --recursive
  git submodule init
fi

if [ ! -r src/btree/.git ] ;
then
  echo btree submodule error.  Please check out with git clone --recursive
  exit 1
fi

if [ ! -r src/endian/.git ] ;
then
  echo endian submodule error.  Please check out with git clone --recursive
  exit 1
fi

/bin/rm -rf aclocal.m4
autoheader -f
aclocal -I m4

# libtoolize is required
if $(type -p libtoolize); then
  libtoolize
elif  $(type -p glibtoolize); then
  glibtoolize
else
  echo "Please install libtoolize or glibtoolize to run this script"
fi

autoconf -f
automake --add-missing --copy

