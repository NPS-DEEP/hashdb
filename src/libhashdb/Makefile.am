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

# This file assists in building the libhashdb library.

lib_LTLIBRARIES = libhashdb.la
libhashdb_la_LDFLAGS = -static -avoid-version

# to validate backward compatibility to c++98, add option -std=c++98
AM_CPPFLAGS = -isystem${top_srcdir}/src/misc \
	-isystem${top_srcdir}/src/Boost-Btree \
	-I${top_srcdir}/src \
	$(XML_CFLAGS)

AM_LDFLAGS = -static $(XML_LIBS)

# ############################################################
# for libhashdb sources
# ############################################################
QUERY_INCS = \
	../bloom_filter.hpp \
	../hashdb.cpp \
	../hashdb.hpp \
	../hashdb_filenames.cpp \
	../hashdb_filenames.hpp \
	../hashdb_types.h \
	../hashdb_db_manager.hpp \
	../hashdb_settings.h \
	../hash_duplicates_store.cpp \
	../hash_duplicates_store.hpp \
	../hash_store.cpp \
	../hash_store.hpp \
	../manager_modified.h \
	../manager_modified_includes.h \
	../manager_modified_multimap.h \
	../manager_multi_index_container.h \
	../query_by_path.hpp \
	../query_by_socket.hpp \
	../settings_reader.hpp \
	../source_location_record.hpp \
	../source_lookup_record.h \
	../source_lookup_store.cpp \
	../source_lookup_store.hpp

DFXML_INCS = \
	../dfxml/src/hash_t.h \
	../dfxml/src/dfxml_writer.cpp \
	../dfxml/src/dfxml_writer.h \
	../dfxml/src/cppmutex.h

MISC_INCS = \
	../misc/aftimer.h \
	../misc/bloom.cpp \
	../misc/bloom.hpp \
	../misc/util.h

BOOST_BTREE_INCS = \
	../Boost-Btree/boost/btree/dynamic_size.hpp \
	../Boost-Btree/boost/btree/header.hpp \
	../Boost-Btree/boost/btree/map.hpp \
	../Boost-Btree/boost/btree/set.hpp \
	../Boost-Btree/boost/btree/detail/binary_file.hpp \
	../Boost-Btree/boost/btree/detail/buffer_manager.hpp \
	../Boost-Btree/boost/btree/detail/common.hpp \
	../Boost-Btree/boost/btree/detail/config.hpp \
	../Boost-Btree/boost/btree/support/c_str_proxy.hpp \
	../Boost-Btree/boost/btree/support/fixstr.hpp \
	../Boost-Btree/boost/btree/support/random_string.hpp \
	../Boost-Btree/boost/btree/support/strbuf.hpp \
	../Boost-Btree/boost/btree/support/timer.hpp \
	../Boost-Btree/boost/integer/endian.hpp \
	../Boost-Btree/boost/integer/endian_flip.hpp \
	../Boost-Btree/boost/integer/endian_binary_stream.hpp \
	../Boost-Btree/boost/integer/cover_operators.hpp \
	../Boost-Btree/libs/btree/src/detail/run_timer.cpp \
	../Boost-Btree/libs/btree/src/detail/buffer_manager.cpp \
	../Boost-Btree/libs/btree/src/detail/binary_file.cpp \
	../Boost-Btree/libs/btree/src/detail/timer.cpp \
	../Boost-Btree/libs/btree/src/detail/run_timer_ctors.cpp

libhashdb_la_SOURCES = \
	$(QUERY_INCS) $(DFXML_INCS) $(MISC_INCS) $(BOOST_BTREE_INCS)

include_HEADERS = ../hashdb.hpp

