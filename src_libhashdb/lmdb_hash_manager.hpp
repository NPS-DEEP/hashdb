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
 * Manage the LMDB hash store.  Threadsafe.
 */

/** The following Python program generates example count encodings:
#!/usr/bin/env python3
#
# Show some values for 4-bit high, 4-bit low encoding

lookup = [1, 5, 25, 125, 625, 3125, 15625, 78125, 390625, 1953125, 9765625,
          48828125, 244140625, 1220703125, 6103515625, 30517578125]

for i in range(1500):

    m = i + 5

    x = 0
    while m > 19:
        x+=1
        m = m // 5

    # shift m down by 4
    if m > 4:
        m -= 4
    else:
        m = 0

    approximate_count = (m+4) * lookup[x] - 5

    print("before: %d   after: %d   x: %d m:%d" % (i, approximate_count, x, m))
*/

#ifndef LMDB_HASH_MANAGER_HPP
#define LMDB_HASH_MANAGER_HPP

//#define DEBUG_LMDB_HASH_MANAGER_HPP

#include "file_modes.h"
#include "lmdb.h"
#include "lmdb_helper.h"
#include "lmdb_context.hpp"
#include "lmdb_changes.hpp"
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <string>
#include <set>
#include <cassert>
#ifdef DEBUG_LMDB_HASH_MANAGER_HPP
#include "lmdb_print_val.hpp"
#endif

// no concurrent writes
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "mutex_lock.hpp"

namespace hashdb {

static uint8_t masks[8] = {0xff,0x80,0xc0,0xe0,0xf0,0xf8,0xfc,0xfe};

class lmdb_hash_manager_t {

  private:
  const std::string hashdb_dir;
  const hashdb::file_mode_type_t file_mode;
  const size_t num_prefix_bytes;
  const uint8_t prefix_mask;
  const size_t num_suffix_bytes;
  MDB_env* env;
#ifdef HAVE_PTHREAD
  mutable pthread_mutex_t M;                  // mutext
#else
  mutable int M;                              // placeholder
#endif

  // do not allow copy or assignment
  lmdb_hash_manager_t(const lmdb_hash_manager_t&);
  lmdb_hash_manager_t& operator=(const lmdb_hash_manager_t&);

  inline uint8_t count_to_byte(size_t count) const {
    size_t x = 0;
    size_t m = count + 5;
    while (m > 19) {
      m /= 5;
      ++x;
    }
    m = (m > 4) ? m - 4 : 0;
    if (x > 15) {
      // cap exponent
      x = 15;
    }
    return (x<<4) + m;
  }

  inline size_t byte_to_count(uint8_t b) const {
    const uint64_t lookup[] = {1, 5, 25, 125, 625, 3125, 15625, 78125,
                               390625, 1953125, 9765625, 48828125, 244140625,
                               1220703125, 6103515625, 30517578125};
    const size_t x = b >> 4;
    const size_t m = b & 0x0f;
    return (m + 4) * lookup[x] - 5;
  }

  public:
  lmdb_hash_manager_t(const std::string& p_hashdb_dir,
                      const hashdb::file_mode_type_t p_file_mode,
                      const uint32_t p_hash_prefix_bits,
                      const uint32_t p_hash_suffix_bytes) :
       hashdb_dir(p_hashdb_dir),
       file_mode(p_file_mode),
       num_prefix_bytes((p_hash_prefix_bits + 7) / 8),
       prefix_mask(masks[p_hash_prefix_bits % 8]),
       num_suffix_bytes(p_hash_suffix_bytes),
       env(lmdb_helper::open_env(
                           hashdb_dir + "/lmdb_hash_store", file_mode)),
       M() {

    MUTEX_INIT(&M);

    // require valid parameters
    if (num_prefix_bytes == 0) {
      std::cerr << "invalid hash store configuration\n";
      assert(0);
    }
  }

  ~lmdb_hash_manager_t() {
    // close the lmdb_hash_store DB environment
    mdb_env_close(env);

    MUTEX_DESTROY(&M);
  }

