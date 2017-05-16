// Author:  Bruce Allen
// Created: 2/25/2013
//
// The software provided here is released by the Naval Postgraduate
// School, an agency of the U.S. Department of Navy.  The software
// bears no warranty, either expressed or implied. NPS does not assume
// legal liability nor responsibility for a User's use of the software
// or the results of such use.
//
// Please note that within the United States, copyright protection,
// under Section 105 of the United States Code, Title 17, is not
// available for any work of the United States Government and/or for
// any works created by United States Government employees. User
// acknowledges that this software contains work which was created by
// NPS government employees and is therefore in the public domain and
// not subject to copyright.
//
// Released into the public domain on February 25, 2013 by Bruce Allen.

/**
 * \file
 * Provide support for LMDB operations.
 *
 * Note: it would be nice if MDB_val had a const type and a non-const type
 * to handle reading vs. writing.  Instead, we hope the callee works right.
 */

#ifndef LMDB_HELPER_H
#define LMDB_HELPER_H

#include "sys/stat.h"
#include "lmdb.h"
#include "file_modes.h"
#include <stdexcept>
#include <cassert>
#include <stdint.h>
#include <cstring>
#include <sstream>
#include <unistd.h>
#include <iomanip>
#include <pthread.h>
#include <iostream>

//#define DEBUG

namespace lmdb_helper {

  // write value into encoding, return pointer past value written.
  // each write will add no more than 10 bytes.
  // note: code adapted directly from:
  // https://code.google.com/p/protobuf/source/browse/trunk/src/google/protobuf/io/coded_stream.cc?r=417
  uint8_t* encode_uint64_t(uint64_t value, uint8_t* target);

  // read pointer into value, return pointer past value read.
  // each read will consume no more than 10 bytes.
  // note: code adapted directly from:
  // https://code.google.com/p/protobuf/source/browse/trunk/src/google/protobuf/io/coded_stream.cc?r=417
  const uint8_t* decode_uint64_t(const uint8_t* p_ptr, uint64_t& value);

  MDB_env* open_env(const std::string& store_dir,
                           const hashdb::file_mode_type_t file_mode);

  void maybe_grow(MDB_env* env);

  // size
  size_t size(MDB_env* env);
}

#endif

