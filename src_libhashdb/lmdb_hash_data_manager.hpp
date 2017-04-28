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
 * Manage the LMDB hash data store.  Threadsafe.
 *
 * lmdb_hash_data_manager_t supports three types of data fields:
 *
 * Type 1: only one entry for this hash:
 *         source_id, entropy, block_label, 2-byte sub_count up to
 *         65535, do not wrap, 2-byte buffer 0x0000 to allow transition
 *         to Type 2.
 *
 * Type 2: first line of multi-entry hash:
 *         NULL, entropy, block_label, 4-byte count, do not wrap.
 *
 * Type 3: remaining lines of multi-entry hash:
 *         source_id, 2-byte sub_count up to 65535, do not wrap.
 *
 * NOTES:
 *   * Source ID must be > 0 because this field also distinguishes between
 *     type 1 and Type 2 data.
 *   * LMDB sorts Type 2 before Type 3 records because of the NULL byte
 *     in type 2.
 *   * Some entropy precision is lost because entropy values are stored as
 *     integers, see entropy_scale.
 *   * Count and sub_count fields clip at 0xffffffff and 0xffff, respectively.
 */

#ifndef LMDB_HASH_DATA_MANAGER_HPP
#define LMDB_HASH_DATA_MANAGER_HPP

// #define DEBUG_LMDB_HASH_DATA_MANAGER_HPP

#include "file_modes.h"
#include "lmdb.h"
#include "lmdb_helper.h"
#include "lmdb_context.hpp"
#include "lmdb_changes.hpp"
#include "lmdb_hash_data_support.hpp"
#include "source_id_offsets.hpp"
#include "tprint.hpp"
#include <vector>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <string>
#include <set>
#include <cassert>
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
#include "lmdb_print_val.hpp"
#endif

// no concurrent writes
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "mutex_lock.hpp"

namespace hashdb {

  // see if data differs
  static void check_mismatch(const uint64_t k_entropy1,
                             const uint64_t k_entropy2,
                             const std::string& block_label1,
                             const std::string& block_label2,
                             lmdb_changes_t& changes) const {
    if (k_entropy1 != k_entropy2 || block_label1 != block_label2) {
      ++changes.zzzz;
    }
  }

class lmdb_hash_data_manager_t {

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
  lmdb_hash_data_manager_t(const lmdb_hash_data_manager_t&);
  lmdb_hash_data_manager_t& operator=(const lmdb_hash_data_manager_t&);

  public:
  lmdb_hash_data_manager_t(const std::string& p_hashdb_dir,
                           const hashdb::file_mode_type_t p_file_mode,
                           const uint32_t p_byte_alignment,
                           const uint32_t p_max_count,
                           const uint32_t p_max_sub_count) :
       hashdb_dir(p_hashdb_dir),
       file_mode(p_file_mode),
       env(lmdb_helper::open_env(hashdb_dir + "/lmdb_hash_data_store",
                                                                file_mode)),
       M() {

    MUTEX_INIT(&M);
  }

  ~lmdb_hash_data_manager_t() {
    // close the lmdb_hash_store DB environment
    mdb_env_close(env);

    MUTEX_DESTROY(&M);
  }

  // ************************************************************
  // insert
  // ************************************************************

