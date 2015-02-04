// Author:  Bruce Allen <bdallen@nps.edu>
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
#include "file_modes.h"
#include "lmdb.h"
#include "lmdb_source_data.hpp"
#include <cstdint>
#include <sstream>

class lmdb_helper {
  private:
  // read pointer into value, return pointer past value read.
  // each read will consume no more than 10 bytes.
  // note: code adapted directly from:
  // https://code.google.com/p/protobuf/source/browse/trunk/src/google/protobuf/io/coded_stream.cc?r=417 
  static const uint8_t* private_encoding_to_uint64(const uint8_t* p_ptr, uint64_t* value) {

    const uint8_t* ptr = p_ptr;
    uint32_t b;

    // Splitting into 32-bit pieces gives better performance on 32-bit
    // processors.
    uint32_t part0 = 0, part1 = 0, part2 = 0;

    b = *(ptr++); part0  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
    b = *(ptr++); part0 |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;
    b = *(ptr++); part0 |= (b & 0x7F) << 14; if (!(b & 0x80)) goto done;
    b = *(ptr++); part0 |= (b & 0x7F) << 21; if (!(b & 0x80)) goto done;
    b = *(ptr++); part1  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
    b = *(ptr++); part1 |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;
    b = *(ptr++); part1 |= (b & 0x7F) << 14; if (!(b & 0x80)) goto done;
    b = *(ptr++); part1 |= (b & 0x7F) << 21; if (!(b & 0x80)) goto done;
    b = *(ptr++); part2  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
    b = *(ptr++); part2 |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;

    // We have overrun the maximum size of a varint (10 bytes).  The data
    // must be corrupt.
    std::cerr << "corrupted uint64 protocol buffer\n";
    assert(0);

   done:
    *value = (static_cast<uint64_t>(part0)      ) |
             (static_cast<uint64_t>(part1) << 28) |
             (static_cast<uint64_t>(part2) << 56);
    return ptr;
}

  // write value into encoding, return pointer past value written.
  // each write will add no more than 10 bytes.
  // note: code adapted directly from:
  // https://code.google.com/p/protobuf/source/browse/trunk/src/google/protobuf/io/coded_stream.cc?r=417 
  inline static uint8_t* private_uint64_to_encoding(uint64_t value, uint8_t* target) {

    // Splitting into 32-bit pieces gives better performance on 32-bit
    // processors.
    uint32_t part0 = static_cast<uint32_t>(value      );
    uint32_t part1 = static_cast<uint32_t>(value >> 28);
    uint32_t part2 = static_cast<uint32_t>(value >> 56);

    int size;

    // hardcoded binary search tree...
    if (part2 == 0) {
      if (part1 == 0) {
        if (part0 < (1 << 14)) {
          if (part0 < (1 << 7)) {
            size = 1; goto size1;
          } else {
            size = 2; goto size2;
          }
        } else {
          if (part0 < (1 << 21)) {
            size = 3; goto size3;
          } else {
            size = 4; goto size4;
          }
        }
      } else {
        if (part1 < (1 << 14)) {
          if (part1 < (1 << 7)) {
            size = 5; goto size5;
          } else {
            size = 6; goto size6;
          }
        } else {
          if (part1 < (1 << 21)) {
            size = 7; goto size7;
          } else {
            size = 8; goto size8;
          }
        }
      }
    } else {
      if (part2 < (1 << 7)) {
        size = 9; goto size9;
      } else {
        size = 10; goto size10;
      }
    }

    // bad if here
    assert(0);

    size10: target[9] = static_cast<uint8_t>((part2 >>  7) | 0x80);
    size9 : target[8] = static_cast<uint8_t>((part2      ) | 0x80);
    size8 : target[7] = static_cast<uint8_t>((part1 >> 21) | 0x80);
    size7 : target[6] = static_cast<uint8_t>((part1 >> 14) | 0x80);
    size6 : target[5] = static_cast<uint8_t>((part1 >>  7) | 0x80);
    size5 : target[4] = static_cast<uint8_t>((part1      ) | 0x80);
    size4 : target[3] = static_cast<uint8_t>((part0 >> 21) | 0x80);
    size3 : target[2] = static_cast<uint8_t>((part0 >> 14) | 0x80);
    size2 : target[1] = static_cast<uint8_t>((part0 >>  7) | 0x80);
    size1 : target[0] = static_cast<uint8_t>((part0      ) | 0x80);

    target[size-1] &= 0x7F;
    return target + size;
  }

