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
 * Provides source metadata lookup using LMDB.
 *
 * Locks are required around contexts that can write to preserve
 * integrity, in particular to allow grow.
 *
 * It is always a program error to supply an invalid source lookup index.
 */

#ifndef LMDB_SOURCE_METADATA_STORE_HPP
#define LMDB_SOURCE_METADATA_STORE_HPP
#include "file_modes.h"
#include <string>
#include "lmdb.h"

// no concurrent writes
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "mutex_lock.hpp"

// enable debug
// #define DEBUG 1

class lmdb_source_metadata_store_t {

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
  lmdb_source_metadata_store_t(const lmdb_source_metadata_store_t&);
  lmdb_source_metadata_store_t& operator=(const lmdb_source_metadata_store_t&);

  public:
  lmdb_source_metadata_store_t (const std::string p_hashdb_dir,
                     file_mode_type_t p_file_mode_type) :

       hashdb_dir(p_hashdb_dir),
       file_mode(p_file_mode_type),
       env(),
       M() {

    MUTEX_INIT(&M);

    // the DB stage directory
    const std::string store_dir = hashdb_dir + "/lmdb_source_metadata_store";

    // open the DB environment
    env = open_env(store_dir, file_mode);
  }

  ~lmdb_source_metadata_store_t() {

    // close the MDB environment
    mdb_env_close(env);
  }

  // add repository name and filename
  bool add_repository_name_filename(uint64_t source_lookup_index,
              const std::string& repository_name,
              const std::string& filename) {

    MUTEX_LOCK(&M);

    // maybe grow the DB
    lmdb_helper::maybe_grow(env);

    // get context
    lmdb_context_t context(env, true, false);
    context.open();

    // set source lookup index pointer
    point_to_uint64_t(source_lookup_index, context.key);

    // read any existing metadata
    int rc = mdb_cursor_get(context.cursor, &context.key. &context.data,
                            MDB_SET_KEY);

    if (rc == 0) {
      lmdb_source_metadata_t existing_metadata(context.data);
      bool added_more = existing_metadata.add_repository_name_filename(
                                         repository_name, filename);
      if (added_more) {
        // replace the record with the new one
        

      // see if 

    // emplace
    int rc = mdb_put(resources->txn, resources->dbi,
                     &resources->key, &resources->data, MDB_NODUPDATA);
    if (rc != 0) {
      std::cerr << "LMDB emplace error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    context.close();
    MUTEX_UNLOCK(&M);
  }

  // erase hash, encoding pair
  bool erase(const hash_t& hash, uint64_t encoding) {

    MUTEX_LOCK(&M);

    // get context
    lmdb_context_t context(env, true, true);
    context.open();

    // set hash pointer
    point_to_hash(hash, context.key);

    // set encoding pointer
    point_to_uint64_t(encoding, context.value);

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
  size_t erase(const hash_t& hash) {

    MUTEX_LOCK(&M);

    // get context
    lmdb_context_t context(env, true, true);
    context.open();

    // set hash pointer
    point_to_hash(hash, context.key);

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

      // delete if there
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
 
  // find specific hash, value pair
  bool find(const hash_t& hash, uint64_t value) const {

    // get context
    lmdb_context_t context(env, false, true);
    context.open();

    // set hash pointer
    point_to_hash(hash, context.key);

    // set encoding pointer
    point_to_uint64_t(encoding, context.value);

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
      assert(0);
    }

    // close context
    context.close();

    return has_pair;
  }

  // count of entries with this hash value
  size_t count(const hash_t& hash) const {

    // get context
    lmdb_context_t context(env, false, true);
    context.open();

    // set hash pointer
    point_to_hash(hash, context.key);

    // set encoding pointer
    uint64_t encoding;
    point_to_uint64_t(encoding, context.value);

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
 
  hash_encoding_status_t find_first() const {

    // get context
    lmdb_context_t context(env, false, true);
    context.open();

    hash_encoding_status_t result;
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_FIRST);
    bool has_first;
    if (rc == 0) {
      has_first = true;
    } else if (rc == MDB_NOTFOUND) {
      has_first = false;
      at_end = true;
    } else {
      // program error
      has_first = false;
      std::cerr << "rc: " << rc << ", begin: " << is_begin << "\n";
      assert(0);
    }

    hash_encoding_status_t result(get_hash_t(context.key),
                                  get_uint64_t(context.data, has_first);
    // close context
    context.close();

    return result;
  }

  /**
   * Find entry just after this one.
   */
  hash_encoding_status_t find_next(const hash_t& hash, uint64_t value) const {

    // get context
    lmdb_context_t context(env, false, true);
    context.open();

    // set hash pointer
    point_to_hash(hash, context.key);

    // set encoding pointer
    point_to_uint64_t(encoding, context.value);

    // set the cursor to this key,data pair which must exist
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_GET_BOTH);
    if (rc != 0) {
      // invalid usage
      assert(0);
    }

    // set cursor to next key, data pair
    rc = mdb_cursor_get(resources->cursor,
                        &resources->key, &resources->data, MDB_NEXT);

    bool has_next;
    if (rc == 0) {
      has_next = true;
    } else if (rc == MDB_NOTFOUND) {
      has_next = false;
      at_end = true;
    } else {
      // program error
      has_next = false;
      std::cerr << "rc: " << rc << ", begin: " << is_begin << "\n";
      assert(0);
    }

    hash_encoding_status_t result(get_hash_t(context.key),
                                  get_uint64_t(context.data, has_next);
    // close context
    context.close();

    return result;
  }

  // size
  size_t size() const {
    return lmdb_helper::size(env);
  }
};

#endif