  /**
   * Insert hash with accompanying data.  Warn if data present but different.
   * Return updated source count.
   *
   * Use when counting source occurrences for this hash.
   */
  size_t insert(const std::string& block_hash,
                const uint64_t k_entropy,
                std::string block_label,
                const uint64_t source_id,
                hashdb::lmdb_changes_t& changes) {

    // program error if source ID is 0 since NULL distinguishes between
    // type 1 and type 2 data.
    if (source_id == 0) {
      std::cerr << "program error in source_id\n";
      assert(0);
    }

    // get key size
    const size_t key_size = block_hash.size();
    uint8_t* const key_start = static_cast<uint8_t*>(
                 static_cast<void*>(const_cast<char*>(block_hash.c_str())));

    // require valid block_hash
    if (key_size == 0) {
      std::cerr << "Usage error: the block_hash value provided to insert is empty.\n";
      return 0;
    }

    // warn if block_label will get truncated
    if (block_label.size() > max_block_label_size) {
      // invalid file offset so warn and do not add anything
      std::stringstream ss;
      ss << "Invalid block_label length " << block_label.size()
         << " is greater than " << max_block_label_size
         << " and is truncated.\n";
      tprint(std::cerr, ss.str());
      block_label.resize(max_block_label_size);
    }

    MUTEX_LOCK(&M);

    // maybe grow the DB
    lmdb_helper::maybe_grow(env);

    // get context
    hashdb::lmdb_context_t context(env, true, true);
    context.open();
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_whole_mdb("hash_data_manager insert begin", context.cursor);
#endif

    // set key
    context.key.mv_size = key_size;
    context.key.mv_data = key_start;

    // see if hash is already there
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    // return new count when done
    uint64_t count;

    if (rc == MDB_NOTFOUND) {
      // new Type 1
      count = insert_new_type1(context, block_hash,
                               source_id, k_entropy, block_label,
                               changes);

    } else if (rc == 0) {
      // hash is already there

#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager insert found key", context.key);
print_mdb_val("hash_data_manager insert found data", context.data);
#endif
      // require existing data to have size
      if (context.data.mv_size == 0) {
        std::cerr << "program error in data size\n";
        assert(0);
      }

      // find the existing entry type
      if (static_cast<uint8_t*>(context.data.mv_data)[0] != 0) {

        // existing entry is Type 1

        // see if source_id is the same
        const uint64_t existing_source_id = decode_source_id(
                                       hashdb::lmdb_context_t& context);

        if (source_id == existing_source_id) {
          // increment Type 1, check mismatch
          increment_type1(context, source_id, k_entropy, block_label, changes);

        } else {
          // split type 1 into type 2 and type 3, check mismatch
          split_type1(context, source_id, k_entropy, block_label, changes);
        }

      } else {

        // existing entry is Type 2
        // increment type 2, check mismatch
        increment_type2(context, source_id, k_entropy, block_label, changes);

        // look for matching Type 3
        bool worked = cursor_to_type3(context, source_id);
        if (worked) {
          // increment Type 3 at cursor
          count = increment_type3(context, changes);
        } else {
          // insert new Type 3
          count = insert_new_type3(context, source_id, changes);
        }
      }

    } else {
      // invalid rc
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
      return 0; // for mingw
    }
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_whole_mdb("hash_data_manager insert end", context.cursor);
#endif

    context.close();
    MUTEX_UNLOCK(&M);
    return count;
  }

  // ************************************************************
  // merge
  // ************************************************************
  /**
   * Merge hash with accompanying data.  Overwrite data if there
   * and changed.  Add source and offsets if source is not there.
   * Warn and leave alone if source is there and sub_count is different.
   * Return updated source count.
   */
  size_t merge(const std::string& block_hash,
               const uint64_t k_entropy,
               std::string block_label,
               const uint64_t source_id,
               const uint64_t sub_count,
               const file_offsets_t file_offsets,
               hashdb::lmdb_changes_t& changes) {

    // program error if source ID is 0 since NULL distinguishes between
    // type 1 and type 2 data.
    if (source_id == 0) {
      std::cerr << "program error in source_id\n";
      assert(0);
    }

    // get key size
    const size_t key_size = block_hash.size();
    uint8_t* const key_start = static_cast<uint8_t*>(
                 static_cast<void*>(const_cast<char*>(block_hash.c_str())));

    // require valid block_hash
    if (key_size == 0) {
      std::cerr << "Usage error: the block_hash value provided to mergeis empty.\n";
      return 0;
    }

    // require sub_count >= size of file_offsets
    if (sub_count < file_offsets.size()) {
      std::stringstream ss;
      ss << "Usage error: sub_count " << sub_count
         << " provided is less than file_offsets " << file_offsets.size()
         << " provided.  Insert request aborted.\n";
      tprint(std::cerr, ss.str());
      return 0;
    }

    // require that all provided file_offsets are valid
    for (file_offsets_t::const_iterator it = file_offsets.begin();
          it != file_offsets.end(); ++it) {
      if (*it % byte_alignment != 0) {
        // invalid file offset so warn and do not add anything
        std::stringstream ss;
        ss << "Usage error: file offset " << *it 
           << " does not fit evenly along step size " << byte_alignment
           << ".  Insert request aborted.\n";
        tprint(std::cerr, ss.str());
        return 0;
      }
    }

    // warn if block_label will get truncated
    if (block_label.size() > max_block_label_size) {
      // invalid file offset so warn and do not add anything
      std::stringstream ss;
      ss << "Invalid block_label length " << block_label.size()
         << " is greater than " << max_block_label_size
         << " and is truncated.\n";
      tprint(std::cerr, ss.str());
      block_label.resize(max_block_label_size);
    }

    MUTEX_LOCK(&M);

    // maybe grow the DB
    lmdb_helper::maybe_grow(env);

    // get context
    hashdb::lmdb_context_t context(env, true, true);
    context.open();
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_whole_mdb("hash_data_manager merge begin", context.cursor);
#endif

    // set key
    context.key.mv_size = key_size;
    context.key.mv_data = key_start;

    // see if hash is already there
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    // return new count when done
    uint64_t count;

    if (rc == MDB_NOTFOUND) {
      // new Type 1
      count = merge_new_type1(context, block_hash,
                              source_id, k_entropy, block_label,
                              sub_count, file_offsets, changes);

    } else if (rc == 0) {
      // hash is already there

#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager merge found key", context.key);
print_mdb_val("hash_data_manager merge found data", context.data);
#endif
      // require existing data to have size
      if (context.data.mv_size == 0) {
        std::cerr << "program error in data size\n";
        assert(0);
      }

      // find the existing entry type
      if (static_cast<uint8_t*>(context.data.mv_data)[0] != 0) {

        // existing entry is Type 1

        // see if source_id is the same
        uint64_t existing_source_id;
        decode_source_id(context, existing_source_id);

        if (source_id == existing_source_id) {
          // update Type 1
          count = merge_update_type1(context, block_hash,
                                     source_id, k_entropy, block_label,
                                     sub_count, changes);

        } else {
          // new Type 2 and two new Type 3
          count = merge_new_type2(context, block_hash,
                                  source_id, k_entropy, block_label,
                                  sub_count, file_offsets, changes);
        }
      } else {

        // existing entry is Type 2

        // look for matching Type 3
        bool worked = cursor_to_type3(context, source_id);
        if (worked) {
          // update Type 2 and update Type 3
          count = merge_update_type3(context, block_hash,
                                     source_id, k_entropy, block_label,
                                     sub_count, changes);
        } else {
          // update Type 2 and merge new Type 3
          count = merge_new_type3(context, block_hash,
                                  source_id, k_entropy, block_label,
                                  sub_count, file_offsets, changes);
        }
      }

    } else {
      // invalid rc
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
      return 0; // for mingw
    }
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_whole_mdb("hash_data_manager merge end", context.cursor);
#endif

    context.close();
    MUTEX_UNLOCK(&M);
    return count;
  }