  class private_string_reader {
    const char* ptr;
    size_t max;
    public:
    private_string_reader(const char* p_ptr, size_t p_max) :
                 ptr(p_ptr), max(p_max) {
    }
    std::string get() {
      const char* p2 = (char*)memchr(ptr, 0, max);
      std::string s(ptr, (p2==NULL ? max : p2 - ptr));
      if (p2 == NULL) {
        max = 0;
      } else {
        size_t count = p2 - ptr + 1;
        max -= count;
        ptr += count;
      }
      return s;
    }
    ~private_string_reader() {
      if (max != 0) {
        std::cout << "Data error: unread data.\n";
        //assert(0);
      }
    }
  };

  public:
  static MDB_env* open_env(const std::string& store_dir,
                           file_mode_type_t file_mode) {

    // create the DB environment
    MDB_env* env;
    int rc = mdb_env_create(&env);
    if (rc != 0) {
      // bad failure
      assert(0);
    }

    // set flags for open
    unsigned int env_flags;
    switch(file_mode) {
      case READ_ONLY:
        env_flags = MDB_RDONLY;
        break;
      case RW_NEW:
        // store directory must not exist yet
        if (access(store_dir.c_str(), F_OK) == 0) {
          std::cerr << "Error: Database '" << store_dir
                    << "' already exists.  Aborting.\n";
          exit(1);
        }

        // create the store directory
#ifdef WIN32
        if(mkdir(store_dir.c_str())){
          std::cerr << "Error: Could not make new store directory '"
                    << store_dir << "'.\nCannot continue.\n";
          exit(1);
        }
#else
        if(mkdir(store_dir.c_str(),0777)){
          std::cerr << "Error: Could not make new store directory '"
                    << store_dir << "'.\nCannot continue.\n";
          exit(1);
        }
#endif
        // NOTE: These flags improve performance significantly so use them.
        // No sync means no requisite disk action after every transaction.
        // writemap suppresses checking but improves Windows performance.
        env_flags = MDB_NOMETASYNC | MDB_NOSYNC | MDB_WRITEMAP;
        break;
      case RW_MODIFY:
        env_flags = MDB_NOMETASYNC | MDB_NOSYNC | MDB_WRITEMAP;
        break;
      default:
        env_flags = 0; // satisfy mingw32-g++ compiler
        assert(0);
    }

    // open the MDB environment
    rc = mdb_env_open(env, store_dir.c_str(), env_flags, 0664);
    if (rc != 0) {
      // fail
      std::cerr << "Error opening store: " << store_dir
                << ": " <<  mdb_strerror(rc) << "\nAborting.\n";
      exit(1);
    }

    return env;
  }

  static void maybe_grow(MDB_env* env) {
    // http://comments.gmane.org/gmane.network.openldap.technical/11699
    // also see mdb_env_set_mapsize

    // read environment info
    MDB_envinfo env_info;
    int rc = mdb_env_info(env, &env_info);
    if (rc != 0) {
      assert(0);
    }

    // get page size
    MDB_stat ms;
    rc = mdb_env_stat(env, &ms);
    if (rc != 0) {
      assert(0);
    }

    // maybe grow the DB
    if (env_info.me_mapsize / ms.ms_psize == env_info.me_last_pgno + 2) {

      // full so grow the DB, safe since this code is locked
#ifdef DEBUG
      std::cout << "Growing hash store DB from " << env_info.me_mapsize
                << " to " << env_info.me_mapsize * 2 << "\n";
#endif

      // could call mdb_env_sync(env, 1) here but it does not help

      // grow the DB
      rc = mdb_env_set_mapsize(env, env_info.me_mapsize * 2);
      if (rc != 0) {
        // grow failed
        std::cerr << "Error growing DB: " <<  mdb_strerror(rc)
                  << "\nAborting.\n";
        exit(1);
      }
    }
  }

