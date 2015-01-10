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
 * LMDB resources.
 */

#ifndef LMDB_RESOURCES_H
#define LMDB_RESOURCES_H
#include "lmdb.h"
#include "hash_t_selector.h"

// pair type
typedef std::pair<hash_t, uint64_t> hash_pair_t;

// resources for supporting thread-specific transaction objects
struct lmdb_resources_t {
  MDB_txn* txn;
  MDB_dbi dbi;

  // resource-local scratchpad for cursor, key, and data
  MDB_cursor* cursor;
  MDB_val key;
  MDB_val data;

  lmdb_resources_t() :txn(0), dbi(0), cursor(0), key(), data() {
    key.mv_size = sizeof(hash_t);
    data.mv_size = sizeof(uint64_t);
  }
};

inline void pair_to_mdb(const hash_t& hash, const uint64_t& value,
                        MDB_val& key, MDB_val& data) {

  // output points to input so input isn't as safe as is implied.
  key.mv_size = sizeof(hash_t);
  key.mv_data = const_cast<void*>(static_cast<const void*>(&hash));
  data.mv_size = sizeof(uint64_t);
  data.mv_data = const_cast<void*>(static_cast<const void*>(&value));
}

inline hash_pair_t mdb_to_pair(const MDB_val& key, const MDB_val& data) {
  if (key.mv_size != sizeof(hash_t)) {
    std::cout << "key " << key.mv_size << " not " << sizeof(hash_t) << "\n";
    assert(0);
  }
  if (data.mv_size != sizeof(uint64_t)) {
    std::cout << "data " << data.mv_size << " not " << sizeof(uint64_t) << "\n";
    assert(0);
  }

  return hash_pair_t(*static_cast<hash_t*>(key.mv_data),
                     *static_cast<uint64_t*>(data.mv_data));
}

#endif

