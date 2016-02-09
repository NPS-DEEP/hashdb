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
 * Manage the LMDB source ID store of key=encoding(source_id),
 * data=file_binary_hash.
 * Threadsafe.
 */

#ifndef LMDB_SOURCE_ID_MANAGER_HPP
#define LMDB_SOURCE_ID_MANAGER_HPP

//#define DEBUG_LMDB_SOURCE_ID_MANAGER_HPP

#include "file_modes.h"
#include "lmdb.h"
#include "lmdb_helper.h"
#include "lmdb_context.hpp"
#include "lmdb_changes.hpp"
#include <vector>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <string>
#ifdef DEBUG_LMDB_SOURCE_ID_MANAGER_HPP
#include "print_mdb_val.hpp"
#endif

// no concurrent writes
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "mutex_lock.hpp"

namespace hashdb {

class lmdb_source_id_manager_t {

  private:
  const std::string hashdb_dir;
  const hashdb::file_mode_type_t file_mode;
  MDB_env* env;
#ifdef HAVE_PTHREAD
  mutable pthread_mutex_t M;                  // mutext
#else
  mutable int M;                              // placeholder
#endif

  // do not allow copy or assignment
  lmdb_source_id_manager_t(const lmdb_source_id_manager_t&);
  lmdb_source_id_manager_t& operator=(const lmdb_source_id_manager_t&);

  public:
  lmdb_source_id_manager_t(const std::string& p_hashdb_dir,
                           const hashdb::file_mode_type_t p_file_mode) :
       hashdb_dir(p_hashdb_dir),
       file_mode(p_file_mode),
       env(lmdb_helper::open_env(hashdb_dir + "/lmdb_source_id_store",
                                                            file_mode)),
       M() {
    MUTEX_INIT(&M);
  }

  ~lmdb_source_id_manager_t() {
    // close the lmdb_hash_store DB environment
    mdb_env_close(env);

    MUTEX_DESTROY(&M);
  }

  /**
   * Insert key=file_binary_hash, value=source_id.  Return source_id.
   * Return false if already there.
   */
  std::pair<bool, uint64_t> insert(const std::string& file_binary_hash,
                                   hashdb::lmdb_changes_t& changes) {

    // require valid file_binary_hash
    if (file_binary_hash.size() == 0) {
      std::cerr << "empty key\n";
      assert(0);
    }

    MUTEX_LOCK(&M);

    // maybe grow the DB
    lmdb_helper::maybe_grow(env);

    // get context
    hashdb::lmdb_context_t context(env, true, false); // writable, no duplicates
    context.open();

    // set key
    context.key.mv_size = file_binary_hash.size();
    context.key.mv_data =
            static_cast<void*>(const_cast<char*>(file_binary_hash.c_str()));

    // see if source_id exists yet
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    uint64_t source_id;
    if (rc == 0) {
#ifdef DEBUG_LMDB_SOURCE_ID_MANAGER_HPP
print_mdb_val("source_id_manager insert has key", context.key);
print_mdb_val("source_id_manager insert has data", context.data);
#endif
      // source ID found
      const uint8_t* p = static_cast<uint8_t*>(context.data.mv_data);
      p = lmdb_helper::decode_uint64_t(p, source_id);

      // read must align to data record
      if (p != static_cast<uint8_t*>(context.data.mv_data) +
                                                context.data.mv_size) {
        assert(0);
      }

      ++changes.source_id_already_present;
      context.close();
      MUTEX_UNLOCK(&M);
      return std::pair<bool, uint64_t>(false, source_id);

    } else if (rc == MDB_NOTFOUND) {
      // generate new source ID as DB size + 1
      source_id = size() + 1;
      uint8_t data[10];
      uint8_t* p = data;
      p = lmdb_helper::encode_uint64_t(source_id, p);
      context.data.mv_size = p - data;
      context.data.mv_data = data;

#ifdef DEBUG_LMDB_SOURCE_ID_MANAGER_HPP
print_mdb_val("source_id_manager insert new key", context.key);
print_mdb_val("source_id_manager insert new data", context.data);
#endif
      rc = mdb_put(context.txn, context.dbi,
                   &context.key, &context.data, MDB_NODUPDATA);

      // write must work
      if (rc != 0) {
        std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
        assert(0);
      }

      // source ID created
      ++changes.source_id_inserted;
      context.close();
      MUTEX_UNLOCK(&M);
      return std::pair<bool, uint64_t>(true, source_id);

    } else {
      // invalid rc
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
  }

  /**
   * Find source ID else false.
   */
  std::pair<bool, uint64_t> find(const std::string& file_binary_hash) const {

    // require valid file_binary_hash
    if (file_binary_hash.size() == 0) {
      std::cerr << "empty key\n";
      assert(0);
    }

    // get context
    hashdb::lmdb_context_t context(env, false, false);
    context.open();

    // set key
    context.key.mv_size = file_binary_hash.size();
    context.key.mv_data =
            static_cast<void*>(const_cast<char*>(file_binary_hash.c_str()));

    // set the cursor to this key
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    if (rc == 0) {
      // has source ID
#ifdef DEBUG_LMDB_SOURCE_ID_MANAGER_HPP
print_mdb_val("source_id_manager find key", context.key);
print_mdb_val("source_id_manager find data", context.data);
#endif
      const uint8_t* p = static_cast<uint8_t*>(context.data.mv_data);
      uint64_t source_id;
      p = lmdb_helper::decode_uint64_t(p, source_id);

      // read must align to data record
      if (p != static_cast<uint8_t*>(context.data.mv_data) +
                                                context.data.mv_size) {
        assert(0);
      }

      context.close();
      return std::pair<bool, uint64_t>(true, source_id);

    } else if (rc == MDB_NOTFOUND) {
#ifdef DEBUG_LMDB_SOURCE_ID_MANAGER_HPP
print_mdb_val("source_id_manager find not found key", context.key);
#endif
      context.close();
      return std::pair<bool, uint64_t>(false, 0);

    } else {
      // invalid rc
      std::cerr << "LMDB find error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
  }

  // call this from a lock to prevent getting an unstable answer.
  size_t size() const {
    return lmdb_helper::size(env);
  }
};

} // end namespace hashdb

#endif

