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
Parts of this script will be run as root using "sudo".

press any key to continue...
EOF
read

MPKGS="autoconf automake gcc gcc-c++ wine zlib-devel wget md5deep git"
MPKGS+=" mingw32-gcc mingw32-gcc-c++"
MPKGS+=" mingw64-gcc mingw64-gcc-c++"

if [ ! -r /etc/redhat-release ]; then
echo This requires Fedora Linux
  exit 1
fi

if grep 'Fedora.release.' /etc/redhat-release ; then
echo Fedora Release detected
else
echo This script is only tested for Fedora Release 17 and should work on F17 or newer.
  exit 1
fi

echo "Will now try to install yum packages"

sudo yum install -y $MPKGS
if [ $? != 0 ]; then
echo "Could not install some of the packages.  Will not proceed."
  exit 1
fi

echo Attempting to install both DLL and static version of mingw libraries
echo required for building executables relating to sector_hash.
echo At this point we will keep going even if there is an error...
for M in mingw32 mingw64 ; do
for lib in zlib win-iconv libxml2 boost pthreads libgnurx openssl; do
echo ${M}-${lib} ${M}-${lib}-static
  done
done | xargs sudo yum -y install


echo
echo "Now performing a yum update to update system packages"
sudo yum -y update

echo
echo "Done installing yum packages"
echo "Now please type './CONFIGURE_F18_OTHER.sh'"
echo "to install packages not managed by yum"

