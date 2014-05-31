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

map_and_multimap_test_LDADD = ../libhashdb.la
btree_bug_test_LDADD = ../libhashdb.la
map_manager_test_LDADD = ../libhashdb.la
multimap_manager_test_LDADD = ../libhashdb.la
map_multimap_iterator_test_LDADD = ../libhashdb.la
bloom_filter_manager_test_LDADD = ../libhashdb.la
settings_manager_test_LDADD = ../libhashdb.la
logger_test_LDADD = ../libhashdb.la
hashdb_element_test_LDADD = ../libhashdb.la
map_multimap_manager_test_LDADD = ../libhashdb.la
source_lookup_index_manager_test_LDADD = ../libhashdb.la
hashdb_iterator_test_LDADD = ../libhashdb.la
hashdb_manager_test_LDADD = ../libhashdb.la
dfxml_hashdigest_rw_test_LDADD = ../libhashdb.la
identified_blocks_reader_test_LDADD = ../libhashdb.la
libhashdb_test_LDADD = ../libhashdb.la
commands_test_LDADD = ../libhashdb.la

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
# tests
# ############################################################
source_lookup_encoding_test_SOURCES = \
	../source_lookup_encoding.hpp \
	boost_fix.hpp \
	source_lookup_encoding_test.cpp

map_and_multimap_test_SOURCES = \
	boost_fix.hpp \
	to_key_helper.hpp \
	map_and_multimap_test.cpp

btree_bug_test_SOURCES = \
	boost_fix.hpp \
	btree_bug_test.cpp

map_manager_test_SOURCES = \
	boost_fix.hpp \
	directory_helper.hpp \
	map_manager_test.cpp

multimap_manager_test_SOURCES = \
	boost_fix.hpp \
	directory_helper.hpp \
	multimap_manager_test.cpp

map_multimap_iterator_test_SOURCES = \
	boost_fix.hpp \
	directory_helper.hpp \
	map_multimap_iterator_test.cpp

bloom_filter_manager_test_SOURCES = \
	boost_fix.hpp \
	directory_helper.hpp \
	bloom_filter_manager_test.cpp

settings_manager_test_SOURCES = \
	boost_fix.hpp \
	directory_helper.hpp \
	settings_manager_test.cpp

logger_test_SOURCES = \
	boost_fix.hpp \
	directory_helper.hpp \
	logger_test.cpp

hashdb_element_test_SOURCES = \
	boost_fix.hpp \
	hashdb_element_test.cpp

map_multimap_manager_test_SOURCES = \
	boost_fix.hpp \
	directory_helper.hpp \
	map_multimap_manager_test.cpp

source_lookup_index_manager_test_SOURCES = \
	boost_fix.hpp \
	directory_helper.hpp \
	source_lookup_index_manager_test.cpp

hashdb_iterator_test_SOURCES = \
	boost_fix.hpp \
	directory_helper.hpp \
	hashdb_iterator_test.cpp

hashdb_manager_test_SOURCES = \
	boost_fix.hpp \
	directory_helper.hpp \
	hashdb_manager_test.cpp

dfxml_hashdigest_rw_test_SOURCES = \
	boost_fix.hpp \
	directory_helper.hpp \
	dfxml_hashdigest_rw_test.cpp

identified_blocks_reader_test_SOURCES = \
	boost_fix.hpp \
	identified_blocks_reader_test.cpp

libhashdb_test_SOURCES = \
	boost_fix.hpp \
	directory_helper.hpp \
	libhashdb_test.cpp
	
commands_test_SOURCES = \
	boost_fix.hpp \
	directory_helper.hpp \
	commands_test.cpp

.PHONY: run_tests run_tests_valgrind

