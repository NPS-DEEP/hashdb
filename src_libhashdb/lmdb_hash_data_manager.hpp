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
 * Manage the LMDB hash data store.  Threadsafe.
 */

#ifndef LMDB_HASH_DATA_MANAGER_HPP
#define LMDB_HASH_DATA_MANAGER_HPP
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
#include <set>
#ifdef DEBUG
#include "to_hex.hpp"
#endif

// no concurrent writes
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "mutex_lock.hpp"

class lmdb_hash_data_manager_t {

  private:
  typedef std::pair<uint64_t, uint64_t> id_offset_pair_t;
  typedef std::set<id_offset_pair_t>    id_offset_pairs_t;

  const std::string hashdb_dir;
  const file_mode_type_t file_mode;
  const uint32_t sector_size;
  const uint32_t max_id_offset_pairs;
  id_offset_pairs_t* id_offset_pairs;
  MDB_env* env;

#ifdef HAVE_PTHREAD
  mutable pthread_mutex_t M;                  // mutext
#else
  mutable int M;                              // placeholder
#endif

  // do not allow copy or assignment
  lmdb_hash_data_manager_t(const lmdb_hash_data_manager_t&);
  lmdb_hash_data_manager_t& operator=(const lmdb_hash_data_manager_t&);

  // encoder for data=low_entropy_label, entropy, block_label,
  //                  set(source_id, file_offset)
  std::string encode_data(const std::string& low_entropy_label,
                          const uint64_t entropy,
                          const std::string& block_label,
                          const id_offset_pairs_t& pairs) const {

    // allocate space for the encoding
    size_t max_size = low_entropy_label.size() + 10 + 10 +
                      block_label.size() + 10 +
                      pairs.size() * (10 + 10);

    uint8_t encoding[max_size];
    uint8_t* p = encoding;

    // encode each field
    p = lmdb_helper::encode_string(low_entropy_label, p);
    p = lmdb_helper::encode_uint64_t(entropy, p);
    p = lmdb_helper::encode_string(block_label, p);
    for (id_offset_pairs_t::const_iterator it = pairs.begin();
                                         it != pairs.end(); ++it) {
      p = lmdb_helper::encode_uint64_t(it->first, p);
      // higher layer must already require offset to be valid
      if (it->second % sector_size != 0) {
        assert(0);
      }
      uint64_t offset_index = it->second / sector_size;
      p = lmdb_helper::encode_uint64_t(offset_index, p);
    }

#ifdef DEBUG
    std::string encoding_string(reinterpret_cast<char*>(encoding), (p-encoding));
    std::cout << "encoding low entropy label " << low_entropy_label
              << " entropy " << entropy
              << " block_label " << block_label
              << " id_offset_pairs size " << pairs.size()
              << " id_offset_pairs[";
    for (id_offset_pairs_t::const_iterator it = pairs.begin();
                                         it != pairs.end(); ++it) {
      std::cout << "(" << it->first << "," << it->second << ")";
    }

    std::cout << "]\nto binary data "
              << hashdb::to_hex(encoding_string)
              << " size " << encoding_string.size() << "\n";
#endif

    // return encoding
    return std::string(reinterpret_cast<char*>(encoding), (p-encoding));
  }

