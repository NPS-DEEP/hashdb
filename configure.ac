                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# Order is largely irrevellant, although it must start with AC_INIT and end with AC_OUTPUT
# See http://autotoolset.sourceforge.net/tutorial.html
# and http://www.openismus.com/documents/linux/automake/automake.shtml

AC_PREREQ(2.57)

AC_INIT(hashdb, 1.0.0-beta-1)
AC_CONFIG_MACRO_DIR(m4)

# NOTE - src_win/Makefile is not automatically generated. This is necessary
# so that it can be run even after a 'make distclean' is done
AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/libhashdb/Makefile
                 src/test/Makefile
                 man/Makefile
                 doc/Makefile])

# define CC and CXX as desired for alternate compilers
#CC=clang
#CXX=clang++

AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE
AC_PREFIX_PROGRAM
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LIBTOOL

m4_include([src/dfxml/src/dfxml_configure.m4])

# Endian check is required for MD5 implementation
AC_C_BIGENDIAN

##
## optional NSIS Windows Installer
AC_CHECK_PROG(has_nsis, makensis, true, false)
if test x"$has_nsis" == "xfalse" ; then
  AC_MSG_NOTICE([nsis is not available for creating the Windows installer.  Try 'apt-get install mingw32-nsis' or 'yum install mingw32-nsis' or 'port install mingw32-nsis' or whatever package manager you happen to be using....])
fi

##
## Standard macros
##
AC_TYPE_INT64_T
AC_SYS_LARGEFILE
AC_SYS_POSIX_TERMIOS

################################################################
## See if we are running on mingw
# http://osdir.com/ml/gnu.mingw.devel/2003-09/msg00040.html
# Note: Windows 95 WINVER=0x400
# Windows 98 WINVER=0x400  _WIN32_WINDOWS=0x0410
# Windows Me WINVER=0x400  _WIN32_WINDOWS=0x0490
# Windows NT 4.0 WINVER=0x0400  _WIN32_WINNT=0x0400
# Windows NT 4.0 SP3 WINVER=0x0400 _WIN32_WINNT=0x0403
# Windows 2000 WINVER=0x500 _WIN32_WINNT=0x0500
# Windows XP WINVER=0x501 _WIN32_WINNT=0x0501
# Windows Server 2003 WINVER=0x502 _WIN32_WINNT=0x0502
#
# mingw32 includes  i686-w64-mingw32 and  x86_64-w64-mingw32

mingw="no"
case $host in
  *-*-*linux*-*) 
     AC_DEFINE([__LINUX__],1,[Linux operating system functions])
     ;;

  *mingw32*)   
     LIBS="$LIBS -lpsapi -lws2_32 -lgdi32 -lstdc++"  
     CPPFLAGS="-DUNICODE -D_UNICODE -D__MSVCRT_VERSION__=0x0601 -DWINVER=0x0500 -D_WIN32_WINNT=0x0500 -g $CPPFLAGS"
     CPPFLAGS="$CPPFLAGS --static "
     CFLAGS="$CFLAGS --static -static-libgcc -static-libstdc++"
     CXXFLAGS="$CXXFLAGS -Wno-format "  # compiler mingw-4.3.0 is broken on I64u formats
     CXXFLAGS="$CXXFLAGS --static -static-libgcc -static-libstdc++"
     LDFLAGS="$LDFLAGS --static"
     mingw="yes"
     ;;		 		     
esac

if test x"${mingw}" == "xno" ; then
  # Bring additional directories where things might be found into our
  # search path. I don't know why autoconf doesn't do this by default
  for spfx in /usr/local /opt/local /sw /usr/local/ssl; do
    AC_MSG_NOTICE([checking ${spfx}/include])
    if test -d ${spfx}/include; then
        CPPFLAGS="-isystem${spfx}/include $CPPFLAGS"
        LDFLAGS="-L${spfx}/lib $LDFLAGS"
        AC_MSG_NOTICE([ *** ADDING ${spfx}/include to CPPFLAGS *** ])
        AC_MSG_NOTICE([ *** ADDING ${spfx}/lib to LDFLAGS *** ])
    fi
  done
  AC_MSG_NOTICE([ CPPFLAGS = ${CPPFLAGS} ])        
  AC_MSG_NOTICE([ LDFLAGS = ${LDFLAGS} ])        
