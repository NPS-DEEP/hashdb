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
 * Provides hash to encoding lookup using LMDB
 * where encoding contains source_lookup_index and file_offset.
 *
 * Locks are required around contexts that can write to preserve
 * integrity, in particular to allow grow.
 */

#ifndef LMDB_HASH_STORE_HPP
#define LMDB_HASH_STORE_HPP
#include "file_modes.h"
#include "lmdb.h"
#include "lmdb_helper.h"
#include "lmdb_context.hpp"
#include "lmdb_hash_it_data.hpp"
#include <string>

// no concurrent writes
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "mutex_lock.hpp"

// enable debug
// #define DEBUG 1

class lmdb_hash_store_t {

  private:
  const std::string hashdb_dir;
  const file_mode_type_t file_mode;
  MDB_env* env;

#ifdef HAVE_PTHREAD
  mutable pthread_mutex_t M;                  // mutext
#else
  mutable int M;                              // placeholder
#endif

  // disallow these
  lmdb_hash_store_t(const lmdb_hash_store_t&);
  lmdb_hash_store_t& operator=(const lmdb_hash_store_t&);

  public:
  lmdb_hash_store_t (const std::string p_hashdb_dir,
                     file_mode_type_t p_file_mode_type) :

       hashdb_dir(p_hashdb_dir),
       file_mode(p_file_mode_type),
       env(),
       M() {

    MUTEX_INIT(&M);

    // the DB stage directory
    const std::string store_dir = hashdb_dir + "/lmdb_hash_store";

    // open the DB environment
    env = lmdb_helper::open_env(store_dir, file_mode);
  }

  ~lmdb_hash_store_t() {

    // close the MDB environment
    mdb_env_close(env);
  }

