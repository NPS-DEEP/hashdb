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
 * Provides low-level support for moving the cursor and for reading and
 * writing Type1, Type2, and Type3 records in lmdb_hash_data_store.
 * See lmdb_hash_data_manager.
 */

// #define DEBUG_LMDB_HASH_DATA_SUPPORT_HPP

#include <iostream>
#include <config.h>
// this process of getting WIN32 defined was inspired
// from i686-w64-mingw32/sys-root/mingw/include/windows.h.
// All this to include winsock2.h before windows.h to avoid a warning.
#if defined(__MINGW64__) && defined(__cplusplus)
#  ifndef WIN32
#    define WIN32
#  endif
#endif
#ifdef WIN32
  // including winsock2.h now keeps an included header somewhere from
  // including windows.h first, resulting in a warning.
  #include <winsock2.h>
#endif

#include "lmdb.h"
#include "lmdb_helper.h"
#include "lmdb_context.hpp"
#include "tprint.hpp"
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <string>
#include <cassert>

#ifdef DEBUG_LMDB_HASH_DATA_SUPPORT_HPP
#include "lmdb_print_val.hpp"
#endif

const int max_block_label_size = 10;
static const int type1_max_size = 10+1+max_block_label_size+10+2;
static const int type3_max_size = 10+2;

// put and get fixed-width numbers
inline uint8_t* put1(uint8_t* p, uint64_t n) {
  if (n > 0xff) {
    std::cerr << "put1 error: " << n << "\n";
    assert(0);
  }
  *p = static_cast<uint8_t>(n);
  return p+1;
}
inline uint8_t* get1(uint8_t* p, &n) {
  n = p[0];
  return p+1;
}
inline uint8_t* put2(uint8_t* p, uint64_t n) {
  if (n > 0xffff) {
    std::cerr << "put2 error: " << n << "\n";
    assert(0);
  }
  p[0] = static_cast<uint8_t>(n & 0xff);
  p[1] = static_cast<uint8_t>(n >> 8);
  return p+2;
}
inline uint8_t* get2(uint8_t* p, &n) {
  n = p[0] | (p[1]>>8);
  return p+2;
}
inline uint8_t* put4(uint8_t* p, uint64_t n) {
  if (n > 0xffffffff) {
    std::cerr << "put4 error: " << n << "\n";
    assert(0);
  }
  p[0] = static_cast<uint8_t>(n & 0xff);
  p[1] = static_cast<uint8_t>((n >> 8) & 0xff);
  p[2] = static_cast<uint8_t>((n >> 16) & 0xff);
  p[3] = static_cast<uint8_t>((n >> 24) & 0xff);
  return p+4;
}
inline uint8_t* get4(uint8_t* p, &n) {
  n = p[0] | (p[1]>>8) | (p[2]>>16) | (p[3]>>24);
  return p+4;
}

// encode Type 1 record
static size_t encode_type1(uint64_t k_entropy,
                           const std::string& block_label,
                           uint64_t source_id,
                           uint64_t sub_count,
                           uint8_t* const p_buf) {

  if (block_label.size() > max_block_label_size) {
    std::cerr << "block_label too large: " << block_label << "\n";
    assert(0);
  }

  uint8_t* p = p_buf;

  // add scaled entropy
  p = lmdb_helper::encode_uint64_t(k_entropy, p);

  // add block_label size and block_label
  p = lmdb_helper::encode_uint64_t(block_label_size, p);
  std::memcpy(p, block_label.c_str(), block_label_size);
  p += block_label_size;

  // add source_id
  p = lmdb_helper::encode_uint64_t(source_id, p);

  // add sub_count
  p = put2(p, sub_count);

  // check bounds
  if (p - p_buf > type1_max_size) {
    assert(0);
  }

  // size
  return p - p_buf;
}

// encode Type 2 record
static size_t encode_type2(uint64_t k_entropy,
                           const std::string& block_label,
                           uint64_t count,
                           uint8_t* const p_buf) {

  if (block_label.size() > max_block_label_size) {
    std::cerr << "block_label too large: " << block_label << "\n";
    assert(0);
  }

  uint8_t* p = p_buf;

  // add scaled entropy
  p = lmdb_helper::encode_uint64_t(k_entropy, p);

  // add block_label size and block_label
  p = lmdb_helper::encode_uint64_t(block_label_size, p);
  std::memcpy(p, block_label.c_str(), block_label_size);
  p += block_label_size;

  // add count
  p = put4(p, count);

  // check bounds
  if (p - p_buf > type2_max_size) {
    assert(0);
  }

  // size
  return p - p_buf;
}