  // size
  static size_t size(MDB_env* env) {

    // obtain statistics
    MDB_stat stat;
    int rc = mdb_env_stat(env, &stat);
    if (rc != 0) {
      // program error
      std::cerr << "size failure: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
#ifdef DEBUG
    std::cout << "size: " << stat.ms_entries << "\n";
#endif
    return stat.ms_entries;
  }

  static uint64_t encoding_to_uint64(const MDB_val& val) {
    uint64_t n;
    private_encoding_to_uint64(
        static_cast<const uint8_t*>(const_cast<const void*>(val.mv_data)), &n);
    return n;
  }

  static std::pair<uint64_t, uint64_t> encoding_to_uint64_pair(const MDB_val& val) {
    uint64_t n1;
    uint64_t n2;
    const uint8_t* ptr2 = private_encoding_to_uint64(
        static_cast<const uint8_t*>(const_cast<const void*>(val.mv_data)), &n1);
    const uint8_t* ptr3 = private_encoding_to_uint64(ptr2, &n2);
    if (ptr3 - static_cast<const uint8_t*>(const_cast<const void*>(val.mv_data))
                      > val.mv_size) {
      // corrupt data
      std::cerr << "corrupt data on read.\n";
      assert(0);
    }
    return std::pair<uint64_t, uint64_t>(n1, n2);
  }

  static std::string uint64_to_encoding(uint64_t n) {
    // set value
    uint8_t ptr[10];
    uint8_t* ptr2 = private_uint64_to_encoding(n, ptr);
    return std::string(reinterpret_cast<char*>(ptr), ptr2 - ptr);
  }

  static std::string uint64_pair_to_encoding(uint64_t n1,
                                             uint64_t n2) {
    // set value
    uint8_t ptr[10*2];
    uint8_t* ptr2 = private_uint64_to_encoding(n1, ptr);
    uint8_t* ptr3 = private_uint64_to_encoding(n2, ptr2);
    return std::string(reinterpret_cast<char*>(ptr), ptr3 - ptr);
  }

  static std::string string_pair_to_encoding(const std::string& s1,
                                             const std::string& s2) {
    // build cstr
    size_t l1 = s1.length();
    size_t l2 = s2.length();
    size_t l3 = l1 + 1 + l2;
    char cstr[l3];  // space for strings separated by \0
    std::strcpy(cstr, s1.c_str()); // copy first plus \0
    std::memcpy(cstr+l1+1, s2.c_str(), l2);
    std::string encoding(cstr, l3);
    return std::string(cstr, l3);
  }

  static std::string lmdb_source_data_to_encoding(const lmdb_source_data_t data) {
    size_t l1 = data.repository_name.size();
    size_t l2 = data.filename.size();
    size_t l3 = data.filesize.size();
    size_t l4 = data.hashdigest.size();
    
    // fill ordered null-delimited record from fields
    char chars[l1+1+l2+1+l3+1+l4];
    std::memcpy(chars, data.repository_name.c_str(), l1);
    size_t start = l1;
    chars[start++] = 0;
    std::memcpy(chars+start, data.filename.c_str(), l2);
    start += l2;
    chars[start++] = 0;
    std::memcpy(chars+start, data.filesize.c_str(), l3);
    start += l3;
    chars[start++] = 0;
    std::memcpy(chars+start, data.hashdigest.c_str(), l4);

    // return the encoding
    return std::string(chars, l1+1+l2+1+l3+1+l4);
  }

  static lmdb_source_data_t encoding_to_lmdb_source_data(const MDB_val& val) {
    private_string_reader reader(static_cast<char*>(val.mv_data), val.mv_size);
    lmdb_source_data_t data;
    data.repository_name = reader.get();
    data.filename = reader.get();
    data.filesize = reader.get();
    data.hashdigest = reader.get();
    return data;
  }

  static void point_to_string(const std::string& str, MDB_val& val) {
    val.mv_size = str.size();
    val.mv_data = static_cast<void*>(const_cast<char*>(str.c_str()));
  }

  static std::string get_string(const MDB_val& val) {
    return std::string(static_cast<char*>(val.mv_data), val.mv_size);
  }

  // return empty if not even or if any invalid input
  static std::string hex_to_binary_hash(const std::string& hex_string) {

    size_t size = hex_string.size();
    // size must be even
    if (size%2 != 0) {
      std::cerr << "input not even aligned\n";
      return "";
    }

    int i = 0;
    int j = 0;
    uint8_t bin[size];
    for (; i<size; i+=2) {
      uint8_t c0 = hex_string[i];
      uint8_t c1 = hex_string[i+1];
      uint8_t d0;
      uint8_t d1;

      if(c0>='0' && c0<='9') d0 = c0-'0';
      else if(c0>='a' && c0<='f') d0 = c0-'a'+10;
      else if(c0>='A' && c0<='F') d0 = c0-'A'+10;
      else {
        std::cout << "unexpected hex character\n";
        return "";
      }
 
      if(c1>='0' && c1<='9') d1 = c1-'0';
      else if(c1>='a' && c1<='f') d1 = c1-'a'+10;
      else if(c1>='A' && c1<='F') d1 = c1-'A'+10;
      else {
        std::cout << "unexpected hex character\n";
        return "";
      }

      bin[j++] = d0 << 4 | d1;
    }
    return std::string(reinterpret_cast<char*>(bin), j);
  }
      
  static std::string binary_hash_to_hex(const std::string& binary_hash) {
    size_t size = binary_hash.size();
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(binary_hash.c_str());
    std::stringstream ss;
    ss << std::hex;
    for (int i=0; i<size; i++) {
      ss << (uint32_t)ptr[i]; // uint8_t fails with << operator
    }
    return ss.str();
  }
};

#endif

