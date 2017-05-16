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
 * Manage the source name store.  Threadsafe.
 */

#ifndef SOURCE_NAME_MANAGER_HPP
#define SOURCE_NAME_MANAGER_HPP

//#define DEBUG_SOURCE_NAME_MANAGER_HPP

#include "hdb.hpp"
#include <vector>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <string>
#include <cassert>
#ifdef DEBUG_SOURCE_NAME_MANAGER_HPP
//zz#include "lmdb_print_val.hpp"
#endif

namespace hashdb {

static void std::string encode(uint64_t source_id,
                               const std::string& repository_name,
                               const std::string& filename,
                               std::string& value) {
  uint8_t data[10+repository_name.size() + 10 + filename.size() + 10];
  uint8_t* p = data;

  // encode source ID, rn size, rn, fn size, fn
  p = lmdb_helper::encode_uint64_t(source_id, p);

  // encode rn size, rn
  const size_t repository_name_size = repository_name.size();
  p = lmdb_helper::encode_uint64_t(repository_name_size, p);
  std::memcpy(p, repository_name.c_str(), repository_name_size);
  p += repository_name_size;

  // encode fn size, fn
  const size_t filename_size = filename.size();
  p = lmdb_helper::encode_uint64_t(filename_size, p);
  std::memcpy(p, filename.c_str(), filename_size);
  p += filename_size;
  context.data.mv_size = p - data;
  context.data.mv_data = data;

  value = std::string(data, p - data);
}

static void decode(const std::string& value,
                   uint64_t& source_id,
                   std::string& repository_name,
                   std::string& filename) {
  const uint8_t* const p_start = static_cast<uint8_t*>(value.c_str());
  const uint8_t* p = p_start;
  p = lmdb_helper::decode_uint64_t(p, source_id);

  // read repository_name
  size_t repository_name_size;
  p = lmdb_helper::decode_uint64_t(p, repository_name_size);
  repository_name = std::string(reinterpret_cast<const char*>(p),
                          repository_name_size),

  // read filename
  size_t filename_size;
  p = lmdb_helper::decode_uint64_t(p, filename_size);
  filename = std::string(reinterpret_cast<const char*>(p),
                          filename_size),

  // read must align to data record
  if (p != p_start + value.size()) {
    std::cerr << "data decode error in ROCKSDB source ID store\n";
    assert(0);
  }
}


class source_name_manager_t {

  private:
  typedef std::pair<std::string, std::string> source_name_t;
  typedef std::set<source_name_t> source_names_t;

  const db_t db;
  const rocksdb::DB rdb;
  const rocksdb::ColumnFamilyHandle cfh;

  // do not allow copy or assignment
  source_name_manager_t(const source_name_manager_t&);
  source_name_manager_t& operator=(const source_name_manager_t&);

  public:
  source_name_manager_t(const db_t p_db) : db(p_db), rdb(db->db),
                       cfh(rdb->get_cfh("source_name_store")) {
  }

  ~source_name_manager_t() {
  }

  /**
   * Insert repository_name, filename pair unless pair is already there.
   */
  void insert(const uint64_t source_id,
              const std::string& repository_name,
              const std::string& filename) {

    rocksdb::Status s;

    std::string key = encode(source_id, repository_name, filename);
    s = rdb->Put(ReadOptions(), cfh, "\0", &value);
    if (!s.ok()) {
      std::cerr << db_status(s) << "\n";
      assert(0);
    }
  }

  /**
   * Find source names, false on no source ID.
   */
  bool find(const uint64_t source_id,
            source_names_t& names,
            const uint64_t max) const {

// iterate decode from source_id prefix until new source ID or max
zz

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
#ifdef DEBUG_SOURCE_NAME_MANAGER_HPP
print_mdb_val("source_name_manager find start at key", context.key);
#endif

    // note if source ID was found
    bool source_id_found = (rc == 0);

    // read name pairs while data available and key matches
    names.clear();
    while (rc == 0 &&
           context.key.mv_size == key_size &&
           memcmp(context.key.mv_data, key_start, key_size) == 0) {

#ifdef DEBUG_SOURCE_NAME_MANAGER_HPP
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
        std::cerr << "data decode error in source name store\n";
        assert(0);
      }

      // next
      rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                          MDB_NEXT);
    }

    // make sure rc is valid
    if (rc == 0 || rc == MDB_NOTFOUND) {
      // good, DB worked correctly
      context.close();
      return source_id_found;

    } else {
      // invalid rc
      std::cerr << "error: " << mdb_strerror(rc) << "\n";
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

