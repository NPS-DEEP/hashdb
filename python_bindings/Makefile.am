# Author:  Bruce Allen bdallen nps edu
# Created: 2/25/2013
#
# The software provided here is released by the Naval Postgraduate
# School, an agency of the U.S. Department of Navy.  The software
# bears no warranty, either expressed or implied. NPS does not assume
# legal liability nor responsibility for a User's use of the software
# or the results of such use.
#
# Please note that within the United States, copyright protection,
# under Section 105 of the United States Code, Title 17, is not
# available for any work of the United States Government and/or for
# any works created by United States Government employees. User
# acknowledges that this software contains work which was created by
# NPS government employees and is therefore in the public domain and
# not subject to copyright.
#
# Released into the public domain on February 25, 2013 by Bruce Allen.

# This file assists in building the hashdb Python bindings

# http://realmike.org/blog/2010/07/18/building-swig-python-extensions-on-gnulinux/
# https://github.com/roryyorke/swig-python-autotools-example/blob/master/Makefile.am

## the hashdb_wrap.cpp source and also hashdb.py that will be generated
BUILT_SOURCES = hashdb_wrap.cpp

# recipe for hashdb_wrap.c
# note that SWIG creates hashdb_wrap.c and hashdb.py
# SWIG needs to find hashdb.hpp to prepare Python interfaces
hashdb_wrap.cpp: hashdb.i \
               ${top_srcdir}/src_libhashdb/hashdb.hpp \
               ../src_libhashdb/libhashdb.la
	rm -f hashdb.hpp
	ln -s ${top_srcdir}/src_libhashdb/hashdb.hpp .
	$(SWIG) $(AX_SWIG_PYTHON_OPT) -threads -o $@ $<
	rm hashdb.hpp

# install hashdb.py
python_PYTHON = $(builddir)/hashdb.py

# module, so no lib prefix
_hashdb_la_LDFLAGS = -module ${PYTHON_LDFLAGS}

# install _hashdb.la
pyexec_LTLIBRARIES = _hashdb.la

# the files that _hashdb.la depends on
_hashdb_la_SOURCES = hashdb_wrap.cpp hashdb.i

# _hashdb.la requires relocatable hashdb library libhashdb.so
_hashdb_la_LIBADD = ../src_libhashdb/libhashdb.la

# c++ flags for compiling _hashdb.la
_hashdb_la_CPPFLAGS = ${PYTHON_CPPFLAGS} -Wno-deprecated-register -I${top_srcdir}/src_libhashdb


# for testing
AM_TESTS_ENVIRONMENT = export PYTHONPATH='.:.libs';
testfile = test_hashdb.py
EXTRA_DIST = $(testfile)
TESTS = $(testfile)
clean-local:
	rm -f hashdb.py hashdb_wrap.cpp hashdb.hpp
	rm -rf temp_*

