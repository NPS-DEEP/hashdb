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
	vg.supp

check_PROGRAMS = \
	bloom_filter_manager_test \
	db_managers_test

TESTS = $(check_PROGRAMS)

# to validate backward compatibility to c++98, add option -std=c++98
AM_CPPFLAGS = -isystem${top_srcdir}/src/libbloom \
	-isystem${top_srcdir}/src/liblmdb \
	-isystem${top_srcdir}/src/libdfxml \
	-I${top_srcdir}/src \
	$(XML_CFLAGS)

AM_CFLAGS = $(HASHDB_CFLAGS)
AM_CXXFLAGS = $(HASHDB_CXXFLAGS)

bloom_filter_manager_test_LDADD = ../libhashdb.la
db_managers_test_LDADD = ../libhashdb.la

AM_LDFLAGS = -static

clean-local:
	rm -rf temp_*

run_tests_valgrind:
	for test in $(bin_PROGRAMS); do \
	  echo Begin $$test ... ; \
	  valgrind --tool=memcheck --leak-check=full --show-reachable=yes --suppressions=vg.supp ./$$test; \
	done

# ############################################################
# tests
# ############################################################
bloom_filter_manager_test_SOURCES = \
	unit_test.h \
	directory_helper.hpp \
	bloom_filter_manager_test.cpp

db_managers_test_SOURCES = \
	../lmdb_hash_manager.hpp \
	../lmdb_helper.h \
	../hashdb_settings.hpp \
	directory_helper.hpp \
	unit_test.h \
	db_managers_test.cpp

.PHONY: run_tests_valgrind