  /**
   * Read data for the hash.  If the hash does not exist return false
   * and empty fields.
   */
  bool find(const std::string& block_hash,
            uint64_t& k_entropy,
            std::string& block_label,
            uint64_t& count,
            source_id_offsets_t& source_id_offsets) const {

    // clear any previous values
    k_entropy = 0;
    block_label = "";
    count = 0;
    source_id_offsets.clear();

    // require valid block_hash
    if (block_hash.size() == 0) {
      std::cerr << "Usage error: the block_hash value provided to find is empty.\n";
      return false;
    }

    // get context
    hashdb::lmdb_context_t context(env, false, true);
    context.open();
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_whole_mdb("hash_data_manager find", context.cursor);
#endif

    // set key
    const size_t key_size = block_hash.size();
    uint8_t* const key_start = static_cast<uint8_t*>(
                 static_cast<void*>(const_cast<char*>(block_hash.c_str())));
    context.key.mv_size = key_size;
    context.key.mv_data = key_start;

    // set the cursor to this key
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager find start at key", context.key);
print_mdb_val("hash_data_manager find start at data", context.data);
#endif

    if (rc == MDB_NOTFOUND) {
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager find did not find key", context.key);
#endif
      // no hash
      context.close();
      return false;

    } else if (rc == 0) {
      // check first byte to see if the entry is Type 1 or Type 2
      // require data to have size
      if (context.data.mv_size == 0) {
        std::cerr << "program error in data size\n";
        assert(0);
      }

      // find the existing entry type
      if (static_cast<uint8_t*>(context.data.mv_data)[0] != 0) {

        // existing entry is Type 1 so read it and be done:
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager find Type 1 key", context.key);
print_mdb_val("hash_data_manager find Type 1 data", context.data);
#endif
        uint64_t source_id;
        uint64_t sub_count;
        file_offsets_t file_offsets;
        decode_type1(context, source_id, k_entropy, block_label,
                     sub_count, file_offsets);
        count = sub_count;
        source_id_offsets.insert(
                   source_id_offset_t(source_id, sub_count, file_offsets));
        context.close();
        return true;

      } else {
        // existing entry is Type 2 so read all entries for this hash
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager find Type 2 key", context.key);
print_mdb_val("hash_data_manager find Type 2 data", context.data);
#endif
        // read the existing Type 2 entry into returned fields
        uint64_t count_stored;
        decode_type2(context, k_entropy, block_label, count, count_stored);

        // read Type 3 entries while data available and key matches
        while (true) {
          rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                              MDB_NEXT);
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager find Type 3 key", context.key);
print_mdb_val("hash_data_manager find Type 3 data", context.data);
#endif

          if (rc == MDB_NOTFOUND || context.key.mv_size != key_size ||
              memcmp(context.key.mv_data, key_start, key_size) != 0) {
            // EOF or past key so done
            break;
          }

          // make sure rc is valid
          if (rc != 0) {
            // invalid rc
            std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
            assert(0);
          }

          // read Type 3
          uint64_t source_id;
          uint64_t sub_count;
          file_offsets_t file_offsets;
          decode_type3(context, source_id, sub_count, file_offsets);

          // add the LMDB hash data
          source_id_offsets.insert(
                   source_id_offset_t(source_id, sub_count, file_offsets));
        }

        context.close();
        return true;
      }

    } else {
      // invalid rc
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
      return false; // for mingw
    }
    std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
    assert(0); // for mingw
    return false; // for mingw
  }