fi

if test -r /bin/uname.exe ; then
  if test `uname -o` == "Msys" ; then
    AC_MSG_NOTICE([Compiling with Msys. Setting flags appropriately.])
    LIBS="$LIBS -lws2_32 -lgdi32"
    LDFLAGS="$LDFLAGS -Wl,--enable-auto-import"
  fi
fi
################################################################
#
# Enable all the compiler debugging we can find
#
# This is originally from PhotoRec, but modified substantially by Simson
# Figure out which flags we can use with the compiler. 
#
# These I don't like:
# -Wdeclaration-after-statement -Wconversion
# doesn't work: -Wunreachable-code 
# causes configure to crash on gcc-4.2.1: -Wsign-compare-Winline 
# causes warnings with unistd.h:  -Wnested-externs 
# Just causes too much annoyance: -Wmissing-format-attribute 

# Check GCC
WARNINGS_TO_TEST="-MD -Wpointer-arith -Wmissing-declarations -Wmissing-prototypes \
    -Wshadow -Wwrite-strings -Wcast-align -Waggregate-return \
    -Wbad-function-cast -Wcast-qual -Wundef -Wredundant-decls -Wdisabled-optimization \
    -Wfloat-equal -Wmultichar -Wc++-compat -Wmissing-noreturn -funit-at-a-time "

if test x"${mingw}" == "xno" ; then
  # add the warnings we do not want to do on mingw
  WARNINGS_TO_TEST="$WARNINGS_TO_TEST -Wall -Wstrict-prototypes"
fi

echo "Warnings to test: $WARNINGS_TO_TEST"

for option in $WARNINGS_TO_TEST
do
  SAVE_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS $option"
  AC_MSG_CHECKING([whether gcc understands $option])
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
      [has_option=yes],
      [has_option=no; CFLAGS="$SAVE_CFLAGS"])
  AC_MSG_RESULT($has_option)
  unset has_option
  unset SAVE_CFLAGS
  if test $option = "-Wmissing-format-attribute" ; then
    AC_DEFINE(HAVE_MISSING_FORMAT_ATTRIBUTE_WARNING,1,
		[Indicates that we have the -Wmissing-format-attribute G++ warning])
  fi
done
unset option

################################################################
## mcheck support
## enable memory analysis support for detecting memory leaks
## and detecting multiply deallocated memory resources
## see http://en.wikipedia.org/wiki/Mtrace
## here is the basic workflow:
##   1. example usage and recompile: "./configure --enable-mcheck"
##   2. export the environment variable for the output file,
##        e.g. "export MALLOC_TRACE=~/malloc_outfile.txt"
##   3. improve readability of malloc_outfile.txt using mtrace,
##        e.g. "mtrace /usr/local/bin/bulk_extractor ~/malloc_outfile.txt
AC_ARG_ENABLE([mcheck], AS_HELP_STRING([--enable-mcheck], \
              [enable memory analysis support for detecting memory leaks \
               and detecting multiply deallocated memory resources]), \
              AC_DEFINE(HAVE_MCHECK, 1, \
              [define the output file using global variable MALLOC_TRACE, \
               for example "MALLOC_TRACE=./mcheck.out; export MALLOC_TRACE". \
               Then post-process unreadable file mcheck.out using the mtrace command line tool, \
               for example "mtrace /usr/local/bin/bulk_extractor mcheck.out".  \
               Unfortunately, only addresses with written code are looked up, \
               in otherwords, use of malloc is looked up, but use of new is not.]))


################################################################
# Check G++
# We don't use these warnings:
# -Waggregate-return -- aggregate returns are GOOD; they simplify code design
# We can use these warnings after ZLIB gets upgraded:
# -Wundef  --- causes problems with zlib
# -Wcast-qual 
# -Wmissing-format-attribute  --- Just too annoying
AC_LANG_PUSH(C++)
WARNINGS_TO_TEST="-Wall -MD -Wpointer-arith \
    -Wshadow -Wwrite-strings -Wcast-align  \
    -Wredundant-decls -Wdisabled-optimization \
    -Wfloat-equal -Wmultichar -Wmissing-noreturn \
    -Woverloaded-virtual -Wsign-promo -funit-at-a-time"

