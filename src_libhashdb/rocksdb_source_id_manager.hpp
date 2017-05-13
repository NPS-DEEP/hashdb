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
 * Manage the ROCKSDB source ID store of key=file_binary_hash, value=source_id.
 * Threadsafe.
 */

#ifndef ROCKSDB_SOURCE_ID_MANAGER_HPP
#define ROCKSDB_SOURCE_ID_MANAGER_HPP

//#define DEBUG_ROCKSDB_SOURCE_ID_MANAGER_HPP

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
#include <cassert>
#ifdef DEBUG_ROCKSDB_SOURCE_ID_MANAGER_HPP
#include "print_mdb_val.hpp"
#endif

// no concurrent writes
#include <pthread.h>
#include "mutex_lock.hpp"

namespace hashdb {

class rocksdb_source_id_manager_t {

  private:
  const rocksdb::DB* db;
  const rocksdb::ColumnFamilyHandle h;
  mutable pthread_mutex_t M;                  // mutext

  // do not allow copy or assignment
  rocksdb_source_id_manager_t(const rocksdb_source_id_manager_t&);
  rocksdb_source_id_manager_t& operator=(const rocksdb_source_id_manager_t&);

  public:
  rocksdb_source_id_manager_t(const db_t p_db) : db(p_db),
                       h(db->get_cfh("source_id_store")), M() {
    MUTEX_INIT(&M);
  }

  ~lmdb_source_id_manager_t() {
    // close the lmdb_hash_store DB environment
    MUTEX_DESTROY(&M);
  }

  /**
   * Insert key=file_binary_hash, value=source_id.  Return bool, source_id.
   * True if new.
   */
  bool insert(const std::string& file_binary_hash,
              hashdb::lmdb_changes_t& changes, uint64_t& source_id) {

    rocksdb::Status s;

    // require valid file_binary_hash
    if (file_binary_hash.size() == 0) {
      std::cerr << "Usage error: the file_binary_hash value provided to insert is empty.\n";
      return false;
    }

    MUTEX_LOCK(&M);

    // see if source_id exists yet
    std::string value;
    db->Get(ReadOptions(), h, file_binary_hash, &value);

    if (s.ok()) {
      const uint8_t* const p_start = static_cast<uint8_t*>(value.c_str());
      const uint8_t* p = p_start;
      p = lmdb_helper::decode_uint64_t(p, source_id);

      // read must align to data record
      if (p != p_start + value.size()) {
        std::cerr << "data decode error in ROCKSDB source ID store\n";
        assert(0);
      }

      ++changes.source_id_already_present;
      context.close();
      MUTEX_UNLOCK(&M);
      return false;

    } else if (rc == MDB_NOTFOUND) {
      // generate new source ID as DB size + 1
      source_id = size() + 1;
      uint8_t data[10];
      uint8_t* p = data;
      p = lmdb_helper::encode_uint64_t(source_id, p);
      context.data.mv_size = p - data;
      context.data.mv_data = data;

#ifdef DEBUG_ROCKSDB_SOURCE_ID_MANAGER_HPP
print_mdb_val("source_id_manager insert new key", context.key);
print_mdb_val("source_id_manager insert new data", context.data);
#endif
      rc = mdb_put(context.txn, context.dbi,
                   &context.key, &context.data, MDB_NODUPDATA);

      // write must work
      if (rc != 0) {
        std::cerr << "ROCKSDB error: " << mdb_strerror(rc) << "\n";
        assert(0);
      }

      // source ID created
      ++changes.source_id_inserted;
      context.close();
      MUTEX_UNLOCK(&M);
      return true;

    } else {
      // invalid rc
      std::cerr << "ROCKSDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
      source_id = 0;
      return false; // for mingw
    }
  }