  // insert or fail
  void insert(const std::string &binary_hash,
              uint64_t source_lookup_index,
              uint64_t file_offset) {

    MUTEX_LOCK(&M);

    // maybe grow the DB
    lmdb_helper::maybe_grow(env);

    // get context
    lmdb_context_t context(env, true, true);
    context.open();

    // set key
    lmdb_helper::point_to_string(binary_hash, context.key);

    // set data
    std::string encoding = lmdb_helper::uint64_pair_to_encoding(
                                          source_lookup_index, file_offset);
    lmdb_helper::point_to_string(encoding, context.data);

    // insert
    int rc = mdb_put(context.txn, context.dbi,
                     &context.key, &context.data, MDB_NODUPDATA);
    if (rc != 0) {
      std::cerr << "LMDB insert error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    context.close();
    MUTEX_UNLOCK(&M);
  }

  // erase hash, encoding pair
  bool erase(const std::string& binary_hash,
             uint64_t source_lookup_index, uint64_t file_offset) {

    MUTEX_LOCK(&M);

    // get context
    lmdb_context_t context(env, true, true);
    context.open();

    // set key
    lmdb_helper::point_to_string(binary_hash, context.key);

    // set data
    std::string encoding = lmdb_helper::uint64_pair_to_encoding(
                                          source_lookup_index, file_offset);
    lmdb_helper::point_to_string(encoding, context.data);

    // erase
    int rc = mdb_del(context.txn, context.dbi, &context.key, &context.data);
    bool status;
    if (rc == 0) {
      status = true;
    } else if (rc == MDB_NOTFOUND) {
      status = false;
    } else {
      std::cerr << "LMDB erase error: " << mdb_strerror(rc) << "\n";
      status = false;
      assert(0);
    }

    // close context
    context.close();
    MUTEX_UNLOCK(&M);

    return status;
  }

  // erase hash, return count erased
  size_t erase(const std::string& binary_hash) {

    MUTEX_LOCK(&M);

    // get context
    lmdb_context_t context(env, true, true);
    context.open();

    // set key
    lmdb_helper::point_to_string(binary_hash, context.key);

    // set the cursor to this key
    int rc = mdb_cursor_get(context.cursor, &context.key,
                            NULL, MDB_SET_RANGE);

    size_t key_count;
    if (rc == 0) {

      // DB has key so look up the key count
      rc = mdb_cursor_count(context.cursor, &key_count);
      if (rc != 0) {
        std::cerr << "LMDB erase count error: " << mdb_strerror(rc) << "\n";
        assert(0);
      }

    } else if (rc == MDB_NOTFOUND) {
      // DB does not have key
      key_count = 0;

    } else {
      std::cerr << "LMDB erase cursor get error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    if (key_count > 0) {

      // delete
      rc = mdb_del(context.txn, context.dbi, 
                   &context.key, NULL);
      if (rc != 0) {
        std::cerr << "LMDB erase delete error: " << mdb_strerror(rc) << "\n";
        assert(0);
      }
    }

    // close context
    context.close();
    MUTEX_UNLOCK(&M);

    return key_count;
  }
 
  // find specific hash, encoding pair
  bool find(const std::string& binary_hash,
            uint64_t source_lookup_index, uint64_t file_offset) const {

    // get context
    lmdb_context_t context(env, false, true);
    context.open();

    // set key
    lmdb_helper::point_to_string(binary_hash, context.key);

    // set data
    std::string encoding = lmdb_helper::uint64_pair_to_encoding(
                                          source_lookup_index, file_offset);
    lmdb_helper::point_to_string(encoding, context.data);

    // set the cursor to this key,data pair
    int rc = mdb_cursor_get(context.cursor,
                            &context.key, &context.data,
                            MDB_GET_BOTH);
    bool has_pair;
    if (rc == 0) {
      has_pair = true;
    } else if (rc == MDB_NOTFOUND) {
      has_pair = false;
    } else {
      // program error
      has_pair = false; // satisfy mingw32-g++ compiler
      std::cerr << "LMDB find error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    // close context
    context.close();

    return has_pair;
  }

  // count of entries with this hash value
  size_t find_count(const std::string& binary_hash) const {

    // get context
    lmdb_context_t context(env, false, true);
    context.open();

    // set key
    lmdb_helper::point_to_string(binary_hash, context.key);

    // set the cursor to this key
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);
    size_t key_count = 0;
    if (rc == 0) {
      rc = mdb_cursor_count(context.cursor, &key_count);
      if (rc != 0) {
        std::cerr << "LMDB count error: " << mdb_strerror(rc) << "\n";
        assert(0);
      }
    } else if (rc == MDB_NOTFOUND) {
      // fine, key count is zero
    } else {
      std::cerr << "LMDB get error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    // close context
    context.close();

    return key_count;
  }
 
  // first entry with this hash value
  lmdb_hash_it_data_t find_first(const std::string& binary_hash) const {

    // get context
    lmdb_context_t context(env, false, true);
    context.open();

    // set key
    lmdb_helper::point_to_string(binary_hash, context.key);

    // set the cursor to this key
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);
    bool is_found;
    if (rc == 0) {
      is_found = true;
    } else if (rc == MDB_NOTFOUND) {
      is_found = false;
    } else {
      std::cerr << "LMDB get error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    // prepare hash_it_data
    std::pair<uint64_t, uint64_t> uint64_pair =
                          lmdb_helper::encoding_to_uint64_pair(context.data);

    // close context
    context.close();

    return lmdb_hash_it_data_t(binary_hash,
                               uint64_pair.first, uint64_pair.second,
                               is_found);
  }
 
  lmdb_hash_it_data_t find_begin() const {

    // get context
    lmdb_context_t context(env, false, true);
    context.open();

    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_FIRST);
    bool is_begin;
    if (rc == 0) {
      is_begin = true;
    } else if (rc == MDB_NOTFOUND) {
      is_begin = false;
    } else {
      // program error
      is_begin = false;
      std::cerr << "LMDB find_begin error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    // prepare hash_it_data
    std::string binary_hash = lmdb_helper::get_string(context.key);
    std::pair<uint64_t, uint64_t> uint64_pair =
                          lmdb_helper::encoding_to_uint64_pair(context.data);

    // close context
    context.close();

    return lmdb_hash_it_data_t(binary_hash,
                               uint64_pair.first, uint64_pair.second,
                               is_begin);
  }

  /**
   * Find entry just after this one.
   */
  lmdb_hash_it_data_t find_next(const lmdb_hash_it_data_t& hash_it_data) const {

    // get context
    lmdb_context_t context(env, false, true);
    context.open();

    // set key
    lmdb_helper::point_to_string(hash_it_data.binary_hash, context.key);

    // set data
    std::string encoding = lmdb_helper::uint64_pair_to_encoding(
                                         hash_it_data.source_lookup_index,
                                         hash_it_data.file_offset);
    lmdb_helper::point_to_string(encoding, context.data);

    // set the cursor to this key,data pair which must exist
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_GET_BOTH);
    if (rc != 0) {
      // invalid usage
      std::cerr << "LMDB find_next error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    // set cursor to next key, data pair
    rc = mdb_cursor_get(context.cursor,
                        &context.key, &context.data, MDB_NEXT);

    bool has_next;
    if (rc == 0) {
      has_next = true;
    } else if (rc == MDB_NOTFOUND) {
      has_next = false;
    } else {
      // program error
      has_next = false;
      std::cerr << "LMDB has_next error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    // prepare hash_it_data
    std::string binary_hash = lmdb_helper::get_string(context.key);
    std::pair<uint64_t, uint64_t> uint64_pair =
                          lmdb_helper::encoding_to_uint64_pair(context.data);

    // close context
    context.close();

    return lmdb_hash_it_data_t(binary_hash,
                               uint64_pair.first, uint64_pair.second,
                               has_next);
  }

  // size
  size_t size() const {
    return lmdb_helper::size(env);
  }
};

#endif

