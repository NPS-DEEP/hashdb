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
 * Manage the LMDB source data store of: key=source_id,
 * data=(file_binary_hash, filesize, file_type, nonprobative_count)
 * Threadsafe.
 */

#ifndef LMDB_SOURCE_DATA_MANAGER_HPP
#define LMDB_SOURCE_DATA_MANAGER_HPP

//#define DEBUG_LMDB_SOURCE_DATA_MANAGER_HPP

#include "file_modes.h"
#include "lmdb.h"
#include "lmdb_helper.h"
#include "lmdb_context.hpp"
#include "lmdb_changes.hpp"
#include "hashdb.hpp"
#include <vector>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <string>
#include <cassert>
#ifdef DEBUG_LMDB_SOURCE_DATA_MANAGER_HPP
#include "lmdb_print_val.hpp"
#endif

// no concurrent writes
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "mutex_lock.hpp"

namespace hashdb {

class lmdb_source_data_manager_t {

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
  lmdb_source_data_manager_t(const lmdb_source_data_manager_t&);
  lmdb_source_data_manager_t& operator=(const lmdb_source_data_manager_t&);

  public:
  lmdb_source_data_manager_t(const std::string& p_hashdb_dir,
                            const hashdb::file_mode_type_t p_file_mode) :
       hashdb_dir(p_hashdb_dir),
       file_mode(p_file_mode),
       env(lmdb_helper::open_env(hashdb_dir + "/lmdb_source_data_store",
                                                            file_mode)),
       M() {
    MUTEX_INIT(&M);
  }

  ~lmdb_source_data_manager_t() {
    // close the lmdb_hash_store DB environment
    mdb_env_close(env);

    MUTEX_DESTROY(&M);
  }

  /**
   * Insert unless there and same.
   */
  void insert(const uint64_t source_id,
              const std::string& file_binary_hash,
              const uint64_t filesize,
              const std::string& file_type,
              const uint64_t nonprobative_count,
              hashdb::lmdb_changes_t& changes) {

    MUTEX_LOCK(&M);

    // maybe grow the DB
    lmdb_helper::maybe_grow(env);

    // get context
    hashdb::lmdb_context_t context(env, true, false); // writable, no duplicates
    context.open();

    // set key
    uint8_t key_start[10];
    uint8_t* key_p = key_start;
    key_p = lmdb_helper::encode_uint64_t(source_id, key_p);
    context.key.mv_size = key_p - key_start;
    context.key.mv_data = key_start;

    // set new data
    const uint64_t file_binary_hash_size = file_binary_hash.size();
    const uint64_t file_type_size = file_type.size();
    uint8_t data[file_binary_hash_size + 10 + 10 + file_type_size + 10 + 10];
    uint8_t* p = data;
    p = lmdb_helper::encode_uint64_t(file_binary_hash_size, p);
    std::memcpy(p, file_binary_hash.c_str(), file_binary_hash_size);
    p += file_binary_hash_size;
    p = lmdb_helper::encode_uint64_t(filesize, p);
    p = lmdb_helper::encode_uint64_t(file_type_size, p);
    std::memcpy(p, file_type.c_str(), file_type_size);
    p += file_type_size;
    p = lmdb_helper::encode_uint64_t(nonprobative_count, p);
    const size_t new_data_size = p - data;

    // see if source data is already there
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    if (rc == MDB_NOTFOUND) {

      // write new data directly
      context.data.mv_size = new_data_size;
      context.data.mv_data = data;
#ifdef DEBUG_LMDB_SOURCE_DATA_MANAGER_HPP
print_mdb_val("source_data_manager insert new key", context.key);
print_mdb_val("source_data_manager insert new data", context.data);
#endif
      rc = mdb_put(context.txn, context.dbi,
                   &context.key, &context.data, MDB_NODUPDATA);

      // the add request must work
      if (rc != 0) {
        std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
        assert(0);
      }
 
      // new source data inserted
      ++changes.source_data_inserted;
      context.close();
      MUTEX_UNLOCK(&M);
      return;

    } else if (rc == 0) {
#ifdef DEBUG_LMDB_SOURCE_DATA_MANAGER_HPP
print_mdb_val("source_data_manager insert change from key", context.key);
print_mdb_val("source_data_manager insert change from data", context.data);
#endif
      // already there
      if (context.data.mv_size == (new_data_size) && (std::memcmp(
                  context.data.mv_data, data, context.data.mv_size) == 0)) {
        // size and data same
        ++changes.source_data_same;
      } else {

        // different so overwrite
        context.data.mv_size = new_data_size;
        context.data.mv_data = data;
#ifdef DEBUG_LMDB_SOURCE_DATA_MANAGER_HPP
print_mdb_val("source_data_manager insert change to key", context.key);
print_mdb_val("source_data_manager insert change to data", context.data);
#endif
        rc = mdb_put(context.txn, context.dbi,
                     &context.key, &context.data, MDB_NODUPDATA);

        // the overwrite must work
        if (rc != 0) {
          std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
          assert(0);
        }
        ++changes.source_data_changed;
      }

      // closeure for already there
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
   * Find data, false on no source ID.
   */
  bool find(const uint64_t source_id,
            std::string& file_binary_hash,
            uint64_t& filesize,
            std::string& file_type,
            uint64_t& nonprobative_count) const {

    // get context
    hashdb::lmdb_context_t context(env, false, false); // not writable, no duplicates
    context.open();

    // set key
    uint8_t key_start[10];
    uint8_t* key_p = key_start;
    key_p = lmdb_helper::encode_uint64_t(source_id, key_p);
    context.key.mv_size = key_p - key_start;
    context.key.mv_data = key_start;

    // set the cursor to this key
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    if (rc == 0) {
      // hash found
#ifdef DEBUG_LMDB_SOURCE_DATA_MANAGER_HPP
print_mdb_val("source_data_manager find key", context.key);
print_mdb_val("source_data_manager find data", context.data);
#endif
      // read data
      const uint8_t* p = static_cast<uint8_t*>(context.data.mv_data);
      const uint8_t* const p_stop = p + context.data.mv_size;

      // read file_binary_hash
      uint64_t file_binary_hash_size;
      p = lmdb_helper::decode_uint64_t(p, file_binary_hash_size);
      file_binary_hash = std::string(reinterpret_cast<const char*>(p),
                            file_binary_hash_size);
      p += file_binary_hash_size;

      // read filesize
      p = lmdb_helper::decode_uint64_t(p, filesize);

      // read file_type
      uint64_t file_type_size;
      p = lmdb_helper::decode_uint64_t(p, file_type_size);
      file_type = std::string(reinterpret_cast<const char*>(p),
                            file_type_size);
      p += file_type_size;

      // read nonprobative_count
      p = lmdb_helper::decode_uint64_t(p, nonprobative_count);

      // validate that the decoding was properly consumed
      if (p != p_stop) {
        assert(0);
      }

      context.close();
      return true;

    } else if (rc == MDB_NOTFOUND) {
#ifdef DEBUG_LMDB_SOURCE_DATA_MANAGER_HPP
print_mdb_val("source_data_manager no find key", context.key);
#endif
      file_binary_hash = "";
      filesize = 0;
      file_type = "";
      nonprobative_count = 0;
      context.close();
      return false;

    } else {
      // invalid rc
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
      return false; // for mingw
    }
  }

  // call this from a lock to prevent getting an unstable answer.
  size_t size() const {
    return lmdb_helper::size(env);
  }
};

} // end namespace hashdb

#endif

