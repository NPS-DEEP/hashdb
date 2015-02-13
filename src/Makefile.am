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

bin_PROGRAMS = hashdb
lib_LTLIBRARIES = libhashdb.la
libhashdb_la_LIBADD = liblmdb.la
noinst_LTLIBRARIES = liblmdb.la
liblmdb_la_SOURCES = \
	lmdb/libraries/liblmdb/lmdb.h \
	lmdb/libraries/liblmdb/mdb.c \
	lmdb/libraries/liblmdb/midl.h \
	lmdb/libraries/liblmdb/midl.c

EXTRA_DIST = \
	lmdb/libraries/liblmdb/COPYRIGHT
	lmdb/libraries/liblmdb/LICENSE

# these CFLAGS are adapted from liblmdb/Makefile
liblmdb_la_CFLAGS = \
	-pthread -O3 -g \
	-W -Wall -Wno-unused-parameter -Wbad-function-cast -Wuninitialized \
	-Wno-format-extra-args
liblmdb_la_LDFLAGS = -static -PIC

hashdb_LDFLAGS = -static
libhashdb_la_LDFLAGS = -static -PIC -avoid-version

# to validate backward compatibility to c++98, add option -std=c++98
AM_CPPFLAGS = -isystem${top_srcdir}/src/misc \
	-I${top_srcdir}/src/lmdb/libraries/liblmdb \
	$(XML_CFLAGS)
AM_CFLAGS = $(HASHDB_CFLAGS)
AM_CXXFLAGS = $(HASHDB_CXXFLAGS)

hashdb_LDADD = libhashdb.la

HASHDB_INCS = \
	commands.hpp \
	dfxml_hashdigest_reader.hpp \
	dfxml_import_consumer.hpp \
	dfxml_scan_consumer.hpp \
	dfxml_scan_expanded_consumer.hpp \
	dfxml_hashdigest_writer.hpp \
	feature_file_reader.hpp \
	feature_line.hpp \
	json_formatter.hpp \
	main.cpp \
	progress_tracker.hpp \
	random_binary_hash.hpp \
	tab_hashdigest_reader.hpp \
	usage.hpp

LIBHASHDB_INCS = \
	bloom_filter_manager.hpp \
	crc32.h \
	file_modes.h \
	globals.cpp \
	globals.hpp \
	hashdb_changes.hpp \
	hashdb.cpp \
	hashdb_directory_manager.hpp \
	hashdb.hpp \
	hashdb_settings.hpp \
	hashdb_settings_reader.hpp \
	hashdb_settings_store.hpp \
	history_manager.hpp \
	lmdb_rw_manager.hpp \
	lmdb_rw_new.hpp \
	lmdb_context.hpp \
	lmdb_hash_it_data.hpp \
	lmdb_hash_store.hpp \
	lmdb_helper.h \
	lmdb_name_store.hpp \
	lmdb_ro_manager.hpp \
	lmdb_source_data.hpp \
	lmdb_source_it_data.hpp \
	lmdb_source_store.hpp \
	logger.hpp \
	mutex_lock.hpp

BLOOM_INCS = \
	misc/bloom.c \
	misc/bloom.h \
	misc/file_mapper.cpp \
	misc/file_mapper.hpp

DFXML_INCS = \
	dfxml/src/hash_t.h \
	dfxml/src/dfxml_writer.cpp \
	dfxml/src/dfxml_writer.h \
	dfxml/src/cppmutex.h

hashdb_SOURCES = \
	$(HASHDB_INCS)

libhashdb_la_SOURCES = \
	$(LIBHASHDB_INCS) \
	$(LMDB_INCS) \
	$(BLOOM_INCS) $(DFXML_INCS)

include_HEADERS = hashdb.hpp

