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

EXTRA_DIST = \
	identified_blocks.txt \
	sample_dfxml

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
	dfxml_hashdigest_rw_test \
	identified_blocks_reader_test \
	libhashdb_test \
	commands_test

# to validate backward compatibility to c++98, add option -std=c++98
#zzzzzzzzzzzzzzzzzzzzzzzzzzzzz change -isystem to -I
AM_CPPFLAGS = -isystem${top_srcdir}/src/misc \
	-isystem${top_srcdir}/src/btree/include \
	-isystem${top_srcdir}/src/endian/include \
	-I${top_srcdir}/src \
	$(XML_CFLAGS)

libhashdb_test_LDADD = ../libhashdb/libhashdb.la
commands_test_LDADD = ../libhashdb/libhashdb.la

AM_LDFLAGS = -static

run_tests:
	for test in $(bin_PROGRAMS); do \
	  echo Begin $$test ... ; \
	  ./$$test; \
	done

# test with valgrind memory analysis
run_tests_valgrind:
	for test in $(bin_PROGRAMS); do \
	  echo Begin $$test ... ; \
	  valgrind --tool=memcheck --leak-check=full --show-reachable=yes --suppressions=vg.supp ./$$test; \
	done

# ############################################################
# potentially useful include files
# ############################################################
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

COMMAND_LINE_INCS = \
	../command_line.cpp \
	../command_line.hpp

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
	$(BTREE_INCS) $(ENDIAN_INCS) \
	map_and_multimap_test.cpp

btree_bug_test_SOURCES = \
	boost_fix.hpp \
	$(BTREE_INCS) $(ENDIAN_INCS) \
	btree_bug_test.cpp

map_manager_test_SOURCES = \
	boost_fix.hpp \
	directory_helper.hpp \
	$(BTREE_INCS) $(ENDIAN_INCS) \
	map_manager_test.cpp

multimap_manager_test_SOURCES = \
	boost_fix.hpp \
	directory_helper.hpp \
	$(BTREE_INCS) $(ENDIAN_INCS) \
	multimap_manager_test.cpp

map_multimap_iterator_test_SOURCES = \
	boost_fix.hpp \
	directory_helper.hpp \
	$(BTREE_INCS) $(ENDIAN_INCS) \
	map_multimap_iterator_test.cpp

bloom_filter_manager_test_SOURCES = \
	boost_fix.hpp \
	directory_helper.hpp \
	$(BLOOM_INCS) \
	bloom_filter_manager_test.cpp

settings_manager_test_SOURCES = \
	boost_fix.hpp \
	directory_helper.hpp \
	$(COMMAND_LINE_INCS) \
	$(DFXML_INCS) \
	settings_manager_test.cpp

logger_test_SOURCES = \
	boost_fix.hpp \
	directory_helper.hpp \
	$(COMMAND_LINE_INCS) \
	$(DFXML_INCS) \
	logger_test.cpp

hashdb_element_test_SOURCES = \
	boost_fix.hpp \
	$(DFXML_INCS) \
	hashdb_element_test.cpp

map_multimap_manager_test_SOURCES = \
	boost_fix.hpp \
	directory_helper.hpp \
	$(BTREE_INCS) $(ENDIAN_INCS) \
	$(COMMAND_LINE_INCS) \
	$(BLOOM_INCS) \
	$(DFXML_INCS) \
	map_multimap_manager_test.cpp

source_lookup_index_manager_test_SOURCES = \
	boost_fix.hpp \
	directory_helper.hpp \
	$(BTREE_INCS) $(ENDIAN_INCS) \
	source_lookup_index_manager_test.cpp

hashdb_iterator_test_SOURCES = \
	boost_fix.hpp \
	directory_helper.hpp \
	$(BTREE_INCS) $(ENDIAN_INCS) \
	$(COMMAND_LINE_INCS) \
	$(BLOOM_INCS) \
	$(DFXML_INCS) \
	hashdb_iterator_test.cpp

hashdb_manager_test_SOURCES = \
	boost_fix.hpp \
	directory_helper.hpp \
	$(BTREE_INCS) $(ENDIAN_INCS) \
	$(COMMAND_LINE_INCS) \
	$(BLOOM_INCS) \
	$(DFXML_INCS) \
	hashdb_manager_test.cpp

dfxml_hashdigest_rw_test_SOURCES = \
	boost_fix.hpp \
	directory_helper.hpp \
	$(COMMAND_LINE_INCS) \
	$(DFXML_INCS) \
	dfxml_hashdigest_rw_test.cpp

identified_blocks_reader_test_SOURCES = \
	boost_fix.hpp \
	identified_blocks_reader_test.cpp

libhashdb_test_SOURCES = \
	boost_fix.hpp \
	directory_helper.hpp \
	$(DFXML_INCS) \
	libhashdb_test.cpp
	
commands_test_SOURCES = \
	boost_fix.hpp \
	directory_helper.hpp \
	$(BTREE_INCS) $(ENDIAN_INCS) \
	$(COMMAND_LINE_INCS) \
	$(BLOOM_INCS) \
	$(DFXML_INCS) \
	commands_test.cpp

.PHONY: run_tests run_tests_valgrind

