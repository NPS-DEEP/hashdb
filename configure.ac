#                                  -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# Order is largely irrevellant, although it must start with AC_INIT and end with AC_OUTPUT
# See http://autotoolset.sourceforge.net/tutorial.html
# and http://www.openismus.com/documents/linux/automake/automake.shtml

AC_PREREQ(2.57)

AC_INIT(hashdb, 3.0.0-dev-0, github.com/NPS-DEEP/hashdb/issues)
AC_CONFIG_MACRO_DIR(m4)

AC_CONFIG_FILES([Makefile
                 src_libhashdb/Makefile
                 src/Makefile
                 src_win/Makefile
                 test/Makefile
                 test_py/Makefile
                 python_bindings/Makefile
                 man/Makefile
                 doc/Makefile])


# define CC and CXX as desired for alternate compilers
# for example "./configure CC=clang CXX=clang++"
#CC=clang
#CXX=clang++

AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([subdir-objects])
AC_PREFIX_PROGRAM(hashdb)
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LIBTOOL
LT_INIT([win32-dll])
AC_DISABLE_STATIC

# use C++11 mode if available; HAVE_CXX11 is defined in config.h if so.  Don't
# use the GNU C++11 extensions for portability's sake (noext).
# https://www.gnu.org/software/autoconf-archive/ax_cxx_compile_stdcxx_11.html
m4_include([m4/ax_cxx_compile_stdcxx_11.m4])
AX_CXX_COMPILE_STDCXX_11(noext, optional)

# Endian check is required for MD5 implementation
AC_C_BIGENDIAN

################################################################
## Includes
m4_include([m4/slg_searchdirs.m4])
m4_include([m4/hashdb_cflags.m4])

################################################################
## establish default Python for tests and for SWIG
AM_PATH_PYTHON([2.7],, [:])
AM_CONDITIONAL([HAVE_PYTHON], [test "$PYTHON" != :])
if test "$PYTHON" == : ; then
  AC_MSG_NOTICE([Python is not available.])
fi

################################################################
## MinGW
## Note: may need work
mingw="no"
case $host in
  *-*-*linux*-*)
     AC_DEFINE([__LINUX__],1,[Linux operating system functions])
     ;;

  *mingw*)
     LIBS="$LIBS -lpsapi -lws2_32 -lgdi32"
     CPPFLAGS="-D__MSVCRT_VERSION__=0x0601 -DWINVER=0x0500 -D_WIN32_WINNT=0x0500 -g $CPPFLAGS"
     CPPFLAGS="$CPPFLAGS --static "
     CFLAGS="$CFLAGS --static -static-libgcc -static-libstdc++"
     CXXFLAGS="$CXXFLAGS -Wno-format "  # compiler mingw-4.3.0 is broken on I64u formats
     CXXFLAGS="$CXXFLAGS --static -static-libgcc -static-libstdc++"
     LDFLAGS="$LDFLAGS --static"
     mingw="yes"
     ;;
esac

## if using mingw then require NSIS and set HAVE_MINGW to build src_win/
AM_CONDITIONAL(HAVE_MINGW, [test x"$mingw" = x"yes"])

## SWIG
has_swig="no"
if test x"$mingw" != x"yes" ; then
  # not mingw so try to load SWIG
  m4_include([m4/ax_pkg_swig.m4])
  m4_include([m4/ax_python_devel.m4])
  m4_include([m4/ax_swig_enable_cxxx.m4])
  m4_include([m4/ax_swig_python.m4])
  AX_PKG_SWIG(2.0.11, [has_swig=yes], [ AC_MSG_NOTICE([SWIG is required to build Python bindings]) ])
  AX_SWIG_PYTHON
  AX_SWIG_ENABLE_CXX
fi
AM_CONDITIONAL(HAVE_SWIG, [test x"$has_swig" = x"yes"])

################################################################

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
## AddressSanitizer support
AC_ARG_ENABLE([address-sanitizer], AS_HELP_STRING([--enable-address-sanitizer], \
              [enabled AddressSanitizer support for detecting a wide variety of \
               memory allocation and deallocation errors]), \
              [AC_DEFINE(HAVE_ADDRESS_SANITIZER, 1, [enable AddressSanitizer])
              CFLAGS="$CFLAGS -fsanitize=address"
              CXXFLAGS="$CXXFLAGS -fsanitize=address"
              AC_TRY_COMPILE([],[const int i=0;],[AC_MSG_NOTICE([Address Sanitizer Enabled])],
                                                 [AC_MSG_ERROR([Address Sanitizer not available])])
              ])