  // decoder for data=low_entropy_label, entropy, block_label,
  //                  set(source_id, file_offset)
  void decode_data(const std::string& encoding,
                   std::string& low_entropy_label,
                   uint64_t& entropy,
                   std::string& block_label,
                   id_offset_pairs_t& pairs) const {
    pairs.clear();
    const uint8_t* p_start = reinterpret_cast<const uint8_t*>(encoding.c_str());
    const uint8_t* p_stop = p_start + encoding.size();
    const uint8_t* p = p_start;

    p = lmdb_helper::decode_string(p, low_entropy_label);
    p = lmdb_helper::decode_uint64_t(p, entropy);
    p = lmdb_helper::decode_string(p, block_label);

    while(p < p_stop) {
      id_offset_pair_t pair;
      p = lmdb_helper::decode_uint64_t(p, pair.first);
      p = lmdb_helper::decode_uint64_t(p, pair.second);
      pair.second *= sector_size;
      pairs.insert(pair);
    }

#ifdef DEBUG
    std::string hex_encoding = hashdb::to_hex(encoding);
    std::cout << "decoding " << hex_encoding
              << " size " << encoding.size() << "\n to"
              << " low_entropy_label " << low_entropy_label
              << " entropy " << entropy 
              << " block_label " << block_label
              << " id_offset_pairs size " << pairs.size()
              << " id_offset_pairs[";
    for (id_offset_pairs_t::const_iterator it = pairs.begin();
                                         it != pairs.end(); ++it) {
      std::cout << "(" << it->first << "," << it->second << ")";
    }
    std::cout << "]\n";
#endif

    // validate that the decoding was properly consumed
    if ((size_t)(p - p_start) != encoding.size()) {
      std::cerr << "decode failure: " << &p << " is not " << &p_start << "\n";
      assert(0);
    }
  }

  public:
  lmdb_hash_data_manager_t(const std::string& p_hashdb_dir,
                           const file_mode_type_t p_file_mode,
                           const uint32_t p_sector_size,
                           const uint32_t p_max_id_offset_pairs) :
       hashdb_dir(p_hashdb_dir),
       file_mode(p_file_mode),
       sector_size(p_sector_size),
       max_id_offset_pairs(p_max_id_offset_pairs),
       id_offset_pairs(new id_offset_pairs_t),
       env(lmdb_helper::open_env(hashdb_dir + "/lmdb_hash_data_store",
                                                                file_mode)),
       M() {

    MUTEX_INIT(&M);

    // require valid parameters
    if (sector_size == 0) {
      std::cerr << "invalid hash data store configuration\n";
      assert(0);
    }
  }

  ~lmdb_hash_data_manager_t() {
    // close the lmdb_hash_store DB environment
    mdb_env_close(env);
    delete id_offset_pairs;

    MUTEX_DESTROY(&M);
  }