if test x$CXX != "xclang++" ; then
  # -Wstrict-null-sentinel is not supported under clang
  WARNINGS_TO_TEST="$WARNINGS_TO_TEST -Wstrict-null-sentinel"
fi

if test x"${mingw}" == "xno" ; then
  # add the warnings we don't want to do on mingw
  WARNINGS_TO_TEST="$WARNINGS_TO_TEST  -Weffc++"
fi

for option in $WARNINGS_TO_TEST
do
  SAVE_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS $option"
  AC_MSG_CHECKING([whether g++ understands $option])
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
      [has_option=yes],
      [has_option=no; CXXFLAGS="$SAVE_CXXFLAGS"])
  AC_MSG_RESULT($has_option)
  unset has_option
  unset SAVE_CXXFLAGS
done
unset option
AC_LANG_POP()    

# Determine UTC date offset
CPPFLAGS="$CPPFLAGS -DUTC_OFFSET=`date +%z`"

#################################################################
## PTHREAD support required by bloom.h
## With special nods to compiling under mingw
#
#if test  x"$mingw" = x"yes";  then
#  #AC_DEFINE([HAVE_STRUCT_TIMESPEC],1,[Required for mingw])
#  CFLAGS="$CFLAGS -mthreads "
#  CPPFLAGS="-DPTW32_STATIC_LIB $CPPFLAGS"
#  CXXFLAGS="$CXXFLAGS -mthreads "
#  AC_DEFINE(HAVE_PTHREAD,1,[Defined to POSIX threads for mingw])
#else
#  m4_include([m4/ax_pthread.m4])
#  AC_MSG_CHECKING([Running pthread test from http://www.gnu.org/software/autoconf-archive/ax_pthread.html..])
#  AX_PTHREAD([
#              echo Using settings from AX_PTHREAD
#              LIBS="$LIBS $PTHREAD_LIBS"
#              CFLAGS="  $PTHREAD_CFLAGS $CFLAGS"
#              CXXFLAGS="$PTHREAD_CFLAGS $CXXFLAGS "
#              CC="$PTHREAD_CC"],
#            [AC_MSG_ERROR(hashdb requires pthreads)])
#fi
#
#AC_CHECK_HEADERS([pthread.h])
#AC_CHECK_LIB([pthreadGC2],[pthread_create])

################################################################
# PTHREAD support
# With special nods to compiling under mingw

if test  x"$mingw" = x"yes";  then
  #AC_DEFINE([HAVE_STRUCT_TIMESPEC],1,[Required for mingw])
  CFLAGS="$CFLAGS -mthreads "
  CPPFLAGS="-DPTW32_STATIC_LIB $CPPFLAGS"
  CXXFLAGS="$CXXFLAGS -mthreads "

  AC_DEFINE(HAVE_PTHREAD,1,[Defined to POSIX threads for mingw])