################################################################
# PTHREAD support
# With special nods to compiling under mingw

case $host in
 *mingw*)
  AC_MSG_NOTICE([mingw pthreads support])
  CFLAGS="$CFLAGS -mthreads -pthread "
  CPPFLAGS="-DPTW32_STATIC_LIB $CPPFLAGS"
  CXXFLAGS="$CXXFLAGS -mthreads -pthread "
  LDFLAGS="$LDFLAGS -mthreads -pthread "
#  LDFLAGS="$LDFLAGS -mthreads -pthread -Wl,--no-undefined "
  AC_DEFINE(HAVE_PTHREAD,1,[Defined to POSIX threads for mingw])
 ;;
 *)
  AC_MSG_NOTICE([normal pthreads support])
  m4_include([m4/ax_pthread.m4])
  AC_MSG_CHECKING([Running pthread test from http://www.gnu.org/software/autoconf-archive/ax_pthread.html..])
  AX_PTHREAD([
              echo Using settings from AX_PTHREAD
              LIBS="$PTHREAD_LIBS $LIBS"
              CFLAGS="  $PTHREAD_CFLAGS $CFLAGS"
              CXXFLAGS="$PTHREAD_CFLAGS $CXXFLAGS "
              CC="$PTHREAD_CC"],
            [AC_MSG_ERROR(bulk_extractor requires pthreads)])
esac

AC_MSG_NOTICE([After pthread test, CFLAGS: $CFLAGS])
AC_MSG_NOTICE([After pthread test, CXXFLAGS: $CXXFLAGS])
AC_MSG_NOTICE([After pthread test, LDFLAGS: $LDFLAGS])

AC_CHECK_HEADERS([pthread.h])
AC_CHECK_LIB([pthreadGC2],[pthread_create])

################################################################
##
## Multithreading stuff

# On mingw, be sure we are using mthread option
# (which should be a no-op on later version of G++ anyway)

AC_CHECK_FUNCS([pthread_win32_process_attach_np pthread_win32_process_detach_np pthread_win32_thread_attach_np pthread_win32_thread_detach_np ])

# end PTHREAD SUPPORT
################################################################

################################################################
# support for mallinfo for malloc usage reporting
AC_CHECK_HEADERS([malloc.h])
AC_CHECK_LIB([stdc++],[mallinfo],
  [AC_DEFINE(HAVE_MALLINFO,1,[define 1 if stdc++ supports mallinfo])]
  ,[])

# Specific functions, may not be required
AC_TYPE_INT64_T
AC_SYS_LARGEFILE
AC_SYS_POSIX_TERMIOS


#################################################################
############### drop optimization flags and add -g if requested ################
#if test x"${AFF_NOOPT}" != "x" ; then
#   with_noopt="yes";
#fi
#
#if test "${with_opt}" = "no" ; then
#  CFLAGS=`echo   -g "$CFLAGS"   | sed s/-O[[0-9]]//`             # note the double quoting!
#  CXXFLAGS=`echo -g "$CXXFLAGS" | sed s/-O[[0-9]]//`
#else
#  # If we are not stripping the optimizer, add fortify source
#  CFLAGS="  $CFLAGS   -D_FORTIFY_SOURCE=2 "
#  CXXFLAGS="$CXXFLAGS -D_FORTIFY_SOURCE=2 "
#
#  # and increase optimizer from -O2 to -O3 if not explicitly forbidden
#  if test "${with_o3}" != "no" ; then
#      CFLAGS=`echo   -g "$CFLAGS"   | sed s/-O2/-O3/`             # note the double quoting!
#      CXXFLAGS=`echo -g "$CXXFLAGS" | sed s/-O2/-O3/`
#  fi
#fi

################################################################
## OpenSSL Support is required for md5 hash

if test  x"$mingw" = x"yes";  then
  AC_CHECK_LIB([z], [gzdopen],[LIBS="-lz $LIBS"], [AC_MSG_ERROR([Could not find zlib library])])
fi

AC_CHECK_HEADERS([openssl/aes.h openssl/bio.h openssl/evp.h openssl/hmac.h openssl/md5.h openssl/pem.h openssl/rand.h openssl/rsa.h openssl/sha.h openssl/pem.h openssl/x509.h])
# OpenSSL has been installed under at least two different names...
AC_CHECK_LIB([crypto],[EVP_get_digestbyname])	
AC_CHECK_LIB([ssl],[SSL_library_init])
AC_CHECK_FUNCS([EVP_get_digestbyname],,
	AC_MSG_ERROR([SSL/OpenSSL support required]))

################################################################
## support required by LMDB
if test  x"$mingw" != x"yes";  then
  AC_CHECK_FUNCS([mmap])
  AC_CHECK_HEADERS([sys/mman.h],,[AC_MSG_ERROR([mmap support required])])
fi

#AC_CHECK_FUNCS([printf getrusage err errx warn warnx mmap],,[AC_MSG_WARN([Not all requested functions are available: printf, getrusage...])])
#AC_CHECK_HEADERS([err.h sys/mman.h sys/resource.h unistd.h],,[AC_MSG_WARN([Not all requested headers are available: err.h, sys/mman.h, ...])])
#PTHREAD: AC_CHECK_HEADERS([pthread.h],,[AC_MSG_ERROR([pthread headers are required])])

# note that crypto requires zlib
AC_CHECK_LIB([crypto],[EVP_Digest],[],[AC_MSG_ERROR([Could not find crypto library])])
LIBS="-lcrypto $LIBS"

#PTHREAD: AC_CHECK_LIB([pthread],[pthread_mutexattr_settype],[],[AC_MSG_ERROR([Could not find pthread library])])
#LIBS="-lssl -lcrypto -lpthread $LIBS"
#LIBS="-lcrypto -lpthread $LIBS"

################################################################
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

# zzz NO regex or tre
#AC_CHECK_HEADERS([regex.h tre/tre.h])
#AC_CHECK_LIB([regex],[regcomp])
#AC_CHECK_LIB([tre],[tre_regcomp])
#AC_CHECK_FUNCS([regcomp tre_regcomp tre_version])

################################################################
## support required by hashdb_environment.hpp
#AC_CHECK_HEADERS([sys/cdefs.h sys/resource.h pwd.h sys/utsname.h])
AC_CHECK_HEADERS([pwd.h])
AC_CHECK_FUNCS([getuid getpwuid])

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
#AC_TRY_COMPILE([#pragma GCC diagnostic ignored "-Wredundant-decls"],[int a=3;],
#  [AC_DEFINE(HAVE_DIAGNOSTIC_REDUNDANT_DECLS,1,[define 1 if GCC supports -Wredundant-decls])]
#)

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

#################################################################
# Get the GIT commit into the GIT_COMMIT variable
AC_CHECK_PROG([git],[git],[yes],[no])
AM_CONDITIONAL([FOUND_GIT],[test "x$git" = xyes])
AM_COND_IF([FOUND_GIT],
        [GIT_COMMIT=`git describe --dirty --always`
         AC_MSG_NOTICE([git commit $GIT_COMMIT])],
        [AC_MSG_WARN([git not found])])

#################################################################
##
## Do we have the CPUID instruction?
#AC_TRY_COMPILE([#define cpuid(id) __asm__( "cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(id), "b"(0), "c"(0), "d"(0))],
#			[unsigned long eax, ebx, ecx, edx;cpuid(0);],
#			have_cpuid=yes,
#			have_cpuid=no)
#if test "$have_cpuid" = yes; then
# AC_DEFINE(HAVE_ASM_CPUID, 1, [define to 1 if __asm__ CPUID is available])
#fi

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
AC_MSG_NOTICE([  HASHDB_CFLAGS:    $HASHDB_CFLAGS])
AC_MSG_NOTICE([  HASHDB_CXXFLAGS:  $HASHDB_CXXFLAGS])

AC_MSG_NOTICE([  PYTHON_VERSION:   $PYTHON_VERSION])
#AC_MSG_NOTICE([  PYTHON_CPPFLAGS:  $PYTHON_CPPFLAGS])
#AC_MSG_NOTICE([  PYTHON_LIBS:      $PYTHON_LIBS])
#AC_MSG_NOTICE([  PYTHON_EXTRA_LIBS:    $PYTHON_EXTRA_LIBS])
#AC_MSG_NOTICE([  PYTHON_EXTRA_LDFLAGS: $PYTHON_LDFLAGS])

AC_OUTPUT

## for hashdb git version
if test x"$GIT_COMMIT" != "x" ; then
  echo "#define GIT_COMMIT  \"$GIT_COMMIT\""  >> config.h
fi

