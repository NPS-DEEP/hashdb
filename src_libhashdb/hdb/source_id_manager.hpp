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

#include "lmdb.h"
#include "lmdb_helper.h"
#include "lmdb_context.hpp"
#include "lmdb_changes.hpp"
#include "rocksdb_status.hpp"
#include "rocksdb_protobuf.hpp"
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

namespace hdb {

static void std::string encode_source_id(uint64_t source_id,
                                         std::string& value) {
  uint8_t data[10];
  uint8_t* p = data;
  p = lmdb_helper::encode_uint64_t(source_id, p);
  value = std::string(data, p - data);
}

static void decode_source_id(const std::string& value, uint64_t& source_id) {
  const uint8_t* const p_start = static_cast<uint8_t*>(value.c_str());
  const uint8_t* p = p_start;
  p = lmdb_helper::decode_uint64_t(p, source_id);

  // read must align to data record
  if (p != p_start + value.size()) {
    std::cerr << "data decode error in ROCKSDB source ID store\n";
    assert(0);
  }
}

class source_id_manager_t {

  private:
  const db_t db;
  const rocksdb::DB rdb;
  const rocksdb::ColumnFamilyHandle cfh;
  mutable pthread_mutex_t M;                  // mutext

  // do not allow copy or assignment
  source_id_manager_t(const source_id_manager_t&);
  source_id_manager_t& operator=(const source_id_manager_t&);

  public:
  source_id_manager_t(const db_t p_db) : db(p_db), rdb(db->db),
                       cfh(rdb->get_cfh("source_id_store")), M() {
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
    s = rdb->Get(ReadOptions(), cfh, file_binary_hash, &value);

    if (s.ok()) {
      // source ID exists so return it
      decode_source_id(value, source_id);

      ++changes.source_id_already_present;
      MUTEX_UNLOCK(&M);
      return false;

    } else if (s.IsNotFound()) {
      // find largest source ID which is stored at record key="\0"
      s = rdb->Get(ReadOptions(), cfh, "\0", &value);
      if (s.ok()) {
        // value exists so use next
        decode_source_id(value, source_id);
        ++source_id;
      } else if (s.IsNotFound()) {
        // new value
        source_id = 1;
      } else {
        // error
        std::cerr << db_status(s) << "\n";
        assert(0);
      }

      // store incremented largest source ID
      encode_source_id(source_id, value);
      s = rdb->Put(ReadOptions(), cfh, "\0", &value);
      if (!s.ok()) {
        std::cerr << db_status(s) << "\n";
        assert(0);
      }
      
      ++changes.source_id_inserted;
      MUTEX_UNLOCK(&M);
      return true;

    } else {
      // invalid status
      std::cerr << db_status(s) << "\n";
      assert(0);
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

    // find source_id
    std::string value;
    s = rdb->Get(ReadOptions(), cfh, file_binary_hash, &value);

    if (s.ok()) {
      // source ID exists so return it
      decode_source_id(value, source_id);
      return true;

    } else if (s.IsNotFound()) {
      source_id = 0;
      return false;

    } else {

      // invalid status
      std::cerr << db_status(s) << "\n";
      assert(0);
    }
  }

  /**
   * Return heap-allocated file_binary_hash rocksdb::Iterator.
   * Delete it when done.
   */
  rocksdb::Iterator* iterator() const {
    rocksdb::Iterator* it = rdb->NewIterator(ReadOptions(), cfh);
    // position to front
    it->SeekToFirst();
    return it;
  }

  /**
   * Return next file_binary_hash else "".
   */
  std::string next(rocksdb::Iterator* it) const {

    if(it->valid()) {
      // key available
      rocksdb::Slice key = it->key();
      it.Next();
      return std::string(key.data(), key.size());
    } else {
      // no key
      return "";
    }
  }

/*
zz not available with RocksDB
  // call this from a lock to prevent getting an unstable answer.
  size_t size() const {
    return lmdb_helper::size(env);
  }
*/
};

} // end namespace hdb

#endif

