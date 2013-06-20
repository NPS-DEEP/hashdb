#!/bin/sh
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

# the git dfxml module is required in src/ path
if [ ! -r src/dfxml ] ;
then
  # until hashdb is on git, the dfxml module must be downloaded separately
  echo bringing in the dfxml submodule from git...
  cd src; git clone https://github.com/simsong/dfxml.git; cd ..

#  echo bringing in submodules
#  echo next time check out with git clone --recursive
#  git submodule init
fi

