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
#include "source_id_sub_counts.hpp"
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
inline bool mismatched_data(const uint64_t k_entropy1,
                            const uint64_t k_entropy2,
                            const std::string& block_label1,
                            const std::string& block_label2) {
  return (k_entropy1 != k_entropy2 || block_label1 != block_label2);
}

// see if sub_countdiffers
inline bool mismatched_sub_count(const uint64_t sub_count1,
                                 const uint64_t sub_count2) {
  return (sub_count1 != sub_count2);
}

// add or clip
inline uint64_t add2(const uint64_t a, const uint64_t b) {
  return (a+b>0xffff) ? 0xffff : a+b;
}
inline uint64_t add4(const uint64_t a, const uint64_t b) {
  return (a+b>0xffffffff) ? 0xffffffff : a+b;
}

// maybe truncate block_label
static std::string truncate_block_label(std::string block_label) {
  if (block_label.size() > hashdb::max_block_label_size) {
    // truncate and warn
    std::stringstream ss;
    ss << "Invalid block_label length " << block_label.size()
       << " is greater than " << hashdb::max_block_label_size
       << " and is truncated.\n";
    hashdb::tprint(std::cerr, ss.str());
    block_label.resize(hashdb::max_block_label_size);
  }
  return block_label;
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
                           const hashdb::file_mode_type_t p_file_mode) :
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
                const std::string& p_block_label,
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

    // maybe truncate block_label
    const std::string block_label = truncate_block_label(p_block_label);

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

    // return new count when done
    uint64_t count;

    // see if hash is already there
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    if (rc == MDB_NOTFOUND) {
      // new Type 1
      new_type1(context, block_hash, k_entropy, block_label, source_id, 1);
      count = 1;

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

        // read type 1
        uint64_t existing_k_entropy;
        std::string existing_block_label;
        uint64_t existing_source_id;
        uint64_t existing_sub_count;
        decode_type1(context, existing_k_entropy, existing_block_label,
                     existing_source_id, existing_sub_count);

        // check for mismatched data
        if (mismatched_data(k_entropy, existing_k_entropy,
                            block_label, existing_block_label)) {
          ++changes.hash_data_mismatched_data_detected;
        }

        // see if source_id is the same
        if (source_id == existing_source_id) {
          // increment Type 1
          replace_type1(context, block_hash, existing_k_entropy,
                        existing_block_label, source_id,
                        add2(existing_sub_count,1));

        } else {
          // split type 1 into type 2 and two type 3
          replace_type2(context, block_hash, existing_k_entropy,
                        existing_block_label, add2(existing_sub_count,1));
          new_type3(context, block_hash, existing_source_id,
                    existing_sub_count);
          new_type3(context, block_hash, source_id, 1);
        }

        // for type 1, new count is existing_sub_count + 1
        count = add2(existing_sub_count, 1);

      } else {

        // existing entry is Type 2

        // read type 2
        uint64_t existing_k_entropy;
        std::string existing_block_label;
        uint64_t existing_count;
        decode_type2(context, existing_k_entropy, existing_block_label,
                     existing_count);

        // check for mismatched data
        if (mismatched_data(k_entropy, existing_k_entropy,
                            block_label, existing_block_label)) {
          ++changes.hash_data_mismatched_data_detected;
        }

        // increment count at type 2
        replace_type2(context, block_hash, existing_k_entropy,
                      existing_block_label, add4(existing_count,1));

        // look for existing type 3
        uint64_t existing_sub_count;
        if (cursor_to_type3(context, source_id, existing_sub_count)) {
          // increment sub_count at type 3
          replace_type3(context, block_hash, source_id,
                        add2(existing_sub_count,1));
        } else {
          // new type 3
          new_type3(context, block_hash, source_id, 1);
        }

        // for type 2, new count is existing_count + 1
        count = add4(existing_count,1);
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

    // insert is always accepted
    ++changes.hash_data_inserted;

    context.close();
    MUTEX_UNLOCK(&M);
    return count;
  }

  // ************************************************************
  // merge
  // ************************************************************
  /**
   * Merge hash with accompanying data.  Warn if data present but different.
   * Return updated source count.
   */
  size_t merge(const std::string& block_hash,
               const uint64_t k_entropy,
               const std::string& p_block_label,
               const uint64_t source_id,
               const uint64_t sub_count,
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

    // maybe truncate block_label
    const std::string block_label = truncate_block_label(p_block_label);

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

    // return new count when done
    uint64_t count;

    // see if hash is already there
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    if (rc == MDB_NOTFOUND) {
      // new Type 1
      new_type1(context, block_hash, k_entropy, block_label, source_id,
                sub_count);
      changes.hash_data_merged += add2(sub_count,0);
      count = add2(sub_count,0);

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

        // read type 1
        uint64_t existing_k_entropy;
        std::string existing_block_label;
        uint64_t existing_source_id;
        uint64_t existing_sub_count;
        decode_type1(context, existing_k_entropy, existing_block_label,
                     existing_source_id, existing_sub_count);

        // check for mismatched data
        if (mismatched_data(k_entropy, existing_k_entropy,
                            block_label, existing_block_label)) {
          ++changes.hash_data_mismatched_data_detected;
        }

        // see if source_id is the same
        if (source_id == existing_source_id) {

          // check for mismatched sub_count
          if (mismatched_sub_count(sub_count, existing_sub_count)) {
            ++changes.hash_data_mismatched_sub_count_detected;
          }

          // merged before, no change
          ++changes.hash_data_merged_same += sub_count;

        } else {
          // split type 1 into type 2 and two type 3
          replace_type2(context, block_hash, existing_k_entropy,
                        existing_block_label,
                        add2(existing_sub_count,sub_count));
          new_type3(context, block_hash, existing_source_id,
                    existing_sub_count);
          new_type3(context, block_hash, source_id, sub_count);

          changes.hash_data_merged += sub_count;
        }

        // for type 1, new count is existing_sub_count + sub_count
        count = add2(existing_sub_count,sub_count);

      } else {

        // existing entry is Type 2

        // read type 2
        uint64_t existing_k_entropy;
        std::string existing_block_label;
        uint64_t existing_count;
        decode_type2(context, existing_k_entropy, existing_block_label,
                     existing_count);

        // check for mismatched data
        if (mismatched_data(k_entropy, existing_k_entropy,
                            block_label, existing_block_label)) {
          ++changes.hash_data_mismatched_data_detected;
        }

        // look for existing type 3
        uint64_t existing_sub_count;
        if (cursor_to_type3(context, source_id, existing_sub_count)) {

          // existing type 3 was merged before, no change

          // check for mismatched sub_count
          if (mismatched_sub_count(sub_count, existing_sub_count)) {
            ++changes.hash_data_mismatched_sub_count_detected;
          }
          count = existing_sub_count;
          ++changes.hash_data_merged_same += sub_count;

        } else {
          // new type 3

          // add sub_count to type 2
          cursor_to_first_current(context);
          replace_type2(context, block_hash, existing_k_entropy,
                        existing_block_label, add4(existing_count,sub_count));

          // new type 3 for new souce_id
          new_type3(context, block_hash, source_id, sub_count);

          count = add4(existing_count,sub_count);
          changes.hash_data_merged += sub_count;
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

  // ************************************************************
  // find
  // ************************************************************
  /**
   * Read data for the hash.  If the hash does not exist return false
   * and empty fields.
   */
  bool find(const std::string& block_hash,
            uint64_t& k_entropy,
            std::string& block_label,
            uint64_t& count,
            source_id_sub_counts_t& source_id_sub_counts) const {

    // clear any previous values
    k_entropy = 0;
    block_label = "";
    count = 0;
    source_id_sub_counts.clear();

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
        decode_type1(context, k_entropy, block_label, source_id, sub_count);
        source_id_sub_counts.insert(source_id_sub_count_t(source_id,
                                                          sub_count));
        context.close();
        return true;

      } else {
        // existing entry is Type 2 so read all entries for this hash
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager find Type 2 key", context.key);
print_mdb_val("hash_data_manager find Type 2 data", context.data);
#endif
        // read the existing Type 2 entry into returned fields
        decode_type2(context, k_entropy, block_label, count);

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
          decode_type3(context, source_id, sub_count);

          // add the LMDB hash data
          source_id_sub_counts.insert(source_id_sub_count_t(source_id,
                                                            sub_count));
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

  // ************************************************************
  // find_count
  // ************************************************************
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
        uint64_t k_entropy;
        std::string block_label;
        uint64_t source_id;
        uint64_t sub_count;
        decode_type1(context, k_entropy, block_label, source_id, sub_count);

        context.close();
        return sub_count;

      } else {
        // Type 2

        // read the existing Type 2 entry into fields:
        uint64_t k_entropy;
        std::string block_label;
        uint64_t count;
        decode_type2(context, k_entropy, block_label, count);

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

  // ************************************************************
  // first_hash
  // ************************************************************
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

  // ************************************************************
  // next_hash
  // ************************************************************
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

