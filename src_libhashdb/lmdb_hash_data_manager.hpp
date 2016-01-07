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
 * Manage the LMDB hash store.
 *
 * Lock non-thread-safe interfaces before use.
 */

#ifndef LMDB_HASH_DATA_MANAGER_HPP
#define LMDB_HASH_DATA_MANAGER_HPP
#include "file_modes.h"
#include "hashdb_settings_store.hpp"
#include "hashdb_settings.hpp"
#include "lmdb.h"
#include "lmdb_helper.h"
#include "lmdb_context.hpp"
#include <vector>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <string>

class lmdb_hash_data_manager_t {

  private:
  typedef std::pair<uint64_t, uint64_t> id_offset_pair_t;
  typedef std::set<id_offset_pair_t>    id_offset_pairs_t;

  std::string hashdb_dir;
  file_mode_type_t file_mode;
  hashdb_settings_t settings;
  id_offset_pairs_t& id_offset_pairs_t;
  MDB_env* env;

  // do not allow copy or assignment
  lmdb_hash_data_manager_t(const lmdb_hash_data_manager_t&);
  lmdb_hash_data_manager_t& operator=(const lmdb_hash_data_manager_t&);

  // encoder for data=non_probative_label, entropy, block_label,
  //                  set(source_id, file_offset)
  static std::string encode_data(const std::string& non_probative_label,
                                 const uint64_t entropy,
                                 const std::string& block_label,
                                 const id_offset_pairs_t& id_offset_pairs) {

    // allocate space for the encoding
    size_t max_size = non_probative_label.size() + 10 + 10 +
                      block_label.size + 10 +
                      id_offset_pairs.size * (10 + 10);

    uint8_t encoding[max_size];
    uint8_t* p = encoding;

    // encode each field
    p = lmdb_helper::encode_sized_string(non_probative_label, p);
    p = lmdb_helper::encode_uint64_t(entropy, p);
    p = lmdb_helper::encode_sized_string(block_label, p);
    for (std::set<std::string>::const_iterator it = id_offset_pairs.begin();
                                         it != id_offset_pairs.end(); ++it) {
      p = lmdb_helper::encode_uint64_t(it->first, p);
      // require offset to be valid
      if (it->second % settings.sector_size != 0) {
        assert(0);
      }
      uint64_t offset_index = it->second / settings.sector_size;
      p = lmdb_helper::encode_uint64_t(offset_index, p);
    }

#ifdef DEBUG
    std::string encoding_string(reinterpret_cast<char*>(encoding), (p-encoding));
    std::cout << "encoding label " << non_probative_label
              << " entropy " << entropy
              << " block_label " << block_label
              << " pairs count " << id_offset_pairs.size()
              << "\nto binary data "
              << binary_hash_to_hex(encoding_string)
              << " size " << encoding_string.size() << "\n";
#endif

    // return encoding
    return std::string(reinterpret_cast<char*>(encoding), (p-encoding));
  }

  // decoder for data=non_probative_label, entropy, block_label,
  //                  set(source_id, file_offset)
  static void decode_data(std::string& encoding,
                          std::string& non_probative_label,
                          uint64_t& entropy,
                          std::string& block_label,
                          id_offset_pairs_t& id_offset_pairs) {
    id_offset_pairs.clear();
    const uint8_t* p_start = reinterpret_cast<const uint8_t*>(encoding.c_str());
    const uint8_t* p_stop = p_start + encoding.size();
    const uint8_t* p = p_start;

    p = lmdb_helper::decode_string(p, non_probative_label);
    p = lmdb_helper::decode_uint64_t(p, entropy);
    p = lmdb_helper::decode_string(p, block_label);

    while(p < p_stop) {
      id_offset_pair_t pair;
      p = lmdb_helper::decode_uint64_t(p, pair.first);
      p = lmdb_helper::decode_uint64_t(p, pair.second);
      pair.second *= settings.sector_size;
      id_offset_pairs.insert(pair);
    }

#ifdef DEBUG
    std::string hex_encoding = binary_hash_to_hex(encoding);
    std::cout << "decoding " << hex_encoding
              << " size " << encoding.size() << "\n to"
              << " non_probative_label " << non_probative_label
              << " entropy " << entropy 
              << " block_label " << block_label
              << " id_offset_pairs size " << id_offset_pairs.size() << "\n";
#endif

    // validate that the decoding was properly consumed
    if ((size_t)(p - p_start) != encoding.size()) {
      std::cerr << "decode failure: " << &p << " is not " << &p_start << "\n";
      assert(0);
    }
  }

