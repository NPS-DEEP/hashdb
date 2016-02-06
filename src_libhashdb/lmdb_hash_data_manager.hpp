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

#define DEBUG_LMDB_HASH_DATA_MANAGER_HPP

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
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
#include "to_hex.hpp"
#endif

// no concurrent writes
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "mutex_lock.hpp"

#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
static void print_mdb_val(const std::string& name, const MDB_val& val) {
  std::cerr << name << ": "
            << hashdb::to_hex(lmdb_helper::get_string(val)) << "\n";
}
#endif

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
   * Insert hash with source data and metadata.  Overwrite data if there
   * and changed.  Return source count.
   */
  size_t insert(const std::string& binary_hash,
                const uint64_t source_id,
                const uint64_t file_offset,
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
      ++changes.hash_data_invalid_file_offset;
      return 0;
    }
    const uint64_t file_offset_index = file_offset / sector_size;

    MUTEX_LOCK(&M);

    // maybe grow the DB
    lmdb_helper::maybe_grow(env);

    // get context
    lmdb_context_t context(env, true, false);
    context.open();

    // set key
    context.key.mv_size = binary_hash.size();
    context.key.mv_data =
               static_cast<void*>(const_cast<char*>(binary_hash.c_str()));

    // see if hash is already there
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    if (rc == MDB_NOTFOUND) {
      // hash is not there

      // make data with enough space for fields
      const size_t block_label_size = block_label.size();
      uint8_t data[10 + (10 + block_label_size) + 10 + 10];
      uint8_t* p = data;
      p = lmdb_helper::encode_uint64_t(entropy, p);
      p = lmdb_helper::encode_uint64_t(block_label_size, p);
      std::memcpy(p, block_label.c_str(), block_label_size);
      p += block_label_size;
      p = lmdb_helper::encode_uint64_t(source_id, p);
      p = lmdb_helper::encode_uint64_t(file_offset_index, p);

      // store data at new key
      context.data.mv_size = p - data;
      context.data.mv_data = data;
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager insert new key", context.key);
print_mdb_val("hash_data_manager insert new data", context.data);
#endif
      rc = mdb_put(context.txn, context.dbi,
                   &context.key, &context.data, MDB_NODUPDATA);

      // the add request must work
      if (rc != 0) {
        std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
        assert(0);
      }
 
      // hash data inserted
      ++changes.hash_data_data_inserted;
      ++changes.hash_data_source_inserted;
      context.close();
      MUTEX_UNLOCK(&M);
      return 0;
 
    } else if (rc == 0) {
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager insert change from key", context.key);
print_mdb_val("hash_data_manager insert change from data", context.data);
#endif
      // read the hash data entropy
      uint64_t p_entropy;
      const uint8_t* const old_p_start =
                                 static_cast<uint8_t*>(context.data.mv_data);
      const uint8_t* old_p = old_p_start;
      old_p = lmdb_helper::decode_uint64_t(old_p, p_entropy);

      // read the hash data block_label size
      uint64_t p_block_label_size;
      old_p = lmdb_helper::decode_uint64_t(old_p, p_block_label_size);

      // read the hash data block_label
      std::string p_block_label =
         std::string(reinterpret_cast<const char*>(old_p), p_block_label_size);
      old_p += p_block_label_size;

      // note if the data portion is the same
      const bool data_same = entropy == p_entropy &&
                             block_label == p_block_label;

      // note old_p_source_start for later
      const uint8_t* const old_p_source_start = old_p;

      // now look for the source_id, offset
      bool source_present = false;
      const uint8_t* old_p_end = static_cast<uint8_t*>(context.data.mv_data) +
                                 context.data.mv_size;
      size_t count = 0;
      while (old_p < old_p_end) {
        uint64_t p_source_id;
        old_p = lmdb_helper::decode_uint64_t(old_p, p_source_id);
        uint64_t p_file_offset_index;
        old_p = lmdb_helper::decode_uint64_t(old_p, p_file_offset_index);
        if (source_id == p_source_id &&
            file_offset_index == p_file_offset_index) {
          // source is present
          source_present = true;
          // don't break, keep going because we still need to calculate count
        }
        ++count;
      }

      // read must align to data record
      if (old_p != old_p_end) {
        assert(0);
      }

      // note if count is at max
      const bool at_max = (count >= max_id_offset_pairs) ? true : false;
 
std::cerr << "insert.b\n";
      // handle no change: same data, same source or at max
      if (data_same && (source_present || at_max)) {
std::cerr << "insert.c\n";
        ++changes.hash_data_data_same;
        if (source_present) {
          ++changes.hash_data_source_already_present;
        } else {
          ++changes.hash_data_source_at_max;
        }
        context.close();
        MUTEX_UNLOCK(&M);
        return count;

      } else {
std::cerr << "insert.d\n";
        // handle change: changed data and/or new source

        // build new record, make it sufficiently bigger than before.
        const size_t block_label_size = block_label.size();
        uint8_t* const new_data =
                 new uint8_t[context.data.mv_size + 10*5 + block_label_size];
        uint8_t* new_p = new_data;

        // put in the data fields
        new_p = lmdb_helper::encode_uint64_t(entropy, new_p);
        new_p = lmdb_helper::encode_uint64_t(block_label_size, new_p);
        std::memcpy(new_p, block_label.c_str(), block_label_size);
        new_p += block_label_size;

        // put in the existing source
        size_t existing_source_size = old_p - old_p_source_start;
        memcpy(new_p, old_p_source_start, existing_source_size);
        new_p += existing_source_size;

        // append any new source
        if (!source_present && !at_max) {
          // append the source
          new_p = lmdb_helper::encode_uint64_t(source_id, new_p);
          new_p = lmdb_helper::encode_uint64_t(file_offset_index, new_p);
        }

        // point to new_data
        context.data.mv_size = new_p - new_data;
        context.data.mv_data = new_data;
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager insert change to key", context.key);
print_mdb_val("hash_data_manager insert change to data", context.data);
#endif

        // store the new data
        rc = mdb_put(context.txn, context.dbi,
                     &context.key, &context.data, MDB_NODUPDATA);

        // the add request must work
        if (rc != 0) {
          std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
          assert(0);
        }

        // note changes
        if (data_same) {
          ++changes.hash_data_data_same;
        } else {
          ++changes.hash_data_data_changed;
        }
        if (source_present) {
          ++changes.hash_data_source_already_present;
        }
        if (at_max) {
          ++changes.hash_data_source_at_max;
        }
        if (!source_present && !at_max) {
          ++changes.hash_data_source_inserted;
        }

        delete new_data;
      }

      // done
      context.close();
      MUTEX_UNLOCK(&M);
      return count;

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
    context.key.mv_size = binary_hash.size();
    context.key.mv_data =
                 static_cast<void*>(const_cast<char*>(binary_hash.c_str()));

    // set the cursor to this key
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    if (rc == 0) {
      // hash found
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager find key", context.key);
print_mdb_val("hash_data_manager find data", context.data);
#endif

      // read the hash data entropy
      const uint8_t* p = static_cast<uint8_t*>(context.data.mv_data);
      p = lmdb_helper::decode_uint64_t(p, entropy);

      // read the hash data block_label size
      uint64_t p_block_label_size;
      p = lmdb_helper::decode_uint64_t(p, p_block_label_size);

      // read the hash data block_label
      block_label = std::string(
                     reinterpret_cast<const char*>(p), p_block_label_size);
      p += p_block_label_size;

      // clear any existing pairs
      pairs.clear();

      // read the source_id, offset pairs
      const uint8_t* const p_end =
           static_cast<uint8_t*>(context.data.mv_data) + context.data.mv_size;
      while (p < p_end) {
        uint64_t source_id;
        p = lmdb_helper::decode_uint64_t(p, source_id);
        uint64_t file_offset_index;
        p = lmdb_helper::decode_uint64_t(p, file_offset_index);
        uint64_t file_offset = file_offset_index * sector_size;
        pairs.insert(id_offset_pair_t(source_id, file_offset));
      }

      // read must align to data record
      if (p != p_end) {
        assert(0);
      }

      context.close();
      return true;

    } else if (rc == MDB_NOTFOUND) {
      // no hash
      context.close();
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

