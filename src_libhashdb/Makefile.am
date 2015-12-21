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

# This file assists in building the hashdb_manager tool.

LMDB_INCS = \
	liblmdb/lmdb.h \
	liblmdb/mdb.c \
	liblmdb/midl.h \
	liblmdb/midl.c

lib_LTLIBRARIES = libhashdb.la
libhashdb_la_LIBADD = liblmdb.la
noinst_LTLIBRARIES = liblmdb.la
liblmdb_la_SOURCES = $(LMDB_INCS)

EXTRA_DIST = \
	libdfxml/README \
	libdfxml/dfxml_configure.m4 \
	liblmdb/COPYRIGHT \
	liblmdb/LICENSE

liblmdb_la_CFLAGS = \
	-pthread -O3 -g \
	-W -Wall -Wno-unused-parameter -Wbad-function-cast -Wuninitialized \
	-Wno-format-extra-args
liblmdb_la_LDFLAGS = -static -PIC

libhashdb_la_LDFLAGS = -static -PIC -avoid-version

AM_CPPFLAGS = -isystem${top_srcdir}/src_libhashdb/libbloom \
	-I${top_srcdir}/src_libhashdb/liblmdb \
	-I${top_srcdir}/src_libhashdb/libdfxml \
	-I${top_srcdir}/rapidjson \
	$(XML_CFLAGS)
AM_CFLAGS = $(HASHDB_CFLAGS)
AM_CXXFLAGS = $(HASHDB_CXXFLAGS)

LIBHASHDB_INCS = \
	bloom_filter_manager.hpp \
	file_helper.hpp \
	file_modes.h \
	hashdb_changes.hpp \
	hashdb.cpp \
	hashdb.hpp \
	hashdb_import_manager_private.hpp \
	hashdb_scan_manager_private.hpp \
	hashdb_settings.hpp \
	hashdb_settings_store.hpp \
	hashdb_tools.hpp \
	history_manager.hpp \
	lmdb_context.hpp \
	lmdb_data_codec.hpp \
	lmdb_hash_label_manager.hpp \
	lmdb_hash_manager.hpp \
	lmdb_helper.h \
	lmdb_source_id_manager.hpp \
	lmdb_source_metadata_manager.hpp \
	lmdb_source_name_manager.hpp \
	logger.hpp \
	mutex_lock.hpp

BLOOM_INCS = \
	libbloom/bloom.c \
	libbloom/bloom.h \
	libbloom/file_mapper.cpp \
	libbloom/file_mapper.hpp

DFXML_INCS = \
	libdfxml/cppmutex.h \
	libdfxml/dfxml_writer.cpp \
	libdfxml/dfxml_writer.h

libhashdb_la_SOURCES = \
	$(LIBHASHDB_INCS) \
	$(BLOOM_INCS) $(DFXML_INCS)

include_HEADERS = hashdb.hpp

