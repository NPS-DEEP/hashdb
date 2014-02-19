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

# This file assists in building test programs.

bin_PROGRAMS = \
	source_lookup_encoding_test \
	map_and_multimap_test \
	btree_bug_test \
	map_manager_test \
	multimap_manager_test \
	map_multimap_iterator_test \
	bloom_filter_manager_test \
	settings_manager_test \
	logger_test \
	hashdb_element_test \
	map_multimap_manager_test \
	source_lookup_index_manager_test \
	hashdb_iterator_test \
	hashdb_manager_test \
	dfxml_hashdigest_rw_test

# to validate backward compatibility to c++98, add option -std=c++98
#zzzzzzzzzzzzzzzzzzzzzzzzzzzzz change -isystem to -I
AM_CPPFLAGS = -isystem${top_srcdir}/src/misc \
	-isystem${top_srcdir}/src/btree/include \
	-isystem${top_srcdir}/src/endian/include \
	-I${top_srcdir}/src \
	$(XML_CFLAGS)

AM_LDFLAGS = -static $(XML_LIBS)

run_tests:
	for test in $(bin_PROGRAMS); do \
	  echo $$test ... ; \
	  ./$$test; \
	done

# ############################################################
# potentially useful include files
# ############################################################
HASHDB_INCS = \
	../bi_data_types.hpp \
	../bi_store.hpp \
	../bloom_filter.hpp \
	../command_line.cpp \
	../command_line.hpp \
	../hashdb.cpp \
	../hashdb.hpp \
	../hashdb_filenames.cpp \
	../hashdb_filenames.hpp \
	../hashdb_types.h \
	../hashdb_db_manager.hpp \
	../settings.hpp \
	../hashdb_settings_reader.hpp \
	../hash_duplicates_store.cpp \
	../hash_duplicates_store.hpp \
	../hash_store.cpp \
	../hash_store.hpp \
	../manager_modified.h \
	../manager_modified_includes.h \
	../manager_modified_multimap.h \
	../query_by_path.hpp \
	../query_by_socket.hpp \
	../source_lookup_manager.hpp \
	../source_lookup_encoding.hpp

DFXML_INCS = \
	../dfxml/src/hash_t.h \
	../dfxml/src/dfxml_writer.cpp \
	../dfxml/src/dfxml_writer.h \
	../dfxml/src/cppmutex.h

BLOOM_INCS = \
	../misc/bloom.c \
	../misc/bloom.h \
	../misc/file_mapper.cpp \
	../misc/file_mapper.hpp

BTREE_INCS = \
	../btree/include/boost/btree/index_helpers.hpp \
	../btree/include/boost/btree/support/string_view.hpp \
	../btree/include/boost/btree/support/random_string.hpp \
	../btree/include/boost/btree/support/size_t_codec.hpp \
	../btree/include/boost/btree/support/string_holder.hpp \
	../btree/include/boost/btree/btree_index_set.hpp \
	../btree/include/boost/btree/mmff.hpp \
	../btree/include/boost/btree/btree_set.hpp \
	../btree/include/boost/btree/btree_index_set.hpp \
	../btree/include/boost/btree/helpers.hpp \
	../btree/include/boost/btree/detail/binary_file.hpp \
	../btree/include/boost/btree/detail/test_traits.hpp \
	../btree/include/boost/btree/detail/index_bases.hpp \
	../btree/include/boost/btree/detail/config.hpp \
	../btree/include/boost/btree/detail/btree_bases.hpp \
	../btree/include/boost/btree/detail/buffer_manager.hpp \
	../btree/include/boost/btree/header.hpp \
	../btree/include/boost/btree/bulk_load.hpp \
	../btree/include/boost/btree/btree_map.hpp \
	../btree/src/detail/buffer_manager.cpp \
	../btree/src/detail/binary_file.cpp

ENDIAN_INCS = \
	../endian/include/boost/endian/std_pair.hpp \
	../endian/include/boost/endian/types.hpp \
	../endian/include/boost/endian/detail/cover_operators.hpp \
	../endian/include/boost/endian/detail/disable_warnings.hpp \
	../endian/include/boost/endian/detail/config.hpp \
	../endian/include/boost/endian/detail/disable_warnings_pop.hpp \
	../endian/include/boost/endian/detail/intrinsic.hpp \
	../endian/include/boost/endian/conversion.hpp

# ############################################################
# tests
# ############################################################
source_lookup_encoding_test_SOURCES = \
	../source_lookup_encoding.hpp \
	boost_fix.hpp \
	source_lookup_encoding_test.cpp

map_and_multimap_test_SOURCES = \
	boost_fix.hpp \
	to_key_helper.hpp \
        ../map_stats.hpp \
        ../file_modes.h \
	../dfxml/src/dfxml_writer.h \
	../map_red_black_tree.hpp \
	../map_unordered_hash.hpp \
	../map_flat_sorted_vector.hpp \
	../map_btree.hpp \
	../multimap_red_black_tree.hpp \
	../multimap_unordered_hash.hpp \
	../multimap_flat_sorted_vector.hpp \
	../multimap_btree.hpp \
	$(BTREE_INCS) $(ENDIAN_INCS) \
	map_and_multimap_test.cpp