  /**
   * Return source count for this hash.
   */
  size_t find_count(const std::string& block_hash) const {

    // require valid block_hash
    if (block_hash.size() == 0) {
      std::cerr << "Usage error: the block_hash value provided to find_count is empty.\n";
      return 0;
    }

    // get context
    hashdb::lmdb_context_t context(env, false, true);
    context.open();

    // set key
    context.key.mv_size = block_hash.size();
    context.key.mv_data =
                 static_cast<void*>(const_cast<char*>(block_hash.c_str()));

    // set the cursor to this key
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    if (rc == MDB_NOTFOUND) {
      // this hash is not in the DB
      context.close();
      return 0;

    } else if (rc == 0) {
      // hash found
      // require data to have size
      if (context.data.mv_size == 0) {
        std::cerr << "program error in data size\n";
        assert(0);
      }

      // check first byte to see if the entry is Type 1 or Type 2
      if (static_cast<uint8_t*>(context.data.mv_data)[0] != 0) {
        // Type 1

        // read the existing Type 1 entry into fields:
        uint64_t source_id;
        uint64_t k_entropy;
        std::string block_label;
        uint64_t sub_count;
        file_offsets_t file_offsets;
        decode_type1(context, source_id, k_entropy, block_label,
                     sub_count, file_offsets);

        context.close();
        const uint64_t count = sub_count;
        return count;

      } else {
        // Type 2
        uint64_t k_entropy;
        std::string block_label;
        uint64_t count;
        uint64_t count_stored;
        decode_type2(context, k_entropy, block_label, count, count_stored);

        context.close();
        return count;
      }

    } else {
      // invalid rc
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
      return 0; // for mingw
    }
  }

  /**
   * Return first hash else "".
   */
  std::string first_hash() const {

    // get context
    hashdb::lmdb_context_t context(env, false, true);
    context.open();

    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_FIRST);

    if (rc == 0) {
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager find_begin key", context.key);
print_mdb_val("hash_data_manager find_begin data", context.data);
#endif
      // return the key
      std::string block_hash = std::string(
                static_cast<char*>(context.key.mv_data), context.key.mv_size);
      context.close();
      return block_hash;

    } else if (rc == MDB_NOTFOUND) {
      // no hash
      context.close();
      return "";

    } else {
      // invalid rc
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
      return ""; // for mingw
    }
  }

  /**
   * Return next hash value else "" if end.  Error if last is "" or invalid.
   */
  std::string next_hash(const std::string& block_hash) const {

    if (block_hash == "") {
      // program error to ask for next when at end
      std::cerr << "Usage error: the block_hash value provided to next_hash is empty.\n";
      return "";
    }

    // get context
    hashdb::lmdb_context_t context(env, false, true);
    context.open();

    // set the cursor to previous hash
    context.key.mv_size = block_hash.size();
    context.key.mv_data =
             static_cast<void*>(const_cast<char*>(block_hash.c_str()));
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    // the last hash must exist
    if (rc == MDB_NOTFOUND) {
      std::cerr << "Usage error: the block_hash value provided to next_hash does not exist.\n";
      context.close();
      return "";
    }

    // the attempt to access the last hash must work
    if (rc != 0) {
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    // move cursor to this hash
    rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                        MDB_NEXT_NODUP);

    if (rc == MDB_NOTFOUND) {
      // no values for this hash
      context.close();
      return "";

    } else if (rc == 0) {
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager find_next key", context.key);
print_mdb_val("hash_data_manager find_next data", context.data);
#endif

      // return the next hash
      std::string next_block_hash = std::string(
               static_cast<char*>(context.key.mv_data), context.key.mv_size);
      context.close();
      return next_block_hash;

    } else {
      // invalid rc
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
      return "";
    }
  }

  // call this from a lock to prevent getting an unstable answer.
  size_t size() const {
    return lmdb_helper::size(env);
  }
};

} // end namespace hashdb

#endif