  /**
   * Find source ID else false and 0.
   */
  bool find(const std::string& file_binary_hash, uint64_t& source_id) const {

    // require valid file_binary_hash
    if (file_binary_hash.size() == 0) {
      std::cerr << "Usage error: the file_binary_hash value provided to find is empty.\n";
      return false;
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
#ifdef DEBUG_ROCKSDB_SOURCE_ID_MANAGER_HPP
print_mdb_val("source_id_manager find key", context.key);
print_mdb_val("source_id_manager find data", context.data);
#endif
      const uint8_t* p = static_cast<uint8_t*>(context.data.mv_data);
      p = lmdb_helper::decode_uint64_t(p, source_id);

      // read must align to data record
      if (p != static_cast<uint8_t*>(context.data.mv_data) +
                                                context.data.mv_size) {
        std::cerr << "data decode error in ROCKSDB source ID store\n";
        assert(0);
      }

      context.close();
      return true;

    } else if (rc == MDB_NOTFOUND) {
#ifdef DEBUG_ROCKSDB_SOURCE_ID_MANAGER_HPP
print_mdb_val("source_id_manager find not found key", context.key);
#endif
      context.close();
      source_id = 0;
      return false;

    } else {
      // invalid rc
      std::cerr << "ROCKSDB find error: " << mdb_strerror(rc) << "\n";
      assert(0);
      source_id = 0;
      return false; // for mingw
    }
  }

  /**
   * Return first file_binary_hash else false and "".
   */
  std::string first_source() const {

    // get context
    hashdb::lmdb_context_t context(env, false, false);
    context.open();

    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_FIRST);

    if (rc == 0) {
#ifdef DEBUG_ROCKSDB_SOURCE_ID_MANAGER_HPP
print_mdb_val("source_id_manager find_begin key", context.key);
print_mdb_val("source_id_manager find_begin data", context.data);
#endif
      // return the key
      std::string file_binary_hash = std::string(
              static_cast<char*>(context.key.mv_data), context.key.mv_size);
      context.close();
      return file_binary_hash;

    } else if (rc == MDB_NOTFOUND) {
      // no hash
      context.close();
      return "";

    } else {
      // invalid rc
      std::cerr << "ROCKSDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
      return ""; // for mingw
    }
  }

  /**
   * Return next source.  Error if no next.
   */
  std::string next_source(const std::string& file_binary_hash) const {

    if (file_binary_hash == "") {
      // program error to ask for next when at end
      std::cerr << "Usage error: the file_binary_hash value provided to next_source is empty.\n";
      return "";
    }

    // get context
    hashdb::lmdb_context_t context(env, false, false);
    context.open();

    // set the cursor to last file binary hash
    context.key.mv_size = file_binary_hash.size();
    context.key.mv_data =
         static_cast<void*>(const_cast<char*>(file_binary_hash.c_str()));
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    // the last file binary hash must exist
    if (rc == MDB_NOTFOUND) {
      std::cerr << "Usage error: the file_binary_hash value provided to next_source does not exist.\n";
      context.close();
      return "";
    }

    // the attempt to read the last file binary hash must work
    if (rc != 0) {
      std::cerr << "ROCKSDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    // move cursor to this file binary hash
    rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                        MDB_NEXT_NODUP);

    if (rc == MDB_NOTFOUND) {
      // no values for this file binary hash
      context.close();
      return "";

    } else if (rc == 0) {
#ifdef DEBUG_ROCKSDB_HASH_DATA_MANAGER_HPP
print_mdb_val("source_id_manager find_next key", context.key);
print_mdb_val("source_id_manager find_next data", context.data);
#endif

      // return the next file binary hash
      std::string next_file_binary_hash = std::string(
               static_cast<char*>(context.key.mv_data), context.key.mv_size);
      context.close();
      return next_file_binary_hash;

    } else {
      // invalid rc
      std::cerr << "ROCKSDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
      return ""; // for mingw
    }
  }

  // call this from a lock to prevent getting an unstable answer.
  size_t size() const {
    return lmdb_helper::size(env);
  }
};

} // end namespace hashdb

#endif