btree_bug_test_SOURCES = \
	boost_fix.hpp \
	../map_btree.hpp \
	../multimap_btree.hpp \
	$(BTREE_INCS) $(ENDIAN_INCS) \
	btree_bug_test.cpp

map_manager_test_SOURCES = \
	boost_fix.hpp \
        ../file_modes.h \
        ../map_types.h \
	../map_red_black_tree.hpp \
	../map_unordered_hash.hpp \
	../map_flat_sorted_vector.hpp \
	../map_btree.hpp \
	../map_manager.hpp \
	$(BTREE_INCS) $(ENDIAN_INCS) \
	map_manager_test.cpp

multimap_manager_test_SOURCES = \
	boost_fix.hpp \
	../file_modes.h \
	../multimap_types.h \
	../multimap_red_black_tree.hpp \
	../multimap_unordered_hash.hpp \
	../multimap_flat_sorted_vector.hpp \
	../multimap_btree.hpp \
	../multimap_manager.hpp \
	$(BTREE_INCS) $(ENDIAN_INCS) \
	multimap_manager_test.cpp

map_multimap_iterator_test_SOURCES = \
	boost_fix.hpp \
        ../file_modes.h \
        ../map_types.h \
	../map_manager.hpp \
        ../multimap_types.h \
	../multimap_manager.hpp \
	../hashdb_iterator.hpp \
	$(BTREE_INCS) $(ENDIAN_INCS) \
	map_multimap_iterator_test.cpp

bloom_filter_manager_test_SOURCES = \
	boost_fix.hpp \
	../file_modes.h \
	../bloom_filter_manager.hpp \
	$(BLOOM_INCS) \
	bloom_filter_manager_test.cpp

settings_manager_test_SOURCES = \
	boost_fix.hpp \
	../settings.hpp \
	../settings_manager.hpp \
	../command_line.cpp \
	../command_line.hpp \
	$(DFXML_INCS) \
	settings_manager_test.cpp

logger_test_SOURCES = \
	boost_fix.hpp \
	../logger.hpp \
	../hashdb_changes.hpp \
	../settings.hpp \
	../command_line.cpp \
	../command_line.hpp \
	$(DFXML_INCS) \
	logger_test.cpp

hashdb_element_test_SOURCES = \
	boost_fix.hpp \
	../hashdb_element.hpp \
	$(DFXML_INCS) \
	hashdb_element_test.cpp

map_multimap_manager_test_SOURCES = \
	boost_fix.hpp \
        ../file_modes.h \
        ../map_types.h \
        ../multimap_types.h \
	../map_multimap_manager.hpp \
	../map_multimap_iterator.hpp \
	$(BTREE_INCS) $(ENDIAN_INCS) \
	../command_line.cpp \
	../command_line.hpp \
	$(BLOOM_INCS) \
	$(DFXML_INCS) \
	map_multimap_manager_test.cpp

source_lookup_index_manager_test_SOURCES = \
	boost_fix.hpp \
        ../file_modes.h \
        ../source_lookup_index_manager.hpp \
	$(BTREE_INCS) $(ENDIAN_INCS) \
	source_lookup_index_manager_test.cpp

hashdb_iterator_test_SOURCES = \
	boost_fix.hpp \
        ../file_modes.h \
        ../map_types.h \
        ../multimap_types.h \
	../map_multimap_manager.hpp \
	../map_multimap_iterator.hpp \
	../hashdb_iterator.hpp \
	$(BTREE_INCS) $(ENDIAN_INCS) \
	../command_line.cpp \
	../command_line.hpp \
	$(BLOOM_INCS) \
	$(DFXML_INCS) \
	hashdb_iterator_test.cpp

hashdb_manager_test_SOURCES = \
	boost_fix.hpp \
        ../file_modes.h \
        ../map_types.h \
        ../multimap_types.h \
	../map_multimap_manager.hpp \
	../map_multimap_iterator.hpp \
	../hashdb_iterator.hpp \
	../hashdb_manager.hpp \
	$(BTREE_INCS) $(ENDIAN_INCS) \
	../command_line.cpp \
	../command_line.hpp \
	$(BLOOM_INCS) \
	$(DFXML_INCS) \
	hashdb_manager_test.cpp

dfxml_hashdigest_rw_test_SOURCES = \
	boost_fix.hpp \
	../dfxml_hashdigest_reader.hpp \
	../dfxml_hashdigest_reader_manager.hpp \
	../dfxml_hashdigest_writer.hpp \
	../command_line.cpp \
	../command_line.hpp \
	$(DFXML_INCS) \
	dfxml_hashdigest_rw_test.cpp

.PHONY: run_tests

