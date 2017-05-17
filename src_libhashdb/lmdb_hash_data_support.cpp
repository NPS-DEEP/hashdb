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
#include "lmdb_hash_data_support.hpp"
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

namespace hashdb {

const size_t max_block_label_size = 10;
static const size_t type1_max_size = 10+1+max_block_label_size+10+2;
// not used: static const size_t type2_max_size = 10+1+max_block_label_size+4;
static const size_t type3_max_size = 10+2;

// put and get fixed-width numbers
inline uint8_t* put1(uint8_t* p, uint64_t n) {
  if (n > 0xff) {
    std::cerr << "put1 error: " << n << "\n";
    assert(0);
  }
  *p = static_cast<uint8_t>(n);
  return p+1;
}
inline const uint8_t* get1(const uint8_t* const p, uint64_t &n) {
  n = p[0];
  return p+1;
}
inline uint8_t* put2(uint8_t* p, uint64_t n) {
  if (n > 0xffff) {
    std::cerr << "Usage error: lmdb_hash_data_support put2 sub_count " << n << "\n";
    n=0xffff;
  }
  p[0] = static_cast<uint8_t>(n & 0xff);
  p[1] = static_cast<uint8_t>(n >> 8);
  return p+2;
}
inline const uint8_t* get2(const uint8_t* const p, uint64_t &n) {
  n = p[0] | (p[1]<<8);
  return p+2;
}
inline uint8_t* put4(uint8_t* p, uint64_t n) {
  if (n > 0xffffffff) {
    std::cerr << "Usage error: lmdb_hash_data_support put4 sub_count " << n << "\n";
    n=0xffffffff;
  }
  p[0] = static_cast<uint8_t>(n & 0xff);
  p[1] = static_cast<uint8_t>((n >> 8) & 0xff);
  p[2] = static_cast<uint8_t>((n >> 16) & 0xff);
  p[3] = static_cast<uint8_t>((n >> 24) & 0xff);
  return p+4;
}
inline const uint8_t* get4(const uint8_t* const p, uint64_t &n) {
  n = p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24);
  return p+4;
}

// encode Type 1 record
static size_t encode_type1(const uint64_t k_entropy,
                           const std::string& block_label,
                           const uint64_t source_id,
                           const uint64_t sub_count,
                           uint8_t* const p_buf) {

  const size_t block_label_size = block_label.size();
  if (block_label.size() > max_block_label_size) {
    std::cerr << "block_label too large: " << block_label << "\n";
    assert(0);
  }

  uint8_t* p = p_buf;

  // add source_id
  p = lmdb_helper::encode_uint64_t(source_id, p);

  // add sub_count
  p = put2(p, sub_count);

  // add scaled entropy
  p = lmdb_helper::encode_uint64_t(k_entropy, p);

  // add block_label size and block_label
  p = lmdb_helper::encode_uint64_t(block_label_size, p);
  std::memcpy(p, block_label.c_str(), block_label_size);
  p += block_label_size;

  // add padding to allow transition to type2
  if (source_id < 0x4000) {
    *p = 0;
    ++p;
  }
  if (source_id < 0x80) {
    *p = 0;
    ++p;
  }

  // check bounds
  if (p > p_buf + type1_max_size) {
    assert(0);
  }

  // size
  return p - p_buf;
}

