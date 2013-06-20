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

# This file prepares the query client plugin and header distribution.

lib_LTLIBRARIES = libscan_sectorid.la
libscan_sectorid_la_LDFLAGS = -static -avoid-version
include_HEADERS = ../../query.h ../../query_client.hpp

# http://www.flameeyes.eu/autotools-mythbuster/libtool/plugins.html
# section 2.2: Building plug-ins for dlopen()
# zzz may also need -export-dynamic for windows

# to validate backward compatibility to c++98, add option -std=c++98
AM_CPPFLAGS = -I${top_srcdir}/src \
	-isystem${top_srcdir}/src/misc \
	-isystem${top_srcdir}/src/xbtree \
	-I${top_srcdir}/src/dfxml2/src

CLIENT_INCS = \
	../../query.h \
	../../query_client.hpp

HASHDB_INCS = \
	../../bloom_filter.hpp \
	../../hashdb_filenames.hpp \
	../../hashdb.h \
	../../hashdb_manager.hpp \
	../../hashdb_settings.h \
	../../hash_duplicates_store.hpp \
	../../hash_duplicates_store.cpp \
	../../hash_store.hpp \
	../../hash_store.cpp \
	../../manager_modified.h \
	../../manager_modified_includes.h \
	../../manager_modified_multimap.h \
	../../manager_multi_index_container.h \
	../../settings_reader.hpp \
	../../source_location_record.hpp \
	../../source_lookup_record.hpp \
	../../source_lookup_store.hpp \
	../../source_lookup_store.cpp

DFXML_INCL = \
	../../dfxml/src/md5.h

#BE13_API_INCS = \
#	../../be13_api/beregex.cpp \
#	../../be13_api/beregex.h \
#	../../be13_api/bulk_extractor_i.h \
#	../../be13_api/CODING_STANDARDS.txt \
#	../../be13_api/cppmutex.h \
#	../../be13_api/feature_recorder.cpp \
#	../../be13_api/feature_recorder.h \
#	../../be13_api/feature_recorder_set.cpp \
#	../../be13_api/feature_recorder_set.h \
#	../../be13_api/md5.c \
#	../../be13_api/md5.h \
#	../../be13_api/net_ethernet.h \
#	../../be13_api/pcap_fake.cpp \
#	../../be13_api/pcap_fake.h \
#	../../be13_api/plugin.cpp \
#	../../be13_api/README.md \
#	../../be13_api/sbuf.cpp \
#	../../be13_api/sbuf_flex_scanner.h \
#	../../be13_api/sbuf.h \
#	../../be13_api/sbuf_private.h \
#	../../be13_api/sbuf_stream.cpp \
#	../../be13_api/sbuf_stream.h \
#	../../be13_api/unicode_escape.cpp \
#	../../be13_api/unicode_escape.h \
#	../../be13_api/utf8 \
#	../../be13_api/utf8/unchecked.h \
#	../../be13_api/utf8/checked.h \
#	../../be13_api/utf8/core.h \
#	../../be13_api/utf8.h \
#	../../be13_api/utils.cpp \
#	../../be13_api/utils.h \
#	../../be13_api/xml.cpp \
#	../../be13_api/xml.h

MISC_INCS = \
	../../misc/aftimer.h \
	../../misc/bloom.c \
	../../misc/bloom.h \
	../../misc/util.h

BOOST_INCS = \
	../../xbtree/boost/btree/dynamic_size.hpp \
	../../xbtree/boost/btree/header.hpp \
	../../xbtree/boost/btree/map.hpp \
	../../xbtree/boost/btree/set.hpp \
	../../xbtree/boost/btree/detail/binary_file.hpp \
	../../xbtree/boost/btree/detail/buffer_manager.hpp \
	../../xbtree/boost/btree/detail/common.hpp \
	../../xbtree/boost/btree/detail/config.hpp \
	../../xbtree/boost/btree/support/c_str_proxy.hpp \
	../../xbtree/boost/btree/support/fixstr.hpp \
	../../xbtree/boost/btree/support/random_string.hpp \
	../../xbtree/boost/btree/support/strbuf.hpp \
	../../xbtree/boost/btree/support/timer.hpp \
	../../xbtree/boost/integer/endian.hpp \
	../../xbtree/boost/integer/endian_flip.hpp \
	../../xbtree/boost/integer/endian_binary_stream.hpp \
	../../xbtree/boost/integer/cover_operators.hpp \
	../../xbtree/libs/btree/src/detail/run_timer.cpp \
	../../xbtree/libs/btree/src/detail/buffer_manager.cpp \
	../../xbtree/libs/btree/src/detail/binary_file.cpp \
	../../xbtree/libs/btree/src/detail/timer.cpp \
	../../xbtree/libs/btree/src/detail/run_timer_ctors.cpp

libscan_sectorid_la_SOURCES = \
	scan_sectorid.cpp \
	$(CLIENT_INCS) $(HASHDB_INCS) $(MISC_INCS) $(DFXML_INCS) $(BOOST_INCS)