  /**
   * Insert hash with source data and metadata.
   * Return true if new, false, do not change, and note if not new.
   */
  bool insert(const std::string& binary_hash,
              const uint64_t source_id,
              const uint64_t file_offset,
              const std::string& low_entropy_label,
              const uint64_t entropy,
              const std::string& block_label,
              lmdb_changes_t& changes) {

    // require valid binary_hash
    if (binary_hash.size() == 0) {
      std::cerr << "empty key\n";
      assert(0);
    }

    // validate file_offset
    if (file_offset % sector_size != 0) {
      ++changes.hash_data_not_inserted_invalid_file_offset;
      return false;
    }

    MUTEX_LOCK(&M);

    // maybe grow the DB
    lmdb_helper::maybe_grow(env);

    // get context
    lmdb_context_t context(env, true, false);
    context.open();

    // set key
    lmdb_helper::point_to_string(binary_hash, context.key);

    // see if hash is already there
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    std::string encoding;
    if (rc == MDB_NOTFOUND) {
      // hash is not there

      // start id_offset_pairs
      id_offset_pairs->clear();
      id_offset_pairs->insert(id_offset_pair_t(source_id, file_offset));
      encoding = encode_data(low_entropy_label, entropy, block_label,
                             *id_offset_pairs);
      lmdb_helper::point_to_string(encoding, context.data);
      rc = mdb_put(context.txn, context.dbi,
                   &context.key, &context.data, MDB_NODUPDATA);

      // the add request must work
      if (rc != 0) {
        std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
        assert(0);
      }
 
      // hash data inserted
      context.close();
      ++changes.hash_data_inserted;
      MUTEX_UNLOCK(&M);
      return true;
 
    } else if (rc == 0) {
      // hash is already there

      // note if data portion is different
      std::string p_low_entropy_label;
      uint64_t p_entropy;
      std::string p_block_label;
      encoding = lmdb_helper::get_string(context.data);
      decode_data(encoding, p_low_entropy_label, p_entropy, p_block_label,
                  *id_offset_pairs);
      if (low_entropy_label != p_low_entropy_label ||
          entropy != p_entropy ||
          block_label != p_block_label) {
        ++changes.hash_data_metadata_different;
      }

      // skip if source is already there
      id_offset_pair_t id_offset_pair(source_id, file_offset);
      if (id_offset_pairs->find(id_offset_pair) != id_offset_pairs->end()) {
        // source is already there
        context.close();
        ++changes.hash_data_not_inserted_duplicate_source;
        MUTEX_UNLOCK(&M);
        return false;
      }

      // skip if source count is at max source ID offset pairs
      if (max_id_offset_pairs != 0 &&
          id_offset_pairs->size() >= max_id_offset_pairs) {
        context.close();
        ++changes.hash_data_not_inserted_max_id_offset_pairs;
        MUTEX_UNLOCK(&M);
        return false;
      }

      // add the new source
      id_offset_pairs->insert(id_offset_pair);
      encoding = encode_data(low_entropy_label, entropy, block_label,
                             *id_offset_pairs);
      lmdb_helper::point_to_string(encoding, context.data);
      rc = mdb_put(context.txn, context.dbi,
                   &context.key, &context.data, MDB_NODUPDATA);

      // the change request must work
      if (rc != 0) {
        std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
        assert(0);
      }

      // hash source inserted
      context.close();
      ++changes.hash_data_inserted;
      MUTEX_UNLOCK(&M);
      return true;

    } else {
      // invalid rc
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
  }

  /**
   * Read data for the hash.  False if the hash does not exist.
   */
  bool find(const std::string& binary_hash,
            std::string& low_entropy_label,
            uint64_t& entropy,
            std::string& block_label,
            id_offset_pairs_t& pairs) const {

    // require valid binary_hash
    if (binary_hash.size() == 0) {
      std::cerr << "empty key\n";
      assert(0);
    }

    // get context
    lmdb_context_t context(env, false, false);
    context.open();

    // set key
    lmdb_helper::point_to_string(binary_hash, context.key);

    // set the cursor to this key
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    if (rc == 0) {
      // hash found
      std::string encoding = lmdb_helper::get_string(context.data);
      decode_data(encoding, low_entropy_label, entropy, block_label,
                  pairs);
      context.close();
      return true;

    } else if (rc == MDB_NOTFOUND) {
      // no hash
      context.close();
      low_entropy_label = "";
      entropy = 0;
      block_label = "";
      pairs.clear();
      return false;

    } else {
      // invalid rc
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
  }

  /**
   * Return first hash.
   */
  std::pair<bool, std::string> find_begin() const {

    // get context
    lmdb_context_t context(env, false, false);
    context.open();

    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_FIRST);

    if (rc == 0) {
      // return the key
      context.close();
      return std::pair<bool, std::string>(
                            true, lmdb_helper::get_string(context.key));

    } else if (rc == MDB_NOTFOUND) {
      // no hash
      context.close();
      return std::pair<bool, std::string>(false, std::string(""));

    } else {
      // invalid rc
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
  }

  /**
   * Return next hash.  Error if no next.
   */
  std::pair<bool, std::string> find_next(
                        const std::string& last_binary_hash) const {

    if (last_binary_hash == "") {
      // program error to ask for next when at end
      std::cerr << "find_next: already at end\n";
      assert(0);
    }

    // get context
    lmdb_context_t context(env, false, false);
    context.open();

    // set the cursor to last hash
    lmdb_helper::point_to_string(last_binary_hash, context.key);
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    // the last hash must exist
    if (rc != 0) {
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    // move cursor to this hash
    rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                        MDB_NEXT);

    if (rc == MDB_NOTFOUND) {
      // no values for this hash
      context.close();
      return std::pair<bool, std::string>(false, std::string(""));

    } else if (rc == 0) {
      // return this hash
      context.close();
      return std::pair<bool, std::string>(
                            true, lmdb_helper::get_string(context.key));

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

#endif

