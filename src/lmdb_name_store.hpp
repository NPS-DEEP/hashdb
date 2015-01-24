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
 * Provides repository name, filename to source lookup index lookup
 * using LMDB.
 *
 * Locks are required around contexts that can write to preserve
 * integrity, in particular to allow grow and to preserve accurate size.
 */

#ifndef LMDB_NAME_STORE_HPP
#define LMDB_NAME_STORE_HPP
#include <string>

// no concurrent writes
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "mutex_lock.hpp"

// enable debug
// #define DEBUG 1

class lmdb_name_store_t {

  private:
  const std::string hashdb_dir;
  const file_mode_type_t file_mode_type;
  MDB_env* env;

#ifdef HAVE_PTHREAD
  mutable pthread_mutex_t M;                  // mutext
#else
  mutable int M;                              // placeholder
#endif

  // disallow these
  lmdb_name_store_t(const lmdb_name_store_t&);
  lmdb_name_store_t& operator=(const lmdb_name_store_t&);

  public:
  lmdb_name_store_t (const std::string p_hashdb_dir,
                     file_mode_type_t p_file_mode_type) :

       hashdb_dir(p_hashdb_dir),
       file_mode_type(p_file_mode_type),
       env(0),
       M() {

    MUTEX_INIT(&M);

    // create the DB environment
    MDB_env* new_env;
    int rc = mdb_env_create(&new_env);
    if (rc != 0) {
      // bad failure
      assert(0);
    }

    // the DB stage directory
    const std::string store_dir = hashdb_dir + "/lmdb_name_store";

    // open the DB environment
    env = open_env(store_dir, file_mode);
  }

  ~lmdb_name_store_t() {

    // close the MDB environment
    mdb_env_close(env);
  }

  /**
   * Insert and return true and the next source lookup index
   * else return false and existing source_lookup_index if already there.
   */
  std::pair<bool, uint64_t> insert(const std::string& repository_name,
                                   const std::string& filename) {

    MUTEX_LOCK(&M);

    // maybe grow the DB
    lmdb_helper::maybe_grow(env);

    // get context
    lmdb_context_t context(env, true, false);
    context.open();

    // build the cstr to be inserted
    size_t l1 = repository_name.length();
    size_t l2 = filename.length();
    char* cstr = new char[l1 + 1 + l2]; // space for strings separated by \0
    std::strcpy(cstr, repository_name.c_str()); // copy first plus \0
    std::memcpy(cstr+l1+1, filename.c_str(), l2);

    // set up key
    context.key.mv_size = l1 + 1 + l2;
    context.key.mv_data = cstr;

    // see if key is already there
    bool is_new = false;
    uint64_t source_lookup_index;
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);
    if (rc == 0) {
      // great, get the existing index
      source_lookup_index = lmdb_helper::get_uint64_t(context.data);
    } else if (rc == MDB_NOTFOUND) {
      // fine, add new entry
      is_new = true;
      source_lookup_index = size() + 1;
      context.data.mv_size = sizeof(uint64_t);
      context.data.mv_data = static_cast<const void*>(&source_lookup_index);
      rc = mdb_put(context.txn, context.dbi, context.key, context.data);
      if (rc != 0) {
        std::cerr << "name insert failure: " << mdb_strerror(rc) << "\n";
        assert(0);
      }
    } else {
      std::cerr << "name lookup failure: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    delete cstr;
    context.close();
    MUTEX_UNLOCK(&M);

    return std::pair<bool, uint64_t>(is_new, source_lookup_index);
  }

  /**
   * Find index, return true and index else false and zero.
   */
  std::pair<bool, uint64_t> find(const std::string& repository_name,
                                 const std::string& filename) {

    // get context
    lmdb_context_t context(env, false, false);
    context.open();

    // build the cstr key
    size_t l1 = repository_name.length();
    size_t l2 = filename.length();
    char* cstr = new char[l1 + 1 + l2]; // space for strings separated by \0
    std::strcpy(cstr, repository_name.c_str()); // copy first plus \0
    std::memcpy(cstr+l1+1, filename.c_str(), l2);

    // set up key
    context.key.mv_size = l1 + 1 + l2;
    context.key.mv_data = cstr;

    // see if key is there
    bool is_there = false;
    uint64_t source_lookup_index = 0;
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);
    if (rc == 0) {
      // great, get the existing index
      source_lookup_index = lmdb_helper::get_uint64_t(context.data);
    } else if (rc == MDB_NOTFOUND) {
      // no action
    } else {
      std::cerr << "name find failure: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    context.close();
    return std::pair<bool, uint64_t>(is_there, source_lookup_index);
  }

  // size
  size_t size() const {
    return lmdb_helper::size(env);
  }
};

#endif

