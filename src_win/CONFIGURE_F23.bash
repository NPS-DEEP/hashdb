#!/bin/bash
cat <<EOF
*******************************************************************
        Configuring Fedora for cross-compiling multi-threaded
		 64-bit Windows programs with mingw.
*******************************************************************

This script will configure a fresh Fedora system to compile with
mingw64.  Please perform the following steps:

1. Install F20 or newer, running with you as an administrator.
   For a VM:

   1a - download the ISO for the 64-bit DVD (not the live media) from:
        http://fedoraproject.org/en/get-fedora-options#formats
   1b - Create a new VM using this ISO as the boot.
       
2. Run this script to configure the system to cross-compile bulk_extractor.
   Files will be installed and removed from this directory during this process.
   Parts of this script will be run as root using "sudo".

press any key to continue...
EOF
read

MPKGS="autoconf automake gcc gcc-c++ libtool "
MPKGS+="wine wget bison "
MPKGS+="libewf libewf-devel "
MPKGS+="openssl-devel "
MPKGS+="cabextract gettext-devel "
MPKGS+="mingw64-gcc mingw64-gcc-c++ "

if [ ! -r /etc/redhat-release ]; then
  echo This requires Fedora Linux
  exit 1
fi

if grep 'Fedora.release.' /etc/redhat-release ; then
  echo Fedora Release detected
else
  echo This script is only tested for Fedora Release 20 and should work on F20 or newer.
  exit 1
fi

echo Will now try to install 

sudo yum install -y $MPKGS
if [ $? != 0 ]; then
  echo "Could not install some of the packages. Will not proceed."
  exit 1
fi

echo Attempting to install both DLL and static version of all mingw libraries
echo needed for hashdb.
echo At this point we will keep going even if there is an error...
INST=""
# For these install both DLL and static
for lib in zlib bzip2 winpthreads openssl ; do
  INST+=" mingw64-${lib} mingw64-${lib}-static"
done
sudo yum -y install $INST

echo 
echo "Now performing a yum update to update system packages"
sudo yum -y update

# from here on, exit if any command fails
set -e

if [ ! -r /usr/x86_64-w64-mingw32/sys-root/mingw/lib/libewf.a ]; then
  LIBEWF_TAR_GZ=libewf-20140406.tar.gz
  LIBEWF_URL=https://googledrive.com/host/0B3fBvzttpiiSMTdoaVExWWNsRjg/$LIBEWF_TAR_GZ
  echo Building LIBEWF

  if [ ! -r $LIBEWF_TAR_GZ ]; then
    wget --content-disposition $LIBEWF_URL
  fi
  tar xvf $LIBEWF_TAR_GZ

  # get the directory that it unpacked into
  DIR=`tar tf $LIBEWF_TAR_GZ |head -1`

  # build and install LIBEWF in mingw
  pushd $DIR
  echo
  echo %%% $LIB mingw64

  # patch libewf/libuna/libuna_inline.h to work with GCC5
  patch -p0 <../libewf-20140406-gcc5-compatibility.patch

  # patch libewf/libewf/libewf.c to prevent generation of DllMain
  patch -p0 <../libewf-20140406-no-dllmain.patch

  # configure
  CPPFLAGS=-DHAVE_LOCAL_LIBEWF mingw64-configure --enable-static --disable-shared
  make clean
  make
  sudo make install
  make clean
  popd
  rm -rf $DIR
fi

# build pexports on Linux
if [ ! -r /usr/local/bin/pexports ]; then
  echo "Building and installing pexports"
  rm -rf pexports-0.47 pexports-0.47-mingw32-src.tar.zx
  wget https://sourceforge.net/projects/mingw/files/MinGW/Extension/pexports/pexports-0.47/pexports-0.47-mingw32-src.tar.xz

  tar xf pexports-0.47-mingw32-src.tar.xz

  cd pexports-0.47
  ./configure
  make
  sudo make install
  cd ..
  rm -rf pexports-0.47 pexports-0.47-mingw32-src.tar.zx
fi

#
#
#

echo ================================================================
echo ================================================================
echo 'You are now ready to cross-compile for win64.'
echo 'For example: cd hashdb; mkdir win64; cd win64; mingw64-configure; make'
