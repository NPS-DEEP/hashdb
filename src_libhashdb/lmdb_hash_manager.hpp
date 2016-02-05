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
 * Manage the LMDB hash store.  Threadsafe.
 */

#ifndef LMDB_HASH_MANAGER_HPP
#define LMDB_HASH_MANAGER_HPP
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
//#ifdef DEBUG
#include "to_hex.hpp"
//#endif

// no concurrent writes
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "mutex_lock.hpp"

//#ifdef DEBUG
static void print_mdb_val(const std::string& name, const MDB_val& val) {
  std::cerr << name << ": "
            << hashdb::to_hex(lmdb_helper::get_string(val)) << "\n";
}
//#endif

static uint8_t masks[8] = {0xff,0x80,0xc0,0xe0,0xf0,0xf8,0xfc,0xfe};

class lmdb_hash_manager_t {

  private:
  const std::string hashdb_dir;
  const file_mode_type_t file_mode;
  const int num_prefix_bytes;
  const uint8_t prefix_mask;
  const int num_suffix_bytes;
  MDB_env* env;
#ifdef HAVE_PTHREAD
  mutable pthread_mutex_t M;                  // mutext
#else
  mutable int M;                              // placeholder
#endif

  // do not allow copy or assignment
  lmdb_hash_manager_t(const lmdb_hash_manager_t&);
  lmdb_hash_manager_t& operator=(const lmdb_hash_manager_t&);

  public:
  lmdb_hash_manager_t(const std::string& p_hashdb_dir,
                      const file_mode_type_t p_file_mode,
                      const uint32_t p_hash_prefix_bits,
                      const uint32_t p_hash_suffix_bytes) :
       hashdb_dir(p_hashdb_dir),
       file_mode(p_file_mode),
       num_prefix_bytes((p_hash_prefix_bits + 7) / 8),
       prefix_mask(masks[p_hash_prefix_bits % 8]),
       num_suffix_bytes(p_hash_suffix_bytes),
       env(lmdb_helper::open_env(hashdb_dir + "/lmdb_hash_store", file_mode)),
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

  void insert(const std::string& binary_hash, const size_t source_count,
              lmdb_changes_t& changes) {

    // require valid binary_hash
    if (binary_hash.size() == 0) {
      std::cerr << "empty key\n";
      assert(0);
    }

    // ************************************************************
    // make key and data from binary_hash and source_count
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
      memcpy(data, binary_hash.c_str() + (hash_size - 1 - num_suffix_bytes), num_suffix_bytes);
    }

    // calculate exponent for encoding
    size_t temp = source_count + 6;
    if (temp > 0xffffffff) {
      // clip to fit within 32-bit size_t
      temp = 0xffffffff;
    }
    uint8_t x = 0;
    while (temp > 15) {
      x++;
      temp /= 5;
    }

    // calculate mantissa for encoding
//    if (temp < 0) {
//      // invalid data
//      std::cerr << "corrupted DB\n";
//      assert(0);
//    }
    uint8_t m = temp;

    // append count encoding to data
    data[num_suffix_bytes] = (x<<4) + m;

    // ************************************************************
    // insert
    // ************************************************************
    MUTEX_LOCK(&M);

    // maybe grow the DB
    lmdb_helper::maybe_grow(env);

    // get context
    lmdb_context_t context(env, true, false);
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
print_mdb_val("hash_manager insert not found key", context.key);
print_mdb_val("hash_manager insert not found data", context.data);

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
      ++changes.hash_inserted;
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
          ++changes.hash_already_present;

          // maybe update count
          if (p[i + num_suffix_bytes] == data[num_suffix_bytes]) {

            // write back just to update count
            p[i + num_suffix_bytes] = data[num_suffix_bytes];
print_mdb_val("hash_manager insert update count key", context.key);
print_mdb_val("hash_manager insert update count data", context.data);
            rc = mdb_put(context.txn, context.dbi,
                           &context.key, &context.data, MDB_NODUPDATA);

            // write must work
            if (rc != 0) {
              std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
              assert(0);
            }
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
      uint8_t* new_data = new uint8_t[new_size];
      memcpy(new_data, context.data.mv_data, context.data.mv_size);
      memcpy(new_data + context.data.mv_size, data, num_suffix_bytes + 1);

      // write new data
      context.data.mv_size = new_size;
      context.data.mv_data = new_data;
print_mdb_val("hash_manager insert append key", context.key);
print_mdb_val("hash_manager insert append data", context.data);
      rc = mdb_put(context.txn, context.dbi,
                     &context.key, &context.data, MDB_NODUPDATA);

      // release new_data resource
      delete new_data;

      // write must work
      if (rc != 0) {
        std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
        assert(0);
      }

      // hash inserted
      context.close();
      ++changes.hash_inserted;
      MUTEX_UNLOCK(&M);
      return;
    }
  }

  /**
   * Find if hash is present, return approximate source count.
   */
  size_t find(const std::string& binary_hash) const {

std::cerr << "find "
          << hashdb::to_hex(binary_hash) << "\n";
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
      memcpy(data, binary_hash.c_str() + (hash_size - 1 - num_suffix_bytes),
              num_suffix_bytes);
    }

    // ************************************************************
    // find
    // ************************************************************
    // get context
    lmdb_context_t context(env, false, false);
    context.open();

    // see if prefix is already there
    // set context key
    context.key.mv_size = prefix_size;
    context.key.mv_data = key;

print_mdb_val("hash_manager find look for key", context.key);
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
print_mdb_val("hash_manager find key", context.key);
print_mdb_val("hash_manager find data", context.data);

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
          // reconstruct approximate source_count
          const size_t x = (p[i + num_suffix_bytes]) >> 4;
          const size_t m = (p[i + num_suffix_bytes]) & 0x0f;
          size_t source_count = 1;
          for (size_t k = 0; k < x; k++) {
            source_count *= 5;
          }
          // zz
          source_count = (m + 4) * source_count - 10;
          context.close();
          return source_count;
        }
      }

      // here, so no suffix match
      context.close();
      return 0;

    } else {
      // invalid rc
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
  }

  // call this from a lock to prevent getting an unstable answer.
  size_t size() const {
    return lmdb_helper::size(env);
  }
};

#endif

