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

bin_PROGRAMS = hashdb_manager

# to validate backward compatibility to c++98, add option -std=c++98
#zzzzzzzzzzzzzzzzzzzzzzzzzzzzz change -isystem to -I
AM_CPPFLAGS = -isystem${top_srcdir}/src/misc \
	-isystem${top_srcdir}/src/btree/include \
	-isystem${top_srcdir}/src/endian/include \
	$(XML_CFLAGS)

AM_LDFLAGS = -static $(XML_LIBS)

HASHDB_INCS = \
	bi_data_types.hpp \
	bi_store.hpp \
	bloom_filter.hpp \
	commands.hpp \
	dfxml_hashdigest_reader.hpp \
	hashdb.cpp \
	hashdb.hpp \
	hashdb_db_info_provider.hpp \
	hashdb_db_manager.hpp \
	hashdb_exporter.hpp \
	hashdb_filenames.cpp \
	hashdb_filenames.hpp \
	hashdb_manager.cpp \
	hashdb_types.h \
	hashdb_settings.h \
	hash_duplicates_store.hpp \
	hash_duplicates_store.cpp \
	hash_store.hpp \
	hash_store.cpp \
	history_manager.hpp \
	manager_modified.h \
	manager_modified_includes.h \
	manager_modified_multimap.h \
	query_by_path.hpp \
	query_by_socket.hpp \
	query_by_socket_server.hpp \
	settings_reader.hpp \
	settings_writer.hpp \
	source_lookup_manager.hpp \
	source_lookup_record.h

DFXML_INCS = \
	dfxml/src/hash_t.h \
	dfxml/src/dfxml_writer.cpp \
	dfxml/src/dfxml_writer.h \
	dfxml/src/cppmutex.h

MISC_INCS = \
	misc/bloom.c \
	misc/bloom.h \
	misc/file_mapper.cpp \
	misc/file_mapper.hpp

# note: btree/tools/*.cpp files exist but are expected to be unused
BTREE_INCS = \
	btree/include/boost/btree/index_helpers.hpp \
	btree/include/boost/btree/support/string_view.hpp \
	btree/include/boost/btree/support/random_string.hpp \
	btree/include/boost/btree/support/size_t_codec.hpp \
	btree/include/boost/btree/support/string_holder.hpp \
	btree/include/boost/btree/btree_index_set.hpp \
	btree/include/boost/btree/mmff.hpp \
	btree/include/boost/btree/btree_set.hpp \
	btree/include/boost/btree/btree_index_map.hpp \
	btree/include/boost/btree/helpers.hpp \
	btree/include/boost/btree/detail/binary_file.hpp \
	btree/include/boost/btree/detail/test_traits.hpp \
	btree/include/boost/btree/detail/index_bases.hpp \
	btree/include/boost/btree/detail/config.hpp \
	btree/include/boost/btree/detail/btree_bases.hpp \
	btree/include/boost/btree/detail/buffer_manager.hpp \
	btree/include/boost/btree/header.hpp \
	btree/include/boost/btree/bulk_load.hpp \
	btree/include/boost/btree/btree_map.hpp \
	btree/src/detail/buffer_manager.cpp \
	btree/src/detail/binary_file.cpp

ENDIAN_INCS = \
	endian/include/boost/endian/std_pair.hpp \
	endian/include/boost/endian/types.hpp \
	endian/include/boost/endian/detail/cover_operators.hpp \
	endian/include/boost/endian/detail/disable_warnings.hpp \
	endian/include/boost/endian/detail/config.hpp \
	endian/include/boost/endian/detail/disable_warnings_pop.hpp \
	endian/include/boost/endian/detail/intrinsic.hpp \
	endian/include/boost/endian/conversion.hpp

hashdb_manager_SOURCES = \
	$(HASHDB_INCS) $(DFXML_INCS) $(MISC_INCS) $(BTREE_INCS) $(ENDIAN_INCS)