// encode Type 2 record
static size_t encode_type2(const uint64_t k_entropy,
                           const std::string& block_label,
                           const uint64_t count,
                           uint8_t* const p_buf) {

  const size_t block_label_size = block_label.size();
  if (block_label.size() > max_block_label_size) {
    std::cerr << "block_label too large: " << block_label << "\n";
    assert(0);
  }

  uint8_t* p = p_buf;

  // add type2 identifier, type2 starts with 0x00
  *p = 0;
  p++;

  // add scaled entropy
  p = lmdb_helper::encode_uint64_t(k_entropy, p);

  // add block_label size and block_label
  p = lmdb_helper::encode_uint64_t(block_label_size, p);
  std::memcpy(p, block_label.c_str(), block_label_size);
  p += block_label_size;

  // add count
  p = put4(p, count);

  // check bounds
  if (p > p_buf + type1_max_size) {
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
  if (p > p_buf + type3_max_size) {
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
    std::cerr << "write_record wrong key size " << key.size()
              << ", " << context.key.mv_size << "\n";
    assert(0);
  } else if (match_size && context.data.mv_size != data_size) {
    std::cerr << "write_record mismatch size " << context.data.mv_size
              << ", " << data_size << "\n";
    assert(0);
  } else if (!match_size && context.data.mv_size < data_size) {
    std::cerr << "write_record larger size " << context.data.mv_size
              << ", " << data_size << "\n";
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

  // Move cursor forward from Type 2 to correct Type 3 else false and rewind.
  // Return sub_count.
  bool cursor_to_type3(hashdb::lmdb_context_t& context,
                       const uint64_t source_id,
                       uint64_t& sub_count) {

    while (true) {
      // get next Type 3 record
      int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                              MDB_NEXT_DUP);

      if (rc == 0) {
        uint64_t existing_source_id;
        uint64_t existing_sub_count;
        decode_type3(context, existing_source_id, existing_sub_count);
        if (existing_source_id == source_id) {
          sub_count = existing_sub_count;
          return true;
        }
      } else if (rc == MDB_NOTFOUND) {
        // back up cursor to Type 2
        rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_FIRST_DUP);
        sub_count = 0;
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

    // read source ID
    p = lmdb_helper::decode_uint64_t(p, source_id);

    // read sub_count
    p = get2(p, sub_count);

    // read scaled entropy
    p = lmdb_helper::decode_uint64_t(p, k_entropy);

    // read the hash data block_label size
    uint64_t block_label_size;
    p = lmdb_helper::decode_uint64_t(p, block_label_size);

    // read the hash data block_label
    block_label =
       std::string(reinterpret_cast<const char*>(p), block_label_size);
    p += block_label_size;

    // compensate for padding
    if (source_id < 0x4000) {
      if (*p != 0) {
        std::cerr << "data decode padding error1 in LMDB hash data store\n";
        assert(0);
      }
      ++p;
    }
    if (source_id < 0x80) {
      if (*p != 0) {
        std::cerr << "data decode padding error1 in LMDB hash data store\n";
        assert(0);
      }
      ++p;
    }

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

    // expect type2 identifier, type2 starts with 0x00
    if (*p != 0) {
      std::cerr << "data decode identifier error in LMDB hash data store\n";
      assert(0);
    }
    p++;

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
    p = get4(p, count);

    // read must fit within data record
    if (p > p_start + context.data.mv_size) {
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
    p = get2(p, sub_count);

    // read must align to data record
    if (p != p_start + context.data.mv_size) {
      std::cerr << "data decode error in LMDB hash data store\n";
      assert(0);
    }
  }

  // write new Type 1 record, key must be valid
  void new_type1(hashdb::lmdb_context_t& context,
                 const std::string& key,
                 const uint64_t k_entropy,
                 const std::string& block_label,
                 const uint64_t source_id,
                 const uint64_t sub_count) {

    // space for encoding
    uint8_t p_buf[type1_max_size];

    // encode type1
    const size_t size = encode_type1(k_entropy, block_label,
                                     source_id, sub_count, p_buf);

    // write
    write_record(context, key, p_buf, size);
  }

  // write new Type 3 record, key must be valid
  void new_type3(hashdb::lmdb_context_t& context,
                 const std::string& key,
                 const uint64_t source_id,
                 const uint64_t sub_count) {

    // space for encoding
    uint8_t p_buf[type3_max_size];

    // encode type3
    const size_t size = encode_type3(source_id, sub_count, p_buf);

    // write
    write_record(context, key, p_buf, size);
  }

  // replace Type 1 record at cursor
  void replace_type1(hashdb::lmdb_context_t& context,
                     const std::string& key,
                     const uint64_t k_entropy,
                     const std::string& block_label,
                     const uint64_t source_id,
                     const uint64_t sub_count) {

    // space for encoding
    uint8_t p_buf[type1_max_size];

    // encode type1
    const size_t size = encode_type1(k_entropy, block_label,
                                     source_id, sub_count, p_buf);

    // write
    replace_record(context, key, p_buf, size, true);
  }

  // replace Type 2 record at cursor
  void replace_type2(hashdb::lmdb_context_t& context,
                     const std::string& key,
                     const uint64_t k_entropy,
                     const std::string& block_label,
                     const uint64_t count) {

    // space for encoding that can replace old type1
    uint8_t p_buf[type1_max_size];

    // encode type1
    const size_t size = encode_type2(k_entropy, block_label, count, p_buf);

    // write
    replace_record(context, key, p_buf, size, false);
  }

  // replace Type 3 record at cursor
  void replace_type3(hashdb::lmdb_context_t& context,
                     const std::string& key,
                     const uint64_t& source_id,
                     const uint64_t& sub_count) {

    // space for encoding
    uint8_t p_buf[type3_max_size];

    // encode type3
    const size_t size = encode_type3(source_id, sub_count, p_buf);

    // write
    replace_record(context, key, p_buf, size, true);
  }

} // end namespace hashdb

