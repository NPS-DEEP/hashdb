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
 *         source_id, entropy, block_label, sub_count, 0+ file_offsets
 *
 * Type 2: first line of multi-entry hash:
 *         NULL, entropy, block_label, count
 *
 * Type 3: remaining lines of multi-entry hash:
 *         source_id, sub_count, 0+ file_offsets
 *
 * NOTES:
 *   * Source ID must be > 0 because this field also distinguishes between
 *     type 1 and Type 2 data.
 *   * LMDB sorts Type 2 before Type 3 records because of the NULL byte
 *     in type 2.
 *   * On insert, asserts if the file offset is invalid.  Validate at a
 *     higher layer.
 *   * Entropy precision is lost because entropy values are stored as
 *     integers, see entropy_scale.
 */

#ifndef LMDB_HASH_DATA_MANAGER_HPP
#define LMDB_HASH_DATA_MANAGER_HPP

//#define DEBUG_LMDB_HASH_DATA_MANAGER_HPP

#include "file_modes.h"
#include "lmdb.h"
#include "lmdb_helper.h"
#include "lmdb_context.hpp"
#include "lmdb_changes.hpp"
#include "source_id_offsets.hpp"
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

static const float entropy_scale = 1000;

namespace hashdb {

// ************************************************************
// LMDB hash data manager
// ************************************************************
class lmdb_hash_data_manager_t {

  private:
  const std::string hashdb_dir;
  const hashdb::file_mode_type_t file_mode;
  const uint32_t byte_alignment;
  const uint32_t max_count;
  const uint32_t max_sub_count;
  MDB_env* env;

#ifdef HAVE_PTHREAD
  mutable pthread_mutex_t M;                  // mutext
#else
  mutable int M;                              // placeholder
#endif

  // do not allow copy or assignment
  lmdb_hash_data_manager_t(const lmdb_hash_data_manager_t&);
  lmdb_hash_data_manager_t& operator=(const lmdb_hash_data_manager_t&);

