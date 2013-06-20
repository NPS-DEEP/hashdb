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

# This file assists in building the hashdb_checker tool.

bin_PROGRAMS = hashdb_checker

# to validate backward compatibility to c++98, add option -std=c++98
AM_CPPFLAGS = -isystem${top_srcdir}/src/misc \
	-I${top_srcdir}/src \
	$(XML_CFLAGS)

LDADD = ../libhashdb/libhashdb.la

AM_LDFLAGS = -static $(XML_LIBS)

DFXML_INCS = \
	../dfxml/src/hash_t.h

hashdb_checker_SOURCES = \
	$(DFXML_INCS) \
	hashdb_checker.cpp \
	hash_lookup_consumer.hpp \
	../hashdb.hpp