  void insert(const std::string& binary_hash, const size_t count,
              hashdb::lmdb_changes_t& changes) {

    // require valid binary_hash
    if (binary_hash.size() == 0) {
      std::cerr << "Usage error: the binary_hash value provided to insert is empty.\n";
      return;
    }

    // ************************************************************
    // make key and data from binary_hash and count
    // ************************************************************
    uint8_t key[num_prefix_bytes];
    uint8_t data[num_suffix_bytes + 1];  // allow 1 byte for count_encoding

    size_t hash_size = binary_hash.size();

    // set key
    size_t prefix_size =
              (hash_size > num_prefix_bytes) ? num_prefix_bytes : hash_size;
    memcpy(key, binary_hash.c_str(), prefix_size);

    // zero out bits at the end as needed using prefix_mask
    key[prefix_size - 1] &= prefix_mask;

    // set data
    if (hash_size < num_suffix_bytes) {
      // short so use entire hash in aligned encoding
      memset(data, 0, num_suffix_bytes);
      memcpy(data, binary_hash.c_str(), hash_size);
    } else {
      // use right side of hash
      memcpy(data, binary_hash.c_str() + (hash_size - num_suffix_bytes), num_suffix_bytes);
    }

    // append count encoding to data
    data[num_suffix_bytes] = count_to_byte(count);

    // ************************************************************
    // insert
    // ************************************************************
    MUTEX_LOCK(&M);

    // maybe grow the DB
    lmdb_helper::maybe_grow(env);

    // get context
    hashdb::lmdb_context_t context(env, true, false);
    context.open();

    // see if key is already there
    // set context key
    context.key.mv_size = prefix_size;
    context.key.mv_data = key;

    // set cursor
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    // handle when key is new
    if (rc == MDB_NOTFOUND) {

      // set context data
      context.data.mv_size = num_suffix_bytes + 1;
      context.data.mv_data = data;
#ifdef DEBUG_LMDB_HASH_MANAGER_HPP
print_mdb_val("hash_manager insert not found key", context.key);
print_mdb_val("hash_manager insert not found data", context.data);
#endif

      // add this new key and data
      rc = mdb_put(context.txn, context.dbi,
                   &context.key, &context.data, MDB_NODUPDATA);

      // the add request must work
      if (rc != 0) {
        std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
        assert(0);
      }

      // hash inserted
      context.close();
      ++changes.hash_prefix_inserted;
      ++changes.hash_suffix_inserted;
      MUTEX_UNLOCK(&M);
      return;

    // handle when key already exists
    } else if (rc == 0) {

      // validate data size
      if (context.data.mv_size % (num_suffix_bytes +1) != 0) {
        std::cerr << "corrupted DB\n";
        assert(0);
      }

      // look for duplicate suffix
      uint8_t* const p = static_cast<uint8_t*>(context.data.mv_data);
      for (size_t i = 0; i < context.data.mv_size;
                                   i += (num_suffix_bytes + 1)) {
        bool match = true;
        for (size_t j = 0; j < num_suffix_bytes; ++j) {
          if (p[i + j] != data[j]) {
            match = false;
            break;
          }
        }

        if (match == true) {
          // suffix already there

          // maybe update count
          if (p[i + num_suffix_bytes] != data[num_suffix_bytes]) {

            // write back just to update count
            p[i + num_suffix_bytes] = data[num_suffix_bytes];
#ifdef DEBUG_LMDB_HASH_MANAGER_HPP
print_mdb_val("hash_manager insert update count key", context.key);
print_mdb_val("hash_manager insert update count data", context.data);
#endif
            rc = mdb_put(context.txn, context.dbi,
                           &context.key, &context.data, MDB_NODUPDATA);

            // write must work
            if (rc != 0) {
              std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
              assert(0);
            }

            ++changes.hash_count_changed;
          } else {
            ++changes.hash_not_changed;
          }

          // done because suffix matched
          context.close();
          MUTEX_UNLOCK(&M);
          return;
        }
      }

      // here so there was no match so append and write

      // prepare new data
      size_t new_size = context.data.mv_size + num_suffix_bytes + 1;
      uint8_t* new_data = new uint8_t[new_size]();
      memcpy(new_data, context.data.mv_data, context.data.mv_size);
      memcpy(new_data + context.data.mv_size, data, num_suffix_bytes + 1);

      // write new data
      context.data.mv_size = new_size;
      context.data.mv_data = new_data;
#ifdef DEBUG_LMDB_HASH_MANAGER_HPP
print_mdb_val("hash_manager insert append key", context.key);
print_mdb_val("hash_manager insert append data", context.data);
#endif
      rc = mdb_put(context.txn, context.dbi,
                     &context.key, &context.data, MDB_NODUPDATA);

      // release new_data resource
      delete[] new_data;

      // write must work
      if (rc != 0) {
        std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
        std::cerr << "data size: " << context.data.mv_size << "\n";
        assert(0);
      }

      // hash suffix inserted
      context.close();
      ++changes.hash_suffix_inserted;
      MUTEX_UNLOCK(&M);
      return;
    }
  }