// encode Type 3 record
static size_t encode_type3(uint64_t source_id,
                           uint64_t sub_count,
                           uint8_t* const p_buf) {

  uint8_t* p = p_buf;

  // add source_id
  p = lmdb_helper::encode_uint64_t(source_id, p);

  // add sub_count
  p = put2(p, sub_count);

  // check bounds
  if (p - p_buf > type3_max_size) {
    assert(0);
  }

  // size
  return p - p_buf;
}

// write the record given key and data.
static void write_record(hashdb::lmdb_context_t& context,
                         const std::string& key,
                         const uint8_t* const data, const size_t data_size) {

  // set key and data
  context.key.mv_size = key.size();
  context.key.mv_data = static_cast<uint8_t*>(
             static_cast<void*>(const_cast<char*>(key.c_str())));
  context.data.mv_size = data_size;
  context.data.mv_data = const_cast<uint8_t*>(data);

#ifdef DEBUG_LMDB_HASH_DATA_SUPPORT_HPP
print_mdb_val("hash_data_support write_record key", context.key);
print_mdb_val("hash_data_support write_record data", context.data);
#endif

  int rc = mdb_cursor_put(context.cursor, &context.key, &context.data,
                          MDB_NODUPDATA);
  if (rc != 0) {
    std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
    assert(0);
  }
}

// replace the record given data.  Types 1 and 3 must match size.
// New type 2 can be smaller but the record size must stay the same.
static void replace_record(hashdb::lmdb_context_t& context,
                           const std::string& key,
                           const uint8_t* const data, const size_t data_size,
                           bool match_size) {

  // validate size
  if (key.size() != context.key.mv_size) {
    std::cerr << "write_record wrong key size\n";
    assert(0);
  } else if (match_size && context.data.mv_size != data_size) {
    std::cerr << "write_record mismatch size\n";
    assert(0);
  } else if (!match_size && context.data.mv_size < data_size) {
    std::cerr << "write_record larger size\n";
    assert(0);
  }

  // point to key and data
  context.key.mv_data = static_cast<uint8_t*>(
             static_cast<void*>(const_cast<char*>(key.c_str())));
  context.data.mv_data = const_cast<uint8_t*>(data);

#ifdef DEBUG_LMDB_HASH_DATA_SUPPORT_HPP
print_mdb_val("hash_data_support replace_record key", context.key);
print_mdb_val("hash_data_support replace_record data", context.data);
#endif

  // replace
  int rc = mdb_cursor_put(context.cursor, &context.key, &context.data,
                          MDB_CURRENT);
  if (rc != 0) {
    std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
    assert(0);
  }
}

namespace hashdb {

  // move cursor to first entry of current key
  void cursor_to_first_current(hashdb::lmdb_context_t& context) {
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_FIRST_DUP);

    // the move must work
    if (rc != 0) {
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
  }