else
  m4_include([m4/ax_pthread.m4])
  AC_MSG_CHECKING([Running pthread test from http://www.gnu.org/software/autoconf-archive/ax_pthread.html..])
  AX_PTHREAD([
              echo Using settings from AX_PTHREAD
              LIBS="$PTHREAD_LIBS $LIBS"
              CFLAGS="  $PTHREAD_CFLAGS $CFLAGS"
              CXXFLAGS="$PTHREAD_CFLAGS $CXXFLAGS "
              CC="$PTHREAD_CC"],
            [AC_MSG_ERROR(bulk_extractor requires pthreads)])
fi

AC_CHECK_HEADERS([pthread.h])
AC_CHECK_LIB([pthreadGC2],[pthread_create])

################################################################
##
## Multithreading stuff

# On mingw, be sure to use the static version and be sure we are using mthread option
# (which should be a no-op on later version of G++ anyway)

AC_CHECK_FUNCS([pthread_win32_process_attach_np pthread_win32_process_detach_np pthread_win32_thread_attach_np pthread_win32_thread_detach_np ])

AC_MSG_CHECKING([for __sync_fetch_and_add])
AC_LINK_IFELSE(
   [AC_LANG_SOURCE([
    int main(void) { int i; return __sync_fetch_and_add(&i, 0); }
   ])],
   [
     AC_MSG_RESULT([yes])
     AC_DEFINE(HAVE___SYNC_FETCH_AND_ADD,1,[Defined if __sync_fetch_and_add was detected])
    ],
   [AC_MSG_RESULT([no])]
)


# end PTHREAD SUPPORT
################################################################

################################################################
############## drop optimization flags and add -g if requested ################
if test x"${AFF_NOOPT}" != "x" ; then
   with_noopt="yes";
fi

if test "${with_opt}" = "no" ; then
  CFLAGS=`echo   -g "$CFLAGS"   | sed s/-O[[0-9]]//`             # note the double quoting! 
  CXXFLAGS=`echo -g "$CXXFLAGS" | sed s/-O[[0-9]]//`
else
  # If we are not stripping the optimizer, add fortify source
  CFLAGS="  $CFLAGS   -D_FORTIFY_SOURCE=2 "
  CXXFLAGS="$CXXFLAGS -D_FORTIFY_SOURCE=2 "

  # and increase optimizer from -O2 to -O3 if not explicitly forbidden
  if test "${with_o3}" != "no" ; then
      CFLAGS=`echo   -g "$CFLAGS"   | sed s/-O2/-O3/`             # note the double quoting! 
      CXXFLAGS=`echo -g "$CXXFLAGS" | sed s/-O2/-O3/`
  fi
fi

################################################################
## support required by libxml2

# zzzzzzzzzzzzz
# note: a recommended approach is to use PKG_CHECK_MODULES:
#          PKG_CHECK_MODULES(XML, libxml-2.0)
# but this requires compiling with per-target flags,
# requiring 'AM_PROG_CC_C_O' and the addition of
# XML_CFLAGS to hashdb_CPPFLAGS and XML_LIBS to hashdb_LDFLAGS,
# which may be making mingw not work by disrupting names
# and blocking the ability to find libraries.
#
# So instead, check for headers and libraries in the hardcoded path
# expected for a standard Fedora installation
# or hardwaire LDFLAGS for your path:
# zzzzzzzzzzzzz

# the win32 configuration of libxml2 requires zlib and iconv libraries
if test  x"$mingw" = x"yes";  then
  AC_CHECK_LIB([z], [gzdopen],[LIBS="-lz $LIBS"], [AC_MSG_ERROR([Could not find zlib library])])
  AC_CHECK_LIB([iconv], [iconv],[LIBS="-liconv $LIBS"], [AC_MSG_ERROR([Could not find win-iconv library])])
fi

# find the libxml/parser.h header
PKG_CHECK_MODULES(XML, libxml-2.0 >= 2.4, [], [AC_MSG_ERROR([libxml2 failed])])

# find the xml2 library
AC_CHECK_LIB([xml2], [xmlSAXUserParseFile],[LIBS="-lxml2 $LIBS"],
    [AC_MSG_ERROR([Could not find libxml2 library required to build the hashid scanner])])


################################################################
## support required by zmq
AC_CHECK_HEADERS([zmq.h],,[AC_MSG_ERROR([Could not find zmq.h, zeromq v3 is required])])
AC_CHECK_LIB([zmq],[zmq_ctx_new],[LIBS="-lzmq $LIBS"],[AC_MSG_ERROR([Could not find zeromq, zeromq v3 is required])])
AC_CHECK_FUNCS([zmq_version],,[AC_MSG_ERROR([test fails])])

################################################################
## support required by boost
AX_BOOST_BASE([1.53],
  [AX_BOOST_SYSTEM
  AX_BOOST_FILESYSTEM
  AX_BOOST_IOSTREAMS
  LIBS="$BOOST_SYSTEM_LIB $BOOST_FILESYSTEM_LIB $BOOST_IOSTREAMS_LIB $LIBS"],
  [AC_MSG_ERROR([Boost 1.53 or newer is required to build hashdb])])

################################################################
## OpenSSL Support is required for hash_t and bloom
AC_CHECK_HEADERS([openssl/aes.h openssl/bio.h openssl/evp.h openssl/hmac.h openssl/md5.h openssl/pem.h openssl/rand.h openssl/rsa.h openssl/sha.h openssl/pem.h openssl/x509.h])
# OpenSSL has been installed under at least two different names...
AC_CHECK_LIB([crypto],[EVP_get_digestbyname])	
AC_CHECK_LIB([ssl],[SSL_library_init])
AC_CHECK_FUNCS([EVP_get_digestbyname],,
	AC_MSG_ERROR([SSL/OpenSSL support required]))

################################################################
## support required by bloom.h
AC_CHECK_FUNCS([printf getrusage err errx warn warnx mmap],,[AC_MSG_WARN([Not all requested functions are available: printf, getrusage...])])
AC_CHECK_HEADERS([err.h sys/mman.h sys/resource.h unistd.h],,[AC_MSG_WARN([Not all requested headers are available: err.h, sys/mman.h, ...])])
#PTHREAD: AC_CHECK_HEADERS([pthread.h],,[AC_MSG_ERROR([pthread headers are required])])
# note that crypto requires zlib
AC_CHECK_LIB([crypto],[EVP_Digest],[],[AC_MSG_ERROR([Could not find crypto library])])
LIBS="-lcrypto $LIBS"

#PTHREAD: AC_CHECK_LIB([pthread],[pthread_mutexattr_settype],[],[AC_MSG_ERROR([Could not find pthread library])])
#LIBS="-lssl -lcrypto -lpthread $LIBS"
#LIBS="-lcrypto -lpthread $LIBS"
## end support required by bloom.h

# Should we disable optimization?
AC_ARG_WITH([opt], AC_HELP_STRING([--without-opt], [Drop all -O C flags]))
# Or maybe just tone it down a bit?
AC_ARG_WITH([o3], AC_HELP_STRING([--without-o3], [Do not force O3 optimization; use default level]))

################################################################
## regex support
## there are several options
## tre is better than regex
AC_CHECK_LIB([stdc++],[main])
AC_CHECK_LIB([sicudt],[main])
AC_CHECK_HEADER([unicode/ucnv.h])
AC_SEARCH_LIBS([uset_close], [sicuuc])
AC_CHECK_LIB([sicuuc],[main])
#AC_CHECK_HEADER([lightgrep/api.h])
#AC_CHECK_LIB([lightgrep], [lg_create_pattern])
AC_CHECK_HEADERS([regex.h tre/tre.h])
AC_CHECK_LIB([regex],[regcomp])
AC_CHECK_LIB([tre.a],[tre_regcomp])
AC_CHECK_FUNCS([regcomp tre_regcomp tre_version])

################################################################
## support required by be13_api required by DFXML
AC_CHECK_HEADERS([sys/cdefs.h sys/resource.h pwd.h sys/utsname.h])
AC_CHECK_FUNCS([localtime_r getuid gethostname getwpuid getrusage mkstemp vasprintf])

################################################################
## libtool required for preparing the hashdb library
AC_CHECK_PROG(has_libtool, libtool, true, false)
if test x"$has_libtool" == "xfalse" ; then
  AC_MSG_ERROR([libhashdb requires libtool, please install libtool such as The GNU Portable Library Tool])
fi

################################################################
## diagnostic defines
AC_TRY_COMPILE([#pragma GCC diagnostic ignored "-Wshadow"],[int a=3;],
  [AC_DEFINE(HAVE_DIAGNOSTIC_SHADOW,1,[define 1 if GCC supports -Wshadow])]
)
AC_TRY_COMPILE([#pragma GCC diagnostic ignored "-Wundef"],[int a=3;],
  [AC_DEFINE(HAVE_DIAGNOSTIC_UNDEF,1,[define 1 if GCC supports -Wundef])]
)
AC_TRY_COMPILE([#pragma GCC diagnostic ignored "-Wcast-qual"],[int a=3;],
  [AC_DEFINE(HAVE_DIAGNOSTIC_CAST_QUAL,1,[define 1 if GCC supports -Wcast-qual])]
)
AC_TRY_COMPILE([#pragma GCC diagnostic ignored "-Weffcpp"],[int a=3;],
  [AC_DEFINE(HAVE_DIAGNOSTIC_EFFCPP,1,[define 1 if GCC supports -Weffc++])]
)
AC_TRY_COMPILE([#pragma GCC diagnostic ignored "-Wsuggest-attribute=noreturn"],[int a=3;],
  [AC_DEFINE(HAVE_DIAGNOSTIC_SUGGEST_ATTRIBUTE,1,[define 1 if GCC supports -Wsuggest-attribute=noreturn])]
)
AC_TRY_COMPILE([#pragma GCC diagnostic ignored "-Wredundant-decls"],[int a=3;],
  [AC_DEFINE(HAVE_DIAGNOSTIC_REDUNDANT_DECLS,1,[define 1 if GCC supports -Wredundant-decls])]
)

################################################################
# do we have a broken strchr?
# This prevents errors in base64_forensics.cpp
AC_LANG_PUSH(C++)
AC_TRY_COMPILE([#include <string.h>],
               [static const char Base64[] = "ABCDEFGH";int ch=3;char *pos = strchr(Base64, ch);],
               conforming_strchr=yes,
               conforming_strchr=no)
if test "$conforming_strchr" = yes; then
  AC_MSG_NOTICE([strchr is conforming])
  AC_DEFINE(HAVE_CONFORMING_STRCHR, 1,[define to 1 if strchr takes const char*])
else
  AC_MSG_NOTICE([strchr is not conforming])
fi
AC_LANG_POP()
################################################################
#
# Do we have the CPUID instruction?
AC_TRY_COMPILE([#define cpuid(id) __asm__( "cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(id), "b"(0), "c"(0), "d"(0))],
			[unsigned long eax, ebx, ecx, edx;cpuid(0);],
			have_cpuid=yes,
			have_cpuid=no)
if test "$have_cpuid" = yes; then
 AC_DEFINE(HAVE_ASM_CPUID, 1, [define to 1 if __asm__ CPUID is available])
fi

################################################################
############## drop optimization flags and add -g if requested ################
if test x"${AFF_NOOPT}" != "x" ; then
   with_noopt="yes";
fi

if test "${with_opt}" = "no" ; then
  CFLAGS=`echo   -g "$CFLAGS"   | sed s/-O[[0-9]]//`             # note the double quoting! 
  CXXFLAGS=`echo -g "$CXXFLAGS" | sed s/-O[[0-9]]//`
else
  # If we are not stripping the optimizer, add fortify source
  CFLAGS="  $CFLAGS   -D_FORTIFY_SOURCE=2 "
  CXXFLAGS="$CXXFLAGS -D_FORTIFY_SOURCE=2 "

  # and increase optimizer from -O2 to -O3 if not explicitly forbidden
  if test "${with_o3}" != "no" ; then
      CFLAGS=`echo   -g "$CFLAGS"   | sed s/-O2/-O3/`             # note the double quoting! 
      CXXFLAGS=`echo -g "$CXXFLAGS" | sed s/-O2/-O3/`
  fi
fi

AC_MSG_NOTICE([*************************************])
AC_MSG_NOTICE([*************************************])
AC_MSG_NOTICE([  PACKAGE_NAME:     $PACKAGE_NAME])
AC_MSG_NOTICE([  PACKAGE_VERSION:  $PACKAGE_VERSION])
AC_MSG_NOTICE([  CC:               $CC])
AC_MSG_NOTICE([  CXX:              $CXX])
AC_MSG_NOTICE([  CPPFLAGS:         $CPPFLAGS])
AC_MSG_NOTICE([  CFLAGS:           $CFLAGS])
AC_MSG_NOTICE([  CXXFLAGS:         $CXXFLAGS])
AC_MSG_NOTICE([  LIBS:             $LIBS])
AC_MSG_NOTICE([  LDFLAGS:          $LDFLAGS])

AC_OUTPUT

## Finally, record the values of CFLAGS, CPPFLAGS, and CXXFLAGS for DFXML
echo "#define CPPFLAGS \"$CPPFLAGS\"" >> config.h
echo "#define CFLAGS   \"$CFLAGS\""   >> config.h
echo "#define CXXFLAGS \"$CXXFLAGS\"" >> config.h
echo "#define LIBS     \"$LIBS\""     >> config.h
echo "#define LDFLAGS  \"$LDFLAGS\""  >> config.h