  /**
   * Find if hash is present, return approximate count.
   */
  size_t find(const std::string& binary_hash) const {

    // require valid binary_hash
    if (binary_hash.size() == 0) {
      std::cerr << "empty key\n";
      assert(0);
    }

    // ************************************************************
    // make key and data from binary_hash
    // ************************************************************
    uint8_t key[num_prefix_bytes];
    uint8_t data[num_suffix_bytes];

    size_t hash_size = binary_hash.size();

    // set key
    size_t prefix_size =
              (hash_size > num_prefix_bytes) ? num_prefix_bytes : hash_size;
    memcpy(key, binary_hash.c_str(), prefix_size);

    // zero out bits at the end as needed using prefix_mask
    key[prefix_size - 1] &= prefix_mask;

    // set hash suffix portion of data
    if (hash_size < num_suffix_bytes) {
      // short so use entire hash in aligned encoding
      memset(data, 0, num_suffix_bytes);
      memcpy(data, binary_hash.c_str(), hash_size);
    } else {
      // use right side of hash
      memcpy(data, binary_hash.c_str() + (hash_size - num_suffix_bytes),
              num_suffix_bytes);
    }

    // ************************************************************
    // find
    // ************************************************************
    // get context
    hashdb::lmdb_context_t context(env, false, false);
    context.open();

    // see if prefix is already there
    // set context key
    context.key.mv_size = prefix_size;
    context.key.mv_data = key;

#ifdef DEBUG_LMDB_HASH_MANAGER_HPP
print_mdb_val("hash_manager find look for key", context.key);
#endif
    // set cursor
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    // handle when prefix is not there
    if (rc == MDB_NOTFOUND) {
      // the hash is not present because the prefix is not present
      context.close();
      return 0;

    } else if (rc == 0) {
      // prefix present, so look for a match
#ifdef DEBUG_LMDB_HASH_MANAGER_HPP
print_mdb_val("hash_manager find key", context.key);
print_mdb_val("hash_manager find data", context.data);
#endif

      // validate data size
      if (context.data.mv_size % (num_suffix_bytes +1) != 0) {
        std::cerr << "corrupted DB\n";
        assert(0);
      }

      // look for duplicate suffix
      uint8_t* const p = static_cast<uint8_t*>(context.data.mv_data);
      for (size_t i = 0; i < context.data.mv_size;
                                   i += (num_suffix_bytes + 1)) {
        bool match = true;
        for (size_t j = 0; j < num_suffix_bytes; ++j) {
          if (p[i + j] != data[j]) {
            match = false;
            break;
          }
        }

        if (match == true) {
          // extract approximate count then close context
          size_t approximate_count = byte_to_count(p[i + num_suffix_bytes]);
          context.close();
          return approximate_count;
        }
      }

      // here, so no suffix match
      context.close();
      return 0;

    } else {
      // invalid rc
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
      return 0; // for mingw
    }
  }

  // call this from a lock to prevent getting an unstable answer.
  size_t size() const {
    return lmdb_helper::size(env);
  }
};

} // end namespace hashdb

#endif

