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
 * Manage the LMDB source name store.  Threadsafe.
 */

#ifndef LMDB_SOURCE_NAME_MANAGER_HPP
#define LMDB_SOURCE_NAME_MANAGER_HPP

#define DEBUG_LMDB_SOURCE_NAME_MANAGER_HPP

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
#ifdef DEBUG_LMDB_SOURCE_NAME_MANAGER_HPP
#include "print_lmdb_val.hpp"
#endif

// no concurrent writes
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "mutex_lock.hpp"

namespace hashdb {

class lmdb_source_name_manager_t {

  private:
  typedef std::pair<std::string, std::string> source_name_t;
  typedef std::set<source_name_t> source_names_t;

  const std::string hashdb_dir;
  const hashdb::file_mode_type_t file_mode;
  MDB_env* env;
#ifdef HAVE_PTHREAD
  mutable pthread_mutex_t M;                  // mutext
#else
  mutable int M;                              // placeholder
#endif

  // do not allow copy or assignment
  lmdb_source_name_manager_t(const lmdb_source_name_manager_t&);
  lmdb_source_name_manager_t& operator=(const lmdb_source_name_manager_t&);

  public:
  lmdb_source_name_manager_t(const std::string& p_hashdb_dir,
                      const hashdb::file_mode_type_t p_file_mode) :
       hashdb_dir(p_hashdb_dir),
       file_mode(p_file_mode),
       env(lmdb_helper::open_env(hashdb_dir + "/lmdb_source_name_store",
                                                                file_mode)),
       M() {

    MUTEX_INIT(&M);
  }

  ~lmdb_source_name_manager_t() {
    // close the lmdb_hash_store DB environment
    mdb_env_close(env);

    MUTEX_DESTROY(&M);
  }

  /**
   * Insert repository_name, filename pair unless pair is already there.
   */
  void insert(const uint64_t source_id,
              const std::string& repository_name,
              const std::string& filename,
              hashdb::lmdb_changes_t& changes) {

    MUTEX_LOCK(&M);

    // maybe grow the DB
    lmdb_helper::maybe_grow(env);

    // get context
    hashdb::lmdb_context_t context(env, true, true);
    context.open();

    // set key=source_id
    uint8_t key[10];
    uint8_t* key_p = key;
    key_p = lmdb_helper::encode_uint64_t(source_id, key_p);
    context.key.mv_size = key_p - key;
    context.key.mv_data = key;

    // set data=repository_name, filename pair
    size_t repository_name_size = repository_name.size();
    size_t filename_size = filename.size();
    uint8_t data[repository_name_size + 10 + filename_size + 10];
    uint8_t* p = data;
    p = lmdb_helper::encode_uint64_t(repository_name_size, p);
    std::memcpy(p, repository_name.c_str(), repository_name_size);
    p += repository_name_size;
    p = lmdb_helper::encode_uint64_t(filename_size, p);
    std::memcpy(p, filename.c_str(), filename_size);
    p += filename_size;
    context.data.mv_size = p - data;
    context.data.mv_data = data;

    // write the new source name
#ifdef DEBUG_LMDB_SOURCE_NAME_MANAGER_HPP
print_mdb_val("source_name_manager insert new key", context.key);
print_mdb_val("source_name_manager insert new data", context.data);
#endif
    int rc = mdb_put(context.txn, context.dbi,
                     &context.key, &context.data, MDB_NODUPDATA);

    if (rc == 0) {
      // the new name pair went in
      ++changes.source_name_inserted;
      context.close();
      MUTEX_UNLOCK(&M);
      return;

    } else if (rc == MDB_KEYEXIST) {
      // the name pair was already there
      ++changes.source_name_already_present;
      context.close();
      MUTEX_UNLOCK(&M);
      return;

    } else {
      // invalid rc
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
  }

  /**
   * Find source names, false on no source ID.
   */
  bool find(const uint64_t source_id,
            source_names_t& names) const {

    // get context
    hashdb::lmdb_context_t context(env, false, true);
    context.open();

    // set key
    uint8_t key_start[10];
    uint8_t* key_p = key_start;
    key_p = lmdb_helper::encode_uint64_t(source_id, key_p);
    const size_t key_size = key_p - key_start;
    context.key.mv_size = key_size;
    context.key.mv_data = key_start;
    context.data.mv_size = 0;
    context.data.mv_data = NULL;

    // set the cursor to this key
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);
#ifdef DEBUG_LMDB_SOURCE_NAME_MANAGER_HPP
print_mdb_val("source_name_manager find start at key", context.key);
#endif

    // note if source ID was found
    bool source_id_found = (rc == 0);

    // read name pairs while data available and key matches
    names.clear();
    while (rc == 0 &&
           context.key.mv_size == key_size &&
           memcmp(context.key.mv_data, key_start, key_size) == 0) {

#ifdef DEBUG_LMDB_SOURCE_NAME_MANAGER_HPP
print_mdb_val("source_name_manager find key", context.key);
print_mdb_val("source_name_manager find data", context.data);
#endif
      // read repository_name, filename pair into names
      const uint8_t* p = static_cast<uint8_t*>(context.data.mv_data);
      const uint8_t* const p_stop = p + context.data.mv_size;
      uint64_t repository_name_size;
      const uint8_t* const rn_p = lmdb_helper::decode_uint64_t(
                                           p, repository_name_size);
      p = rn_p + repository_name_size;
      uint64_t filename_size;
      const uint8_t* const fn_p = lmdb_helper::decode_uint64_t(
                                         p, filename_size);
      p = fn_p + filename_size;
      names.insert(source_name_t(
              std::string(reinterpret_cast<const char*>(rn_p),
                          repository_name_size),
              std::string(reinterpret_cast<const char*>(fn_p),
                          filename_size)));

      // validate that the decoding was properly consumed
      if (p != p_stop) {
        assert(0);
      }

      // next
      rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                          MDB_NEXT);
    }

    if (rc == 0 || rc == MDB_NOTFOUND) {
      // good, LMDB worked correctly
      context.close();
      return source_id_found;

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

} // end namespace hashdb

#endif