  // delete element at current cursor
  void delete_cursor_entry(hashdb::lmdb_context_t& context) {
    int rc = mdb_cursor_del(context.cursor, 0);

    // the removal must work
    if (rc != 0) {
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
  }

  // encode file offsets into bytes at p
  uint8_t* encode_file_offsets(uint8_t* p, const uint64_t sub_count,
                               const file_offsets_t& file_offsets) const {

    // add the sub_count
    p = lmdb_helper::encode_uint64_t(sub_count, p);

    // add file offsets
    uint64_t added = 0;
    for (file_offsets_t::const_iterator it = file_offsets.begin();
          it != file_offsets.end(); ++it) {

      // stop adding if at a max_count
      if (sub_count + added > max_count) {
        break;
      }

      // stop adding if at a max sub_count
      if (added > max_sub_count) {
        break;
      }

      // add if valid file_offset
      const uint64_t file_offset = *it;
      if (file_offset % byte_alignment == 0) {
        // valid file offset
        const uint64_t file_offset_index = file_offset / byte_alignment;
        p = lmdb_helper::encode_uint64_t(file_offset_index, p);
      } else {
        // invalid file offset so warn and do not encode it
        std::cerr << "Usage error: file offset " << file_offset
                  << " does not fit evenly along step size " << byte_alignment
                  << ".\n";
      }
    }
    return p;
  }

  // read sub_count and append file offsets, require data record alignment
  uint64_t decode_file_offsets(const uint8_t* p, const uint8_t* const p_end,
                               file_offsets_t& file_offsets) const {

    // read sub_count
    uint64_t sub_count;
    p = lmdb_helper::decode_uint64_t(p, sub_count);

    // read 0+ file offsets
    while (p < p_end) {

      // file offset
      uint64_t file_offset_index;
      p = lmdb_helper::decode_uint64_t(p, file_offset_index);
      file_offsets.insert(file_offset_index * byte_alignment);
    }

    // read must align to data record
    if (p != p_end) {
      std::cerr << "data decode error in LMDB hash data store\n";
      assert(0);
    }

    return sub_count;
  }

  // add file offsets A to B
  void add_file_offsets(const file_offsets_t& more_file_offsets, 
                        file_offsets_t& file_offsets) const {

    for (file_offsets_t::const_iterator it = more_file_offsets.begin();
          it != more_file_offsets.end(); ++it) {
      file_offsets.insert(*it);
    }
  }

  // see if metadata differs
  bool metadata_differs(const float entropy1,
                        const float entropy2,
                        const std::string& block_label1,
                        const std::string& block_label2) const {
    return ((int)(entropy1*entropy_scale) != (int)(entropy2*entropy_scale) ||
            block_label1 != block_label2);
  }

/*
 * Type 1: only one entry for this hash:
 *         source_id, entropy, block_label, sub_count, 0+ file_offsets
 *
 * Type 2: first line of multi-entry hash:
 *         NULL, entropy, block_label, count
 *
 * Type 3: remaining lines of multi-entry hash:
 *         source_id, sub_count, 0+ file_offsets
 */

  // write Type 1 context.data, use existing context.key, put must work.
  void put_type1(hashdb::lmdb_context_t& context,
                 const uint64_t source_id,
                 const float entropy,
                 const std::string& block_label,
                 const uint64_t sub_count,
                 const file_offsets_t& file_offsets) {

    const size_t block_label_size = block_label.size();

    // allocate space
    size_t size = 10 + 10 + (10 + block_label_size) + 10 + 10 +
                  (10 * max_sub_count);
    uint8_t* const data = new uint8_t[size];
    uint8_t* p = data;

    // add source_id
    p = lmdb_helper::encode_uint64_t(source_id, p);

    // add scaled entropy
    const uint64_t scaled_entropy = entropy * entropy_scale;
    p = lmdb_helper::encode_uint64_t(scaled_entropy, p);

    // add block_label size and block_label
    p = lmdb_helper::encode_uint64_t(block_label_size, p);
    std::memcpy(p, block_label.c_str(), block_label_size);
    p += block_label_size;

    // add sub_count and file offsets
    p = encode_file_offsets(p, sub_count, file_offsets);

    // store data at new key
    context.data.mv_size = p - data;
    context.data.mv_data = data;
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager put_type1 key", context.key);
print_mdb_val("hash_data_manager put_type1 data", context.data);
#endif
    int rc = mdb_cursor_put(context.cursor,
                            &context.key, &context.data, MDB_NODUPDATA);

    // put must work
    if (rc != 0) {
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    delete[] data;
  }

/*
 * Type 1: only one entry for this hash:
 *         source_id, entropy, block_label, sub_count, 0+ file_offsets
 *
 * Type 2: first line of multi-entry hash:
 *         NULL, entropy, block_label, count
 *
 * Type 3: remaining lines of multi-entry hash:
 *         source_id, sub_count, 0+ file_offsets
 */
  // parse Type 1 context.data into these parameters
  void decode_type1(hashdb::lmdb_context_t& context,
                    uint64_t& source_id,
                    float& entropy,
                    std::string& block_label,
                    uint64_t& sub_count,
                    file_offsets_t& file_offsets
                   ) const {

    // clear any existing offsets
    file_offsets.clear();

    // prepare to read Type 1 entry:
    const uint8_t* const p_start = static_cast<uint8_t*>(context.data.mv_data);
    const uint8_t* p = p_start;

    // read source ID
    p = lmdb_helper::decode_uint64_t(p, source_id);

    // read entropy
    uint64_t scaled_entropy;
    p = lmdb_helper::decode_uint64_t(p, scaled_entropy);
    entropy = scaled_entropy / entropy_scale;

    // read the hash data block_label size
    uint64_t block_label_size;
    p = lmdb_helper::decode_uint64_t(p, block_label_size);

    // read the hash data block_label
    block_label =
       std::string(reinterpret_cast<const char*>(p), block_label_size);
    p += block_label_size;

    // read sub_count and file offsets
    sub_count = decode_file_offsets(p, p_start + context.data.mv_size,
                                    file_offsets);
  }

/*
 * Type 1: only one entry for this hash:
 *         source_id, entropy, block_label, sub_count, 0+ file_offsets
 *
 * Type 2: first line of multi-entry hash:
 *         NULL, entropy, block_label, count
 *
 * Type 3: remaining lines of multi-entry hash:
 *         source_id, sub_count, 0+ file_offsets
 */
  // write Type 2 context.data, use existing context.key, set cursor.
  void put_type2(hashdb::lmdb_context_t& context, const float entropy,
                 const std::string& block_label, const uint64_t count) {

    // prepare Type 2 entry:

    // make data with enough space for fields
    const size_t block_label_size = block_label.size();
    uint8_t data[1 + 10 + (10 + block_label_size) + 10];
    uint8_t* p = data;

    // add NULL byte
    p[0] = 0;
    ++p;

    // calculate scaled entropy
    const uint64_t scaled_entropy = entropy * entropy_scale;

    // set fields
    p = lmdb_helper::encode_uint64_t(scaled_entropy, p);
    p = lmdb_helper::encode_uint64_t(block_label_size, p);
    std::memcpy(p, block_label.c_str(), block_label_size);
    p += block_label_size;
    p = lmdb_helper::encode_uint64_t(count, p);

    // store data at new key
    context.data.mv_size = p - data;
    context.data.mv_data = data;
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager put_type2 key", context.key);
print_mdb_val("hash_data_manager put_type2 data", context.data);
#endif
    int rc = mdb_cursor_put(context.cursor,
                            &context.key, &context.data, MDB_NODUPDATA);

    // put must work
    if (rc != 0) {
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
  }

  // parse Type 2 context.data into these parameters
  void decode_type2(hashdb::lmdb_context_t& context,
                    float& entropy,
                    std::string& block_label,
                    uint64_t& count) const {

#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager decode_type2 key", context.key);
print_mdb_val("hash_data_manager decode_type2 data", context.data);
#endif
    // prepare to read Type 2 entry:
    const uint8_t* const p_start = static_cast<uint8_t*>(context.data.mv_data);
    const uint8_t* p = p_start;

    // move past the NULL byte
    ++p;

    // scaled entropy
    uint64_t scaled_entropy;
    p = lmdb_helper::decode_uint64_t(p, scaled_entropy);

    // entropy
    entropy = scaled_entropy / entropy_scale;

    // read the hash data block_label size
    uint64_t block_label_size;
    p = lmdb_helper::decode_uint64_t(p, block_label_size);

    // read the hash data block_label
    block_label =
       std::string(reinterpret_cast<const char*>(p), block_label_size);
    p += block_label_size;

    // read count
    p = lmdb_helper::decode_uint64_t(p, count);

    // read must align to data record
    if (p != p_start + context.data.mv_size) {
      std::cerr << "data decode error in LMDB hash data store\n";
      assert(0);
    }
  }

/*
 * Type 1: only one entry for this hash:
 *         source_id, entropy, block_label, sub_count, 0+ file_offsets
 *
 * Type 2: first line of multi-entry hash:
 *         NULL, entropy, block_label, count
 *
 * Type 3: remaining lines of multi-entry hash:
 *         source_id, sub_count, 0+ file_offsets
 */
  // write Type 3 context.data, use existing context.key, move cursor.
  void put_type3(hashdb::lmdb_context_t& context,
                 const uint64_t source_id,
                 const uint64_t sub_count,
                 const file_offsets_t& file_offsets) {

    // allocate space
    size_t size = 10 + 10 + (10 * max_sub_count);
    uint8_t* const data = new uint8_t[size];
    uint8_t* p = data;

    // add source_id
    p = lmdb_helper::encode_uint64_t(source_id, p);

    // add count and file offsets
    p = encode_file_offsets(p, sub_count, file_offsets);

    // store data at new key
    context.data.mv_size = p - data;
    context.data.mv_data = data;
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager put_type3 key", context.key);
print_mdb_val("hash_data_manager put_type3 data", context.data);
#endif
    int rc = mdb_cursor_put(context.cursor,
                            &context.key, &context.data, MDB_NODUPDATA);

    // put must work
    if (rc != 0) {
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    delete[] data;
  }

  public:
  lmdb_hash_data_manager_t(const std::string& p_hashdb_dir,
                           const hashdb::file_mode_type_t p_file_mode,
                           const uint32_t p_byte_alignment,
                           const uint32_t p_max_count,
                           const uint32_t p_max_sub_count) :
       hashdb_dir(p_hashdb_dir),
       file_mode(p_file_mode),
       byte_alignment(p_byte_alignment),
       max_count(p_max_count),
       max_sub_count(p_max_sub_count),
       env(lmdb_helper::open_env(hashdb_dir + "/lmdb_hash_data_store",
                                                                file_mode)),
       M() {

    MUTEX_INIT(&M);

    // require valid parameters
    if (byte_alignment == 0) {
      std::cerr << "invalid hash data store configuration\n";
      assert(0);
    }
  }

  ~lmdb_hash_data_manager_t() {
    // close the lmdb_hash_store DB environment
    mdb_env_close(env);

    MUTEX_DESTROY(&M);
  }

  /**
   * Insert hash with accompanying data.  Overwrite data if there
   * and changed.  Return updated source count.
   */
  size_t insert(const std::string& block_hash,
                const float entropy,
                const std::string& block_label,
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

    // require valid block_hash
    if (block_hash.size() == 0) {
      std::cerr << "Usage error: the block_hash value provided to insert is empty.\n";
      return 0;
    }

    MUTEX_LOCK(&M);

    // maybe grow the DB
    lmdb_helper::maybe_grow(env);

    // get context
    hashdb::lmdb_context_t context(env, true, true);
    context.open();
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_whole_mdb("hash_data_manager insert", context.cursor);
#endif

    // set key
    context.key.mv_size = block_hash.size();
    context.key.mv_data =
               static_cast<void*>(const_cast<char*>(block_hash.c_str()));

    // see if hash is already there
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    if (rc == MDB_NOTFOUND) {

      // hash is not there so add new Type 1
      put_type1(context, source_id, entropy, block_label,
                sub_count, file_offsets);
      ++changes.hash_data_source_inserted;
      changes.hash_data_offset_inserted += sub_count;

    } else if (rc == 0) {
      // hash is already there

#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager insert found key", context.key);
print_mdb_val("hash_data_manager insert found data", context.data);
#endif
      // require data to have size
      if (context.data.mv_size == 0) {
        std::cerr << "program error in data size\n";
        assert(0);
      }

      // find the existing entry type
      if (static_cast<uint8_t*>(context.data.mv_data)[0] != 0) {
        // Type 1

        // read the existing Type 1 entry into p_* fields:
        uint64_t p_source_id;
        float p_entropy;
        std::string p_block_label;
        uint64_t p_sub_count;
        file_offsets_t p_file_offsets;
        decode_type1(context, p_source_id, p_entropy, p_block_label,
                     p_sub_count, p_file_offsets);

        // delete the existing Type 1 entry
        delete_cursor_entry(context);

        // write back Type 1 or Type 2 and two Type 3
        if (source_id == p_source_id) {

          // Type 1

          // note if metadata differs
          if (metadata_differs(entropy, p_entropy, block_label,p_block_label)) {
            ++changes.hash_data_data_changed;
          }

          // add new file_offsets
          add_file_offsets(file_offsets, p_file_offsets);

          // write back Type 1
          uint64_t total_sub_count = p_sub_count + sub_count;
          put_type1(context, source_id, entropy, block_label,
                    total_sub_count, p_file_offsets);
        } else {
          // write back Type 2 and two Type 3, the old and the new
          uint64_t total_count = p_sub_count + sub_count;
          put_type2(context, entropy, block_label, total_count);
          put_type3(context, p_source_id, p_sub_count, p_file_offsets);
          put_type3(context, source_id, sub_count, file_offsets);
        }

      } else {
        // Type 2

        // read the existing Type 2 entry into p_* fields
        float p_entropy;
        std::string p_block_label;
        uint64_t p_count;
        decode_type2(context, p_entropy, p_block_label, p_count);

        // note if metadata differs
        if (metadata_differs(entropy, p_entropy, block_label, p_block_label)) {
          ++changes.hash_data_data_changed;
        }

        // delete the existing Type 2 entry
        delete_cursor_entry(context);

        // write back the updated Type 2 entry, setting the cursor
        uint64_t total_sub_count = p_count + sub_count;
        put_type2(context, entropy, block_label, total_sub_count);

        // replace the Type 3 entry containing this source ID
        // else add a new Type 3 entry for the new source ID
        while (true) {
          // get Type 3 record
          rc = mdb_cursor_next(context.cursor, &context.key, &context.data,
                              MDB_NEXT);

          // see if the next is for the same hash
          if (rc == MDB_NOTFOUND || context.key.mv_size != key_size ||
              memcmp(context.key.mv_data, key_start, key_size) != 0) {

            // no source ID for this hash so add new Type 3 record
            put_type3(context, source_id, file_offsets.size(), file_offsets);
            break;
          }

          // make sure rc is valid
          if (rc != 0) {
            // invalid rc
            std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
            assert(0);
          }

          // read the source ID
          const uint8_t* const p_start =
                          static_cast<uint8_t*>(context.data.mv_data);
          const uint8_t* p = p_start;
          uint64_t p_source_id;
          p = lmdb_helper::decode_uint64_t(p, p_source_id);

          // see if the source ID matches
          if (p_source_id == source_id) {

            // match

            // read the remainder of the Type3 record
            file_offsets_t p_file_offsets;
            const uint64_t p_sub_count = decode_file_offsets(
                           p, p_start + context.data.mv_size, p_file_offsets);

            // delete the existing Type 3 entry
            delete_cursor_entry(context);

            // add new file_offsets
            add_file_offsets(file_offsets, p_file_offsets);

            // put in the replacement Type 3 entry
            put_type3(context, p_source_id,
                      p_sub_count + sub_count, file_offsets);
            break;
          }
        }
      }
    } else {
      // invalid rc
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
      return 0; // for mingw
    }

    context.close();
    MUTEX_UNLOCK(&M);
    return count;
  }

  /**
   * Read data for the hash.  False if the hash does not exist.
   */
/*
 * Type 1: only one entry for this hash:
 *         source_id, entropy, block_label, sub_count, 0+ file_offsets
 *
 * Type 2: first line of multi-entry hash:
 *         NULL, entropy, block_label, count
 *
 * Type 3: remaining lines of multi-entry hash:
 *         source_id, sub_count, 0+ file_offsets
 */
  bool find(const std::string& block_hash,
            float& entropy,
            std::string& block_label,
            uint64_t& count,
            source_id_offsets_t& source_id_offsets) const {

    // clear any previous values
    entropy = 0;
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
    uint8_t* key_start = static_cast<uint8_t*>(
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
        decode_type1(context, source_id, entropy, block_label,
                     count, file_offsets);
        const uint64_t sub_count = count;
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
        decode_type2(context, entropy, block_label, count);

        // read Type 3 entries while data available and key matches
        while (true) {
          rc = mdb_cursor_next(context.cursor, &context.key, &context.data,
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

          // read the source ID
          const uint8_t* const p_start =
                          static_cast<uint8_t*>(context.data.mv_data);
          const uint8_t* p = p_start;
          uint64_t source_id;
          p = lmdb_helper::decode_uint64_t(p, source_id);

          // read the file offsets
          file_offsets_t file_offsets;
          const uint64_t sub_count = decode_file_offsets(
                           p, p_start + context.data.mv_size, file_offsets);

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
        float entropy;
        std::string block_label;
        uint64_t sub_count;
        file_offsets_t file_offsets;
        decode_type1(context, source_id, entropy, block_label,
                     sub_count, file_offsets);

        context.close();
        const uint64_t count = sub_count;
        return count;

      } else {
        // Type 2
        float entropy;
        std::string block_label;
        uint64_t count;
        decode_type2(context, entropy, block_label, count);

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
   * Return next hash else "".  Error if no next.
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

    // set the cursor to last hash
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

