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
	lmdb_managers_test \
        hashdb_managers_test

TESTS = $(check_PROGRAMS)

AM_CFLAGS = $(HASHDB_CFLAGS)
AM_CXXFLAGS = $(HASHDB_CXXFLAGS)

AM_LDFLAGS = -static

bloom_filter_manager_test_CPPFLAGS = \
	-I${top_srcdir}/src_libhashdb \
	-isystem${top_srcdir}/src_libhashdb/libbloom \
	-isystem${top_srcdir}/src_libhashdb/libdfxml

lmdb_managers_test_CPPFLAGS = \
	-I${top_srcdir}/src_libhashdb \
	-isystem${top_srcdir}/src_libhashdb/libbloom \
	-isystem${top_srcdir}/src_libhashdb/libdfxml \
	-isystem${top_srcdir}/src_libhashdb/liblmdb \
	$(XML_CFLAGS)

lmdb_managers_test_LDADD = ../src_libhashdb/liblmdb.la

hashdb_managers_test_LDADD = ../src_libhashdb/libhashdb.la

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
bloom_filter_manager_test_SOURCES = bloom_filter_manager_test.cpp \
	../src_libhashdb/libbloom/bloom.c \
	../src_libhashdb/libbloom/file_mapper.cpp

lmdb_managers_test_SOURCES = lmdb_managers_test.cpp \
	../src_libhashdb/libbloom/bloom.c \
	../src_libhashdb/libbloom/file_mapper.cpp \
	../src_libhashdb/libdfxml/dfxml_writer.cpp

hashdb_managers_test_SOURCES = hashdb_managers_test.cpp

.PHONY: run_tests_valgrind

