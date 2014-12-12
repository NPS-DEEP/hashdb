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
 * Converts between LMDB and hashdb key and data types.
 * Can readily be upgraded to enforce an endian order.
 */

#ifndef LMDB_HASH_STORE_TYPE_H
#define LMDB_HASH_STORE_TYPE_H
#include "hash_t_selector.h"

// key and data types for hash_store
//typedef hash_t key_t;
//typedef uint64_t data_t;
//typedef std::pair<key_t, data_t> pair_t;
typedef std::pair<hash_t, uint64_t> pair_t;

inline pair_t lmdb_hash_store_to_pair(const MDB_val& key, const MDB_val& data) {
  if (key.mv_size != sizeof(hash_t)) {
    assert(0);
  }
  if (data.mv_size != sizeof(uint64_t)) {
    assert(0);
  }

  return std::pair<hash_t, uint64_t>(static_cast<hash_t>(*key.mv_data),
                                     static_cast<uint64_t>(*data.mv_data));
}

inline void pair_to_hash_store(const pair_t& pair, MDB_val& key, MDB_val& data) {
  static_cast<hash_t>(key.mv_data) = pair.first;
  key.mv_size = sizeof(pair.first);
  static cast<uint64_t>(data.mv_data) = pair.second;
  data.mv_size = sizeof(pair.second);
}

inline void pair_to_hash_store(const hash_t& hash, uint64_t value, MDB_val& key, MDB_val& data) {
  static_cast<hash_t>(key.mv_data) = hash;
  key.mv_size = sizeof(pair.first);
  static cast<uint64_t>(data.mv_data) = value;
  data.mv_size = sizeof(pair.second);
}

#endif