  // move cursor forward from Type 2 to correct Type 3 else false and rewind
  bool cursor_to_type3(hashdb::lmdb_context_t& context,
                       const uint64_t source_id) {

    while (true) {
      // get next Type 3 record
      int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                              MDB_NEXT_DUP);

      if (rc == 0) {
        uint64_t next_source_id;
        decode_type3_source_id(context, next_source_id);
        if (source_id == next_source_id) {
          return true;
        }
      } else if (rc == MDB_NOTFOUND) {
        // back up cursor to Type 2
        rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_FIRST_DUP);
        return false;
      } else {
        // invalid rc
        std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
        assert(0);
      }
    }
  }

  // parse Type 1 context.data into these parameters
  void decode_type1(hashdb::lmdb_context_t& context,
                    uint64_t& k_entropy,
                    std::string& block_label,
                    uint64_t& source_id,
                    uint64_t& sub_count) {

#ifdef DEBUG_LMDB_HASH_DATA_SUPPORT_HPP
print_mdb_val("hash_data_support decode_type1 key", context.key);
print_mdb_val("hash_data_support decode_type1 data", context.data);
#endif

    // prepare to read Type 1 entry:
    const uint8_t* const p_start = static_cast<uint8_t*>(context.data.mv_data);
    const uint8_t* p = p_start;

    // read scaled entropy
    p = lmdb_helper::decode_uint64_t(p, k_entropy);

    // read the hash data block_label size
    uint64_t block_label_size;
    p = lmdb_helper::decode_uint64_t(p, block_label_size);

    // read the hash data block_label
    block_label =
       std::string(reinterpret_cast<const char*>(p), block_label_size);
    p += block_label_size;

    // read source ID
    p = lmdb_helper::decode_uint64_t(p, source_id);

    // read sub_count
    p = get2(p, sub_count)

    // read must align to data record
    if (p != p_start + context.data.mv_size) {
      std::cerr << "data decode error in LMDB hash data store\n";
      assert(0);
    }
  }

  // parse Type 2 context.data into these parameters
  void decode_type2(hashdb::lmdb_context_t& context,
                    uint64_t& k_entropy,
                    std::string& block_label,
                    uint64_t& count) {

#ifdef DEBUG_LMDB_HASH_DATA_SUPPORT_HPP
print_mdb_val("hash_data_support decode_type2 key", context.key);
print_mdb_val("hash_data_support decode_type2 data", context.data);
#endif

    // prepare to read Type 2 entry:
    const uint8_t* const p_start = static_cast<uint8_t*>(context.data.mv_data);
    const uint8_t* p = p_start;

    // read scaled entropy
    p = lmdb_helper::decode_uint64_t(p, k_entropy);

    // read the hash data block_label size
    uint64_t block_label_size;
    p = lmdb_helper::decode_uint64_t(p, block_label_size);

    // read the hash data block_label
    block_label =
       std::string(reinterpret_cast<const char*>(p), block_label_size);
    p += block_label_size;

    // read count
    p = get4(p, count)

    // read must align to data record
    if (p != p_start + context.data.mv_size) {
      std::cerr << "data decode error in LMDB hash data store\n";
      assert(0);
    }
  }

  // parse Type 3 context.data into these parameters
  void decode_type3(hashdb::lmdb_context_t& context,
                    uint64_t& source_id,
                    uint64_t& sub_count) {

#ifdef DEBUG_LMDB_HASH_DATA_SUPPORT_HPP
print_mdb_val("hash_data_support decode_type3 key", context.key);
print_mdb_val("hash_data_support decode_type3 data", context.data);
#endif

    // prepare to read Type 1 entry:
    const uint8_t* const p_start = static_cast<uint8_t*>(context.data.mv_data);
    const uint8_t* p = p_start;

    // read source ID
    p = lmdb_helper::decode_uint64_t(p, source_id);

    // read sub_count
    p = get2(p, sub_count)

    // read must align to data record
    if (p != p_start + context.data.mv_size) {
      std::cerr << "data decode error in LMDB hash data store\n";
      assert(0);
    }
  }

  // write new Type 1 record, key must be valid
  void new_type1(hashdb::lmdb_context_t& context,
                 const std::string& key,
                 uint64_t k_entropy,
                 const std::string& block_label,
                 uint64_t source_id,
                 uint64_t sub_count) {

    // space for encoding
    const uint8_t* p_buf = data[type1_max_size];

    // encode type1
    const size_t size = encode_type1(k_entropy, block_label,
                                     source_id, sub_count, p_buf);

    // write
    write_record(context, key, p_buf, size);
  }

  // write new Type 3 record, key must be valid
  void new_type3(hashdb::lmdb_context_t& context,
                 const std::string& key,
                 uint64_t source_id,
                 uint64_t sub_count) {

    // space for encoding
    const uint8_t* p_buf = data[type3_max_size];

    // encode type3
    const size_t size = encode_type3(source_id, sub_count, p_buf);

    // write
    write_record(context, key, p_buf, p - p_buf);
  }

  // replace Type 1 record at cursor
  void replace_type1(hashdb::lmdb_context_t& context,
                     const std::string& key,
                     uint64_t k_entropy,
                     const std::string& block_label,
                     uint64_t source_id,
                     uint64_t sub_count) {

    // space for encoding
    const uint8_t* p_buf = data[type1_max_size];

    // encode type1
    const size_t size = encode_type1(k_entropy, block_label,
                                     source_id, sub_count, p_buf);

    // write
    replace_record(context, key, p_buf, size, true);
  }

  // replace Type 2 record at cursor
  void replace_type2(hashdb::lmdb_context_t& context,
                     const std::string& key,
                     uint64_t k_entropy,
                     const std::string& block_label,
                     uint64_t count) {

    // space for encoding that can replace old type1
    const uint8_t* p_buf = data[type1_max_size];

    // encode type1
    const size_t size = encode_type2(k_entropy, block_label, count, p_buf);

    // write
    replace_record(context, key, p_buf, size, true);
  }

  // replace Type 3 record at cursor
  void replace_type3(hashdb::lmdb_context_t& context,
                     const std::string& key,
                     uint64_t& source_id,
                     uint64_t& sub_count) {

    // space for encoding
    const uint8_t* p_buf = data[type3_max_size];

    // encode type3
    const size_t size = encode_type3(source_id, sub_count, p_buf);

    // write
    replace_record(context, key, p_buf, size, true);
  }

} // end namespace hashdb

#endif

