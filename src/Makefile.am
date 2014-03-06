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
#lib_LTLIBRARIES = libhashdb.la

#libhashdb_la_LDFLAGS = -static -avoid-version

# to validate backward compatibility to c++98, add option -std=c++98
#zzzzzzzzzzzzzzzzzzzzzzzzzzzzz change -isystem to -I
AM_CPPFLAGS = -isystem${top_srcdir}/src/misc \
	-isystem${top_srcdir}/src/btree/include \
	-isystem${top_srcdir}/src/endian/include \
	$(XML_CFLAGS)

#zzzzzzzzzAM_LDFLAGS = -static $(XML_LIBS)
AM_LDFLAGS = -static

HAHSDB_INCS = \
	map_multimap_iterator.hpp \
	commands.hpp \
	usage.hpp \
	hashdb.hpp \
	map_multimap_manager.hpp \
	hashdb_manager.hpp \
	source_lookup_index_manager.hpp \
	hashdigest_manager.hpp \
	hashdigest_iterator.hpp \
	source_lookup_index_iterator.hpp \
	bi_store.hpp \
	identified_blocks_reader_iterator.hpp \
	hashdb_iterator.hpp \
	hashdb_settings_reader.hpp \
	hashdb_settings.hpp \
	identified_blocks_reader.hpp \
	bloom_rebuild_manager.hpp \
	hashdb_element_lookup.hpp \
	map_manager.hpp \
	map_unordered_hash.hpp \
	map_flat_sorted_vector.hpp \
	map_btree.hpp \
	map_red_black_tree.hpp \
	hashdb_changes.hpp \
	logger.hpp \
	dfxml_hashdigest_reader_manager.hpp \
	bloom_filter_manager.hpp \
	hashdb_settings_manager.hpp \
	dfxml_hashdigest_writer.hpp \
	dfxml_hashdigest_reader.hpp \
	history_manager.hpp \
	bloom_filter_calculator.hpp \
	bloom_filter.hpp \
	hashdb_element.hpp \
	multimap_manager.hpp \
	hashdigest.hpp \
	multimap_flat_sorted_vector.hpp \
	multimap_red_black_tree.hpp \
	multimap_unordered_hash.hpp \
	multimap_btree.hpp \
	query_by_socket.hpp \
	query_by_socket_server.hpp \
	query_by_path.hpp \
	hashdb_settings_writer.hpp \
	hashdb_db_info_provider.hpp \
	hash_values.hpp \
	multimap_iterator.hpp \
	map_iterator.hpp \
	source_lookup_encoding.hpp \
	command_line.hpp \
	bi_data_types.hpp \
	bi_64_pair_data.hpp \
	bi_64_sv_data.hpp \
	hashdb_filenames.cpp \
	hashdb_filenames.hpp \
	file_modes.h \
	hash_algorithm_types.h \
	hashdigest_types.h \
	map_types.h \
	multimap_types.h

OLD_HASHDB_INCS = \
	bi_64_pair_data.hpp \
	bi_64_sv_data.hpp \
	bi_data_types.hpp \
	bi_store.hpp \
	bloom_filter_calculator.hpp \
	bloom_filter.hpp \
	bloom_filter_manager.hpp \
	commands.hpp \
	dfxml_hashdigest_reader.hpp \
	dfxml_hashdigest_reader_manager.hpp \
	dfxml_hashdigest_writer.hpp \
	file_modes.h \
	hash_algorithm_types.h \
	hashdb_changes.hpp \
	hashdb.cpp \
	hashdb_db_info_provider.hpp \
	hashdb_element.hpp \
	hashdb_element_lookup.hpp \
	hashdb_element_lookup.hpp \
	\
	hashdb_manager.cpp \
	hashdb_types.h \
	hash_duplicates_store.hpp \
	hash_duplicates_store.cpp \
	hash_store.hpp \
	hash_store.cpp \
	hash_values.hpp \
	history_manager.hpp \
	manager_modified.h \
	manager_modified_includes.h \
	manager_modified_multimap.h \
	query_by_path.hpp \
	query_by_socket.hpp \
	query_by_socket_server.hpp \
	settings.hpp \
	hashdb_settings_reader.hpp \
	hashdb_settings_writer.hpp \
	source_lookup_manager.hpp \
	source_lookup_encoding.hpp

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

COMMAND_LINE_INCS = \
	command_line.cpp \
	command_line.hpp

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

#hashdb_manager_SOURCES = \
#	$(HASHDB_INCS) $(DFXML_INCS) $(MISC_INCS) \
#	$(BTREE_INCS) $(ENDIAN_INCS)

hashdb_SOURCES = \
	main.cpp \
	usage.hpp \
	$(COMMAND_LINE_INCS) \
	$(BTREE_INCS) $(ENDIAN_INCS) \
	$(BLOOM_INCS) $(DFXML_INCS) \
	$(HAHSDB_INCS)

# use separate makefile because autotools can't create these
# with and without libtool
#libhashdb_la_SOURCES = \
#	$(COMMAND_LINE_INCS) \
#	$(BTREE_INCS) $(ENDIAN_INCS) \
#	$(BLOOM_INCS) $(DFXML_INCS) \
#	hashdb.cpp \
#	hashdb.hpp
#
#include_HEADERS = hashdb.hpp

