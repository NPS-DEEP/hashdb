# Author:  Bruce Allen
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

EXTRA_DIST = \
	liblmdb/COPYRIGHT \
	liblmdb/LICENSE \
	rapidjson/allocators.h \
	rapidjson/document.h \
	rapidjson/encodedstream.h \
	rapidjson/encodings.h \
	rapidjson/error/en.h \
	rapidjson/error/error.h \
	rapidjson/filereadstream.h \
	rapidjson/filewritestream.h \
	rapidjson/internal/strfunc.h \
	rapidjson/internal/ieee754.h \
	rapidjson/internal/strtod.h \
	rapidjson/internal/pow10.h \
	rapidjson/internal/dtoa.h \
	rapidjson/internal/swap.h \
	rapidjson/internal/itoa.h \
	rapidjson/internal/meta.h \
	rapidjson/internal/diyfp.h \
	rapidjson/internal/biginteger.h \
	rapidjson/internal/stack.h \
	rapidjson/license.txt \
	rapidjson/memorybuffer.h \
	rapidjson/memorystream.h \
	rapidjson/msinttypes/inttypes.h \
	rapidjson/msinttypes/stdint.h \
	rapidjson/pointer.h \
	rapidjson/prettywriter.h \
	rapidjson/rapidjson.h \
	rapidjson/reader.h \
	rapidjson/README \
	rapidjson/stringbuffer.h \
	rapidjson/writer.h

LMDB_INCS = \
	liblmdb/lmdb.h \
	liblmdb/mdb.c \
	liblmdb/midl.h \
	liblmdb/midl.c

lib_LTLIBRARIES = libhashdb.la
libhashdb_la_LIBADD = liblmdb.la
noinst_LTLIBRARIES = liblmdb.la
liblmdb_la_SOURCES = $(LMDB_INCS)

# may remove -fno-strict-aliasing in future if LMDB gets fixed to allow removal
liblmdb_la_CFLAGS = \
	-pthread -fno-strict-aliasing \
	-W -Wall -Wno-unused-parameter -Wbad-function-cast -Wuninitialized \
	-Wno-format-extra-args

liblmdb_la_LDFLAGS = -PIC

libhashdb_la_LDFLAGS = -PIC -avoid-version -static -no-undefined

AM_CPPFLAGS = \
	-I${top_srcdir}/src_libhashdb/hasher \
	-I${top_srcdir}/src_libhashdb/liblmdb \
	-I${top_srcdir}/src_libhashdb/rapidjson
AM_CFLAGS = $(HASHDB_CFLAGS)
AM_CXXFLAGS = $(HASHDB_CXXFLAGS)

HASHER_INCS = \
	hasher/calculate_block_label.cpp \
	hasher/calculate_block_label.hpp \
	hasher/entropy_calculator.hpp \
	hasher/ewf_file_reader.hpp \
	hasher/filename_list.cpp \
	hasher/filename_list.hpp \
	hasher/filename_t.cpp \
	hasher/filename_t.hpp \
	hasher/file_reader_helper.cpp \
	hasher/file_reader_helper.hpp \
	hasher/file_reader.hpp \
	hasher/hash_calculator.hpp \
	hasher/ingest.cpp \
	hasher/ingest_tracker.hpp \
	hasher/job.hpp \
	hasher/job_queue.hpp \
	hasher/process_job.cpp \
	hasher/process_job.hpp \
	hasher/process_recursive.cpp \
	hasher/process_recursive.hpp \
	hasher/read_bytes.cpp \
	hasher/scan_image.cpp \
	hasher/scan_tracker.hpp \
	hasher/single_file_reader.hpp \
	hasher/threadpool.hpp \
	hasher/uncompress.cpp \
	hasher/uncompress.hpp

LIBHASHDB_INCS = \
	crc32.cpp \
	crc32.h \
	file_modes.h \
	fsync.h \
	hashdb.hpp \
	hex_helper.cpp \
	libhashdb.cpp \
	lmdb_changes.hpp \
	lmdb_context.hpp \
	lmdb_hash_data_manager.hpp \
	lmdb_hash_manager.hpp \
	lmdb_helper.h \
	lmdb_print_val.hpp \
	lmdb_source_data_manager.hpp \
	lmdb_source_id_manager.hpp \
	lmdb_source_name_manager.hpp \
	logger.hpp \
	mutex_lock.hpp \
	msvc-nothrow.h \
	num_cpus.cpp \
	num_cpus.hpp \
	print_environment.hpp \
	settings_manager.hpp \
	tprint.cpp \
	tprint.hpp

libhashdb_la_SOURCES = \
	$(LIBHASHDB_INCS) $(HASHER_INCS)

include_HEADERS = hashdb.hpp