  public:
  lmdb_hash_data_manager_t(const std::string& p_hashdb_dir,
                      const file_mode_type_t p_file_mode) :
       hashdb_dir(p_hashdb_dir),
       file_mode(p_file_mode),
       settings(read_settings(hashdb_dir)),
       id_offset_pairs(new id_offset_pairs_t),
       env(lmdb_helper::open_env(hashdb_dir + "/lmdb_hash_data_store",
                                                                file_mode)) {

    // eror if settings is not initialized
    if (settings.sector_size == 0) {
      std::cerr << "Error: settings not initialized: " << settings << "\n";
      assert(0);
    }
  }

  ~lmdb_hash_data_manager_t() {
    // close the lmdb_hash_store DB environment
    mdb_env_close(env);
    delete id_offset_pairs
  }

  /**
   * Insert hash data.
   * Return true if new, false but change if not new.
   */
  bool insert_hash_data(const std::string& binary_hash,
                        const std::string& non_probative_label,
                        const uint64_t entropy,
                        const std::string& block_label) {

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
      id_offset_pairs->clear();
      encoding = encode_data(non_probative_label, entropy, block_label,
                             id_offset_pairs);
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
      return true;
 
    } else if (rc == 0) {
      // already there, but change
      std::string& p_non_probative_label;
      uint64_t p_entropy;
      std::string& p_block_label;
      encoding = lmdb_helper::get_string(context.key);
      decode_data(encoding, p_non_probative_label, p_entropy, p_block_label,
                  id_offset_pairs);
      encoding = encode_data(non_probative_label, entropy, block_label,
                             id_offset_pairs);
      lmdb_helper::point_to_string(encoding, context.data);
      rc = mdb_put(context.txn, context.dbi,
                   &context.key, &context.data, MDB_NODUPDATA);

      // the change request must work
      if (rc != 0) {
        std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
        assert(0);
      }

      // hash inserted
      context.close();
      return false;

    } else {
      // invalid rc
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
  }

  /**
   * Insert hash source.
   * Return true if added, false if the source is already there.
   * Fail if the file offset is invalid or there is no hash data yet.
   */
  bool insert_hash_source(const std::string& binary_hash,
                          const uint64_t source_id,
                          const uint64_t file_offset) {

    // validate file_offset
    if (file_offset % settings.sector_size != 0) {
      std::cerr << "invalid file offset value\n";
      return false;
    }

    // maybe grow the DB
    lmdb_helper::maybe_grow(env);

    // get context
    lmdb_context_t context(env, true, false);
    context.open();

    // set key
    lmdb_helper::point_to_string(binary_hash, context.key);

    // read the existing hash data record
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    if (rc == 0) {
      // good, the hash exists to add source data

      // the hash data values to preserve
      std::string& non_probative_label;
      uint64_t entropy;
      std::string& _block_label;

      // the source pair to add
      id_offset_pair_t id_offset_pair(source_id, file_offset);

      // decode the hash data record
      encoding = lmdb_helper::get_string(context.key);
      decode_data(encoding, non_probative_label, entropy, block_label,
                  id_offset_pairs);

      // make sure the source is not already there
      if (id_offset_pairs.find(id_offset_pair) != id_offset_pairs.end()) {
        // source is already there
        context.close();
        return false;
      } else {

        // add the new source
        id_offset_pairs.insert(id_offset_pair);
        encoding = encode_data(non_probative_label, entropy, block_label,
                               id_offset_pairs);
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
        return true;
      }

    } else {
      // invalid rc
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
  }

  /**
   * Read data for the hash.  Fail if the hash does not exist.
   */
  void find(std::string& binary_hash,
            std::string& non_probative_label,
            uint64_t entropy,
            id_offset_pairs_t& id_offset_pairs) const {

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
      decode_data(encoding, non_probative_label, entropy, block_label,
                  id_offset_pairs);
      context.close();
      return;

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
      return std::pair<bool, std::string>(true,
                                          lmdb_helper::get_string(context.key);

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
  std::string find_next(const std::string& last_binary_hash) const {

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
      return std::pair<bool, std::string>(true,
                                          lmdb_helper::get_string(context.key);

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

