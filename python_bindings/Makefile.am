# Author:  Bruce Allen <bdallen@nps.edu>
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

# put built sources in the build tree
BUILT_SOURCES = $(builddir)/hashdb.py $(builddir)/hashdb_wrap.c

# recipe for example.py and example_wrap.c
$(builddir)/example.py $(builddir)/example_wrap.c: hashdb.i
	$(SWIG) $(AX_SWIG_PYTHON_OPT) -o $@ $<

# install
python_PYTHON = $(builddir)/hashdb.py

# The Python module
pyexec_LTLIBRARIES = _hashdb.la

# The Python module is composed of generated files
_hashdb_la_SOURCES = $(builddir)/hashdb_wrap.c ../src_libhashdb/hashdb.hpp \
                     hashdb.i

# module, so no lib prefix
_hashdb_la_LDFLAGS = -module ${PYTHON_LDFLAGS}

# link libhashdb
_hashdb_la_LIBADD = ../src_libhashdb/libhashdb.la

_hashdb_la_CPPFLAGS = ${PYTHON_CPPFLAGS}
