#!/bin/sh
cat <<EOF
*******************************************************************
Configuring Fedora for cross-compiling multi-threaded 32-bit and
64-bit Windows programs with mingw.

For sector_hash
*******************************************************************

This script will configure a fresh Fedora system to compile with
mingw32 and 64. Please perform the following steps:

1. Install F17 or newer, running with you as an administrator.
For a VM:

1a - download the ISO for the 64-bit DVD (not the live media) from:
http://fedoraproject.org/en/get-fedora-options#formats
1b - Create a new VM using this ISO as the boot.
2. Plese put this CONFIGURE_F18.sh script in you home directory.

3. Run this script to configure the system to cross-compile sector_hash.

press any key to continue...
EOF
read

# exit if any command fails
set -e

#
# TRE
#
echo "Building and installing TRE for mingw"
TREVER=0.8.0
TREFILE=tre-$TREVER.tar.gz
TREDIR=tre-$TREVER
TREURL=http://laurikari.net/tre/$TREFILE

if [ ! -r $TREFILE ]; then
  wget $TREURL
fi
tar xfvz $TREFILE
pushd $TREDIR
for i in 32 64 ; do
echo
echo libtre mingw$i
  mingw$i-configure --enable-static --disable-shared
  make
  sudo make install
  make clean
done
popd
echo "TRE mingw installation complete."

##
## LIBXML2
# NO: using mingw package libxml2 static instead, above.
##
#
#echo "Building and installing libxml2 for mingw"
#LIBXML2DIR=libxml2
#LIBXML2URL=git://git.gnome.org/libxml2
#git clone $LIBXML2URL $LIBXML2DIR
#pushd $LIBXML2DIR
#for i in 32 64 ; do
#echo
#echo libxml2 mingw$i
#  mingw$i-configure --enable-static --disable-shared
#  make
#  sudo make install
#  make clean
#done
#popd
#echo "LIBXML2 mingw installation complete."

#
# ZMQ
#

echo "Building and installing zmq for mingw"
ZMQVER=3.2.2
ZMQFILE=zeromq-$ZMQVER.tar.gz
ZMQDIR=zeromq-$ZMQVER
ZMQURL=download.zeromq.org/$ZMQFILE

if [ ! -r $ZMQFILE ]; then
  wget $ZMQURL
fi
tar xfvz $ZMQFILE
patch -p1 <zmq-configure.in.patch
patch -p1 <zmq-zmq.h.patch
patch -p1 <zmq-zmq_utils.h.patch
pushd $ZMQDIR
sh autogen.sh
for i in 32 64 ; do
echo
echo zmq mingw$i
  mingw$i-configure --enable-static --disable-shared
  make
  sudo make install
  make clean
done
popd
echo "ZMQ mingw installation complete."

#
# ICU
#

echo "Building and installing ICU for mingw"
ICUVER=51_1
ICUFILE=icu4c-$ICUVER-src.tgz
ICUDIR=icu
ICUURL=http://download.icu-project.org/files/icu4c/51.1/$ICUFILE

if [ ! -r $ICUFILE ]; then
  wget $ICUURL
fi
tar xzf $ICUFILE
patch -p1 <icu-mingw32-libprefix.patch
patch -p1 <icu-mingw64-libprefix.patch

# build ICU for Linux to get packaging tools used by MinGW builds
echo
echo icu linux
rm -rf icu-linux
mkdir icu-linux
pushd icu-linux
CC=gcc CXX=g++ CFLAGS=-O3 CXXFLAGS=-O3 CPPFLAGS="-DU_USING_ICU_NAMESPACE=0 -DU_CHARSET_IS_UTF8=1 -DUNISTR_FROM_CHAR_EXPLICIT=explicit -DUNSTR_FROM_STRING_EXPLICIT=explicit" ../icu/source/runConfigureICU Linux --enable-shared --disable-extras --disable-icuio --disable-layout --disable-samples --disable-tests
make VERBOSE=1
popd

# build 32- and 64-bit ICU for MinGW
for i in 32 64 ; do
echo
echo icu mingw$i
  mkdir icu-mingw$i
  rm -rf icu-mingw$i
  pushd icu-mingw$i
  eval MINGW=\$MINGW$i
  eval MINGW_DIR=\$MINGW${i}_DIR
  ../icu/source/configure CC=$MINGW-gcc CXX=$MINGW-g++ CFLAGS=-O3 CXXFLAGS=-O3 CPPFLAGS="-DU_USING_ICU_NAMESPACE=0 -DU_CHARSET_IS_UTF8=1 -DUNISTR_FROM_CHAR_EXPLICIT=explicit -DUNSTR_FROM_STRING_EXPLICIT=explicit" --enable-static --disable-shared --prefix=$MINGW_DIR --host=$MINGW --with-cross-build=`realpath ../icu-linux` --disable-extras --disable-icuio --disable-layout --disable-samples --disable-tests --with-data-packaging=static --disable-dyload
  make VERBOSE=1
  sudo make install
  popd
done
echo "ICU mingw installation complete."

#
# closure
#

echo "Cleaning up"
rm -f $TREFILE $ZMQFILE $ICUFILE
rm -rf $TREDIR icu icu-linux icu-mingw32 icu-mingw64

echo ...
echo 'Now running ../bootstrap.sh and configure'
pushd ..
sh bootstrap.sh
sh configure
popd
echo ================================================================
echo ================================================================
echo 'You are now ready to cross-compile for win32 and win64'
echo 'to make 32-bit and 64-bit configurations of sector_hash,'
echo 'specifically, sector_hash.exe, sector_hash_lookup.exe, and scan_sectorid.dll.'
echo 'To make the 32-bit configuration: cd ..; make win32'
echo 'To make the 64-bit configuration: cd ..; make win64'
#echo 'To make ZIP file with both: cd ..; make windist'
