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

python_tests = \
	new_database.py \
	import_export.py

EXTRA_DIST = \
	vg.supp \
        $(python_tests) \
        hashdb_helpers.py

noinst_LTLIBRARIES = liblmdb.la

liblmdb_la_SOURCES = \
	../lmdb/libraries/liblmdb/lmdb.h \
	../lmdb/libraries/liblmdb/mdb.c \
	../lmdb/libraries/liblmdb/midl.h \
	../lmdb/libraries/liblmdb/midl.c

# these CFLAGS are adapted from liblmdb/Makefile
liblmdb_la_CFLAGS = \
	-pthread -O3 -g \
	-W -Wall -Wno-unused-parameter -Wbad-function-cast -Wuninitialized
liblmdb_la_LDFLAGS = -static -PIC

check_PROGRAMS = \
	db_test \
	bloom_filter_manager_test \
	settings_test \
	logger_test \
	libhashdb_test \
	commands_test

TESTS = $(check_PROGRAMS)
if HAVE_PYTHON
  TESTS += $(python_tests)
endif

# to validate backward compatibility to c++98, add option -std=c++98
AM_CPPFLAGS = -isystem${top_srcdir}/src/misc \
	-isystem${top_srcdir}/src/btree/include \
	-isystem${top_srcdir}/src/endian/include \
	-isystem${top_srcdir}/src/lmdb/libraries/liblmdb \
	-I${top_srcdir}/src \
	$(XML_CFLAGS)

AM_CFLAGS = $(HASHDB_CFLAGS)
AM_CXXFLAGS = $(HASHDB_CXXFLAGS)

db_test_LDADD = liblmdb.la
bloom_filter_manager_test_LDADD = ../libhashdb.la
settings_test_LDADD = ../libhashdb.la
logger_test_LDADD = ../libhashdb.la
libhashdb_test_LDADD = ../libhashdb.la
commands_test_LDADD = ../libhashdb.la

AM_LDFLAGS = -static

clean-local:
	rm -rf temp_dir* temp_dfxml_out.xml

# test with valgrind memory analysis
run_tests_valgrind:
	for test in $(bin_PROGRAMS); do \
	  echo Begin $$test ... ; \
	  valgrind --tool=memcheck --leak-check=full --show-reachable=yes --suppressions=vg.supp ./$$test; \
	done

# ############################################################
# tests
# ############################################################
db_test_SOURCES = \
	../lmdb_context.hpp \
	../lmdb_hash_it_data.hpp \
	../lmdb_hash_store.hpp \
	../lmdb_helper.h \
	../lmdb_name_store.hpp \
	../lmdb_source_data.hpp \
	../lmdb_source_it_data.hpp \
	../lmdb_source_store.hpp \
	unit_test.h \
	db_test.cpp

bloom_filter_manager_test_SOURCES = \
	unit_test.h \
	directory_helper.hpp \
	to_key_helper.hpp \
	bloom_filter_manager_test.cpp

settings_test_SOURCES = \
	unit_test.h \
	directory_helper.hpp \
	settings_test.cpp

logger_test_SOURCES = \
	unit_test.h \
	directory_helper.hpp \
	logger_test.cpp

libhashdb_test_SOURCES = \
	unit_test.h \
	directory_helper.hpp \
	to_key_helper.hpp \
	libhashdb_test.cpp
	
commands_test_SOURCES = \
	unit_test.h \
	directory_helper.hpp \
	commands_test.cpp

.PHONY: run_tests_valgrind

