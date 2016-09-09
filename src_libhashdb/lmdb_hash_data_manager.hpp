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
 *         NULL, entropy, block_label, count, count_stored
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

// #define DEBUG_LMDB_HASH_DATA_MANAGER_HPP

#include "file_modes.h"
#include "lmdb.h"
#include "lmdb_helper.h"
#include "lmdb_context.hpp"
#include "lmdb_changes.hpp"
#include "source_id_offsets.hpp"
#include "tprint.hpp"
#include <vector>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <string>
#include <set>
#include <cassert>
#include <math.h>
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
#include "lmdb_print_val.hpp"
#endif

// no concurrent writes
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "mutex_lock.hpp"

static const float entropy_scale = 1000; // for 3 decimal places of precision
static const size_t max_lmdb_data_size = 511; // imposed by LMDB
static const size_t max_lmdb_block_label_size = 10;
static const size_t max_lmdb_sub_count = 50;

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

  // move cursor to first entry of current key
  void cursor_to_first_current(hashdb::lmdb_context_t& context) {
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_FIRST_DUP);

    // the move must work
    if (rc != 0) {
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
  }

  // move cursor forward from Type 2 to correct Type 3 else false and rewind
  bool cursor_to_type3(hashdb::lmdb_context_t& context,
                       const uint64_t source_id) {

    while (true) {
      // get next Type 3 record
      int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                              MDB_NEXT_DUP);

      if (rc == 0) {
        uint64_t next_source_id;
        decode_source_id(context, next_source_id);
        if (source_id == next_source_id) {
          return true;
        }
      } else if (rc == MDB_NOTFOUND) {
        // back up cursor to Type 2
        rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_FIRST_DUP);
        return false;
      } else {
        // invalid rc
        std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
        assert(0);
      }
    }
  }

  // encode file offsets into bytes at p
  // At this layer, invalid byte alignment is fatal.
  uint8_t* encode_file_offsets(uint8_t* p,
                               const uint64_t sub_count,
                               const file_offsets_t& file_offsets) const {

    // encode the sub_count
    p = lmdb_helper::encode_uint64_t(sub_count, p);

    // encode file offsets
    for (file_offsets_t::const_iterator it = file_offsets.begin();
          it != file_offsets.end(); ++it) {

      // encode file_offset else fail if invalid
      const uint64_t file_offset = *it;
      if (file_offset % byte_alignment == 0) {
        // valid file offset
        const uint64_t file_offset_index = file_offset / byte_alignment;
        p = lmdb_helper::encode_uint64_t(file_offset_index, p);
      } else {
        assert(0);
      }
    }
    return p;
  }

  // decode source ID from Type 1 or Type 3
  void decode_source_id(hashdb::lmdb_context_t& context,
                    uint64_t& source_id) {

    // prepare to read Type 1 or Type 3 entry:
    const uint8_t* const p_start = static_cast<uint8_t*>(context.data.mv_data);
    const uint8_t* p = p_start;

    // source ID
    p = lmdb_helper::decode_uint64_t(p, source_id);

    // read must finish before end of data record
    if (p >= p_start + context.data.mv_size) {
      std::cerr << "data decode error in LMDB hash data store\n";
      assert(0);
    }
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

  // Add file offset A to offsets list B as maximums allow.  Warn when
  // attempting to add duplicate offsets.  Return count added to offsets list.
  uint64_t insert_file_offset(const uint64_t source_file_offset,
                              const size_t count_stored,
                              file_offsets_t& destination_file_offsets,
                              hashdb::lmdb_changes_t& changes) {

    // stop adding if at a max_count
    if (count_stored >= max_count) {
      return 0;
    }

    // stop adding if at a max_sub_count
    if (destination_file_offsets.size() >= max_sub_count) {
      return 0;
    }

    // add the offset
    std::pair<file_offsets_t::const_iterator, bool> pair =
                       destination_file_offsets.insert(source_file_offset);
    if (pair.second == true) {
      // the file offset was added
      return 1;
    } else {
      // the file offset was not added because it was already there
      ++changes.hash_data_duplicate_offset_detected;
      return 0;
    }
  }

  // prepare addable file offsets
  uint64_t merge_file_offsets(const file_offsets_t& file_offsets, 
                              const size_t count_stored,
                              file_offsets_t& addable_file_offsets,
                              hashdb::lmdb_changes_t& changes) {

    // destination must be empty
    if (addable_file_offsets.size() != 0) {
      assert(0);
    }

    // copy to addable as maximums allow
    size_t sub_count_stored = 0;
    for (file_offsets_t::const_iterator it = file_offsets.begin();
          it != file_offsets.end(); ++it) {

      // stop adding if at a max_count
      if (sub_count_stored + count_stored >= max_count) {
        break;
      }

      // stop adding if at a max_sub_count
      if (addable_file_offsets.size() >= max_sub_count) {
        break;
      }

      // add the offset
      addable_file_offsets.insert(*it);

      // the file offset was added
      ++sub_count_stored;
    }
    return sub_count_stored;
  }

  // see if metadata differs
  bool metadata_differs(const float entropy1,
                        const float entropy2,
                        const std::string& block_label1,
                        const std::string& block_label2) const {
    return ((int)(entropy1*entropy_scale) != (int)(entropy2*entropy_scale) ||
            block_label1 != block_label2);
  }

  size_t encode_type1(const uint64_t source_id,
                      const float entropy,
                      const std::string& block_label,
                      const uint64_t sub_count,
                      const file_offsets_t& file_offsets,
                      uint8_t* const data) {

    const size_t block_label_size =
                (block_label.size() > max_lmdb_block_label_size)
                 ? max_lmdb_block_label_size : block_label.size();
    // space for encoding
    uint8_t* p = data;

    // add source_id
    p = lmdb_helper::encode_uint64_t(source_id, p);

    // add scaled entropy
    const uint64_t scaled_entropy = round(entropy * entropy_scale);
    p = lmdb_helper::encode_uint64_t(scaled_entropy, p);

    // add block_label size and block_label
    p = lmdb_helper::encode_uint64_t(block_label_size, p);
    std::memcpy(p, block_label.c_str(), block_label_size);
    p += block_label_size;

    // add sub_count and file offsets
    p = encode_file_offsets(p, sub_count, file_offsets);

    // check bounds
    if (p > data + max_lmdb_data_size) {
      assert(0);
    }

    return p - data;
  }

  size_t encode_type2(const float entropy,
                      const std::string& block_label,
                      const uint64_t count,
                      const uint64_t count_stored,
                      uint8_t* const data) {

    const size_t block_label_size =
                (block_label.size() > max_lmdb_block_label_size)
                 ? max_lmdb_block_label_size : block_label.size();
    // space for encoding
    uint8_t* p = data;

    // add NULL byte
    p[0] = 0;
    ++p;

    // calculate scaled entropy
    const uint64_t scaled_entropy = round(entropy * entropy_scale);

    // set fields
    p = lmdb_helper::encode_uint64_t(scaled_entropy, p);
    p = lmdb_helper::encode_uint64_t(block_label_size, p);
    std::memcpy(p, block_label.c_str(), block_label_size);
    p += block_label_size;
    p = lmdb_helper::encode_uint64_t(count, p);
    p = lmdb_helper::encode_uint64_t(count_stored, p);

    // check bounds
    if (p > data + max_lmdb_data_size) {
      assert(0);
    }

    return p - data;
  }

  size_t encode_type3(const uint64_t source_id,
                      const uint64_t sub_count,
                      const file_offsets_t& file_offsets,
                      uint8_t* const data) {

    // allocate space
    uint8_t* p = data;

    // add source_id
    p = lmdb_helper::encode_uint64_t(source_id, p);

    // add count and file offsets
    p = encode_file_offsets(p, sub_count, file_offsets);

    // check bounds
    if (p > static_cast<uint8_t*>(data + max_lmdb_data_size)) {
      assert(0);
    }

    return p - data;
  }

  // write the enocding.  Key must be valid.
  void write_encoding(hashdb::lmdb_context_t& context,
                      const std::string& key,
                      const uint8_t* const data, const size_t data_size) {

    // set key and data
    context.key.mv_size = key.size();
    context.key.mv_data = static_cast<uint8_t*>(
             static_cast<void*>(const_cast<char*>(key.c_str())));
    context.data.mv_size = data_size;
    context.data.mv_data = const_cast<uint8_t*>(data);

#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager write_encoding key", context.key);
print_mdb_val("hash_data_manager write_encoding data", context.data);
#endif

    int rc = mdb_cursor_put(context.cursor, &context.key, &context.data,
                            MDB_NODUPDATA);
    if (rc != 0) {
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
  }


  // overwrite the record at the cursor, try to optimize
  void overwrite_encoding(hashdb::lmdb_context_t& context,
                          const std::string& key,
                          const uint8_t* const data, const size_t data_size) {

    if (context.key.mv_size == key.size() &&
                                    context.data.mv_size == data_size) {
      // replace in place
      context.key.mv_data = static_cast<uint8_t*>(
             static_cast<void*>(const_cast<char*>(key.c_str())));
      context.data.mv_data = const_cast<uint8_t*>(data);
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager overwriting key", context.key);
print_mdb_val("hash_data_manager overwriting data", context.data);
#endif
      int rc = mdb_cursor_put(context.cursor, &context.key, &context.data,
                              MDB_CURRENT);
      if (rc != 0) {
        std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
        assert(0);
      }

    } else {

#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager deleting key", context.key);
print_mdb_val("hash_data_manager deleting data", context.data);
#endif
      // delete record at cursor
      int rc = mdb_cursor_del(context.cursor, 0);
      if (rc != 0) {
        std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
        assert(0);
      }

      // write replacement record
      write_encoding(context, key, data, data_size);
    }
  }

  // parse Type 1 context.data into these parameters
  void decode_type1(hashdb::lmdb_context_t& context,
                    uint64_t& source_id,
                    float& entropy,
                    std::string& block_label,
                    uint64_t& sub_count,
                    file_offsets_t& file_offsets
                   ) const {

#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager decode_type1 key", context.key);
print_mdb_val("hash_data_manager decode_type1 data", context.data);
#endif
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

  // parse Type 2 context.data into these parameters
  void decode_type2(hashdb::lmdb_context_t& context,
                    float& entropy,
                    std::string& block_label,
                    uint64_t& count,
                    uint64_t& count_stored) const {

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

    // read count_stored
    p = lmdb_helper::decode_uint64_t(p, count_stored);

    // read must align to data record
    if (p != p_start + context.data.mv_size) {
      std::cerr << "data decode error in LMDB hash data store\n";
      assert(0);
    }
  }

  // parse Type 3 context.data into these parameters
  void decode_type3(hashdb::lmdb_context_t& context,
                    uint64_t& source_id,
                    uint64_t& sub_count,
                    file_offsets_t& file_offsets) const {

#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager decode_type3 key", context.key);
print_mdb_val("hash_data_manager decode_type3 data", context.data);
#endif
    // prepare to read Type 3 entry:
    const uint8_t* const p_start = static_cast<uint8_t*>(context.data.mv_data);
    const uint8_t* p = p_start;

    // source ID
    p = lmdb_helper::decode_uint64_t(p, source_id);

    // sub_count
    file_offsets.clear();
    sub_count = decode_file_offsets(p, p_start + context.data.mv_size,
                                    file_offsets);
  }

  // ************************************************************
  // insert into type
  // ************************************************************
  // new Type 1
  uint64_t insert_new_type1(hashdb::lmdb_context_t& context,
                            const std::string& block_hash,
                            const uint64_t source_id,
                            const float entropy,
                            const std::string& block_label,
                            const uint64_t file_offset,
                            hashdb::lmdb_changes_t& changes) {

    // get the set of addable file offsets
    file_offsets_t addable_file_offsets;
    const size_t sub_count_stored = insert_file_offset(file_offset,
                                           0, addable_file_offsets, changes);

    // encode Type 1
    uint8_t data[max_lmdb_data_size];
    size_t data_size = encode_type1(source_id, entropy, block_label,
                                    1, // sub_count
                                    addable_file_offsets, data);
    // write Type 1
    write_encoding(context, block_hash, data, data_size);

    // log changes
    ++changes.hash_data_source_inserted;
    changes.hash_data_offset_inserted += sub_count_stored;

    // return new total count
    return 1;
  }

  // updated Type 1
  // cursor must be at Type 1
  uint64_t insert_update_type1(hashdb::lmdb_context_t& context,
                               const std::string& block_hash,
                               const uint64_t source_id,
                               const float entropy,
                               const std::string& block_label,
                               const uint64_t file_offset,
                               hashdb::lmdb_changes_t& changes) {

    // read the existing Type 1 entry into existing_* fields:
    uint64_t existing_source_id;
    float existing_entropy;
    std::string existing_block_label;
    uint64_t existing_sub_count;
    file_offsets_t existing_file_offsets;
    decode_type1(context, existing_source_id,
                 existing_entropy, existing_block_label,
                 existing_sub_count, existing_file_offsets);

    // note if metadata differs
    if (metadata_differs(entropy, existing_entropy,
                         block_label, existing_block_label)) {
      ++changes.hash_data_data_changed;
    }

    // add new file_offset to existing offsets
    const size_t sub_count_stored = insert_file_offset(file_offset,
             existing_file_offsets.size(), existing_file_offsets, changes);

    // replace Type 1 at cursor
    uint64_t new_sub_count = existing_sub_count + 1;
    uint8_t data[max_lmdb_data_size];
    size_t data_size = encode_type1(source_id, entropy, block_label,
                                new_sub_count, existing_file_offsets, data);
    overwrite_encoding(context, block_hash, data, data_size);

    // log changes
    changes.hash_data_offset_inserted += sub_count_stored;

    // return new total count
    return new_sub_count;
  }

  // new Type 2 and two new Type 3 from old Type 1
  // cursor must be at Type 1
  uint64_t insert_new_type2(hashdb::lmdb_context_t& context,
                            const std::string& block_hash,
                            const uint64_t source_id,
                            const float entropy,
                            const std::string& block_label,
                            const uint64_t file_offset,
                            hashdb::lmdb_changes_t& changes) {

    // read the existing Type 1 entry into existing_* fields:
    uint64_t existing_source_id;
    float existing_entropy;
    std::string existing_block_label;
    uint64_t existing_sub_count;
    file_offsets_t existing_file_offsets;
    decode_type1(context, existing_source_id,
                 existing_entropy, existing_block_label,
                 existing_sub_count, existing_file_offsets);

    // note if metadata differs
    if (metadata_differs(entropy, existing_entropy,
                         block_label, existing_block_label)) {
      ++changes.hash_data_data_changed;
    }

    // new set of addable file offsets for the new source
    file_offsets_t addable_file_offsets;
    size_t sub_count_stored = insert_file_offset(file_offset,
             existing_file_offsets.size(), addable_file_offsets, changes);

    // replace Type 1 with Type 2 and write back two new Type 3
    uint64_t new_count = existing_sub_count + 1;
    uint8_t data[max_lmdb_data_size];
    size_t data_size;
    data_size = encode_type2(entropy, block_label, new_count,
                    existing_file_offsets.size() + sub_count_stored, data);
    overwrite_encoding(context, block_hash, data, data_size);
    data_size = encode_type3(existing_source_id, existing_sub_count,
                             existing_file_offsets, data);
    write_encoding(context, block_hash, data, data_size);
    data_size = encode_type3(source_id,
                             1, // sub_count
                             addable_file_offsets, data);
    write_encoding(context, block_hash, data, data_size);

    // log changes
    ++changes.hash_data_source_inserted;
    changes.hash_data_offset_inserted += sub_count_stored;

    // return new total count
    return new_count;
  }

  // new Type 3
  // cursor must be at Type 2
  uint64_t insert_new_type3(hashdb::lmdb_context_t& context,
                            const std::string& block_hash,
                            const uint64_t source_id,
                            const float entropy,
                            const std::string& block_label,
                            const uint64_t file_offset,
                            hashdb::lmdb_changes_t& changes) {

    // read the existing Type 2 entry into existing_* fields
    float existing_entropy;
    std::string existing_block_label;
    uint64_t existing_count;
    uint64_t existing_count_stored;
    decode_type2(context, existing_entropy, existing_block_label,
                                 existing_count, existing_count_stored);

    // note if metadata differs
    if (metadata_differs(entropy, existing_entropy, block_label,
                                                   existing_block_label)) {
      ++changes.hash_data_data_changed;
    }

    // get the set of addable file offsets
    file_offsets_t addable_file_offsets;
    size_t sub_count_stored = insert_file_offset(file_offset,
                  existing_count_stored, addable_file_offsets, changes);

    // write back the updated Type 2 entry
    uint64_t new_count = existing_count + 1;
    uint8_t data[max_lmdb_data_size];
    size_t data_size;
    data_size = encode_type2(entropy, block_label, new_count,
                             existing_count_stored + sub_count_stored, data);
    overwrite_encoding(context, block_hash, data, data_size);

    // write the new Type 3 entry
    data_size = encode_type3(source_id, 1, // sub_count
                             addable_file_offsets, data);
    write_encoding(context, block_hash, data, data_size);

    // log changes
    ++changes.hash_data_source_inserted;
    changes.hash_data_offset_inserted += sub_count_stored;

    // return new total count
    return new_count;
  }

  // updated Type 3
  // cursor must be at Type 3 where source ID matched
  uint64_t insert_update_type3(hashdb::lmdb_context_t& context,
                               const std::string& block_hash,
                               const uint64_t source_id,
                               const float entropy,
                               const std::string& block_label,
                               const uint64_t file_offset,
                               hashdb::lmdb_changes_t& changes) {

    // read the existing Type 3 entry into existing_* fields
    uint64_t existing_source_id;
    uint64_t existing_sub_count;
    file_offsets_t existing_file_offsets;
    decode_type3(context, existing_source_id, existing_sub_count,
                 existing_file_offsets);

    // validate usage
    if (source_id != existing_source_id) {
      assert(0);
    }

    // move the cursor back to the Type 2 entry
    cursor_to_first_current(context);

    // read the existing Type 2 entry into existing2_* fields
    float existing2_entropy;
    std::string existing2_block_label;
    uint64_t existing2_count;
    uint64_t existing2_count_stored;
    decode_type2(context, existing2_entropy, existing2_block_label,
                                   existing2_count, existing2_count_stored);

    // note if metadata differs
    if (metadata_differs(entropy, existing2_entropy, block_label,
                                                   existing2_block_label)) {
      ++changes.hash_data_data_changed;
    }

    // add file_offset to existing_file_offsets
    size_t sub_count_stored = insert_file_offset(file_offset,
             existing2_count_stored, existing_file_offsets, changes);

    // replace the updated Type 2 entry
    uint64_t new_count = existing2_count + 1;
    uint8_t data[max_lmdb_data_size];
    size_t data_size;
    data_size = encode_type2(entropy, block_label, new_count,
                             existing2_count_stored + sub_count_stored, data);
    overwrite_encoding(context, block_hash, data, data_size);

    // move cursor forward to the correct Type 3 entry
    bool worked = cursor_to_type3(context, source_id);
    if (!worked) {
      // program error
      assert(0);
    }

    // replace the updated Type 3 entry
    data_size = encode_type3(source_id, existing_sub_count + 1,
                            existing_file_offsets, data);
    overwrite_encoding(context, block_hash, data, data_size);

    // track changes
    changes.hash_data_offset_inserted += sub_count_stored;

    // return new total count
    return new_count;
  }

  // ************************************************************
  // merge into type
  // ************************************************************
  // merge new Type 1
  uint64_t merge_new_type1(hashdb::lmdb_context_t& context,
                           const std::string& block_hash,
                           const uint64_t source_id,
                           const float entropy,
                           const std::string& block_label,
                           const uint64_t sub_count,
                           const file_offsets_t file_offsets,
                           hashdb::lmdb_changes_t& changes) {

    // get the set of addable file offsets
    file_offsets_t addable_file_offsets;
    const size_t sub_count_stored = merge_file_offsets(
                  file_offsets, 0, addable_file_offsets, changes);

    // encode Type 1
    uint8_t data[max_lmdb_data_size];
    size_t data_size = encode_type1(source_id, entropy, block_label,
                                    sub_count, addable_file_offsets, data);
    // write Type 1
    write_encoding(context, block_hash, data, data_size);

    // log changes
    ++changes.hash_data_source_inserted;
    changes.hash_data_offset_inserted += sub_count_stored;

    // return new total count
    return sub_count;
  }

  // updated Type 1
  // cursor must be at Type 1
  uint64_t merge_update_type1(hashdb::lmdb_context_t& context,
                              const std::string& block_hash,
                              const uint64_t source_id,
                              const float entropy,
                              const std::string& block_label,
                              const uint64_t sub_count,
                              hashdb::lmdb_changes_t& changes) {

    // read the existing Type 1 entry into existing_* fields:
    uint64_t existing_source_id;
    float existing_entropy;
    std::string existing_block_label;
    uint64_t existing_sub_count;
    file_offsets_t existing_file_offsets;
    decode_type1(context, existing_source_id,
                 existing_entropy, existing_block_label,
                 existing_sub_count, existing_file_offsets);

    // replace Type 1 at cursor if metadata differs
    if (metadata_differs(entropy, existing_entropy,
                         block_label, existing_block_label)) {

      uint8_t data[max_lmdb_data_size];
      size_t data_size = encode_type1(source_id, entropy, block_label,
                           existing_sub_count, existing_file_offsets, data);
      overwrite_encoding(context, block_hash, data, data_size);
      ++changes.hash_data_data_changed;
    }

    // warn if the offset sub_count values are not equivalent
    if (sub_count != existing_sub_count) {
      changes.hash_data_mismatched_sub_count_detected +=
                  (sub_count > existing_sub_count) ?
                  sub_count - existing_sub_count :
                  existing_sub_count - sub_count;
    }

    // return unchanged existing total count
    return existing_sub_count;
  }

  // new Type 2 and two new Type 3 from old Type 1
  // cursor must be at Type 1
  uint64_t merge_new_type2(hashdb::lmdb_context_t& context,
                           const std::string& block_hash,
                           const uint64_t source_id,
                           const float entropy,
                           const std::string& block_label,
                           const uint64_t sub_count,
                           const file_offsets_t file_offsets,
                           hashdb::lmdb_changes_t& changes) {

    // read the existing Type 1 entry into existing_* fields:
    uint64_t existing_source_id;
    float existing_entropy;
    std::string existing_block_label;
    uint64_t existing_sub_count;
    file_offsets_t existing_file_offsets;
    decode_type1(context, existing_source_id,
                 existing_entropy, existing_block_label,
                 existing_sub_count, existing_file_offsets);

    // note if metadata differs
    if (metadata_differs(entropy, existing_entropy,
                         block_label, existing_block_label)) {
      ++changes.hash_data_data_changed;
    }

    // get the set of addable file offsets
    file_offsets_t addable_file_offsets;
    size_t sub_count_stored = merge_file_offsets(
            file_offsets, existing_file_offsets.size(),
            addable_file_offsets, changes);

    // replace Type 1 with Type 2 and write back two new Type 3
    uint64_t new_count = existing_sub_count + sub_count;
    uint8_t data[max_lmdb_data_size];
    size_t data_size;
    data_size = encode_type2(entropy, block_label, new_count,
                    existing_file_offsets.size() + sub_count_stored, data);
    overwrite_encoding(context, block_hash, data, data_size);
    data_size = encode_type3(existing_source_id, existing_sub_count,
                             existing_file_offsets, data);
    write_encoding(context, block_hash, data, data_size);
    data_size = encode_type3(source_id, sub_count, addable_file_offsets, data);
    write_encoding(context, block_hash, data, data_size);

    // log changes
    ++changes.hash_data_source_inserted;
    changes.hash_data_offset_inserted += sub_count_stored;

    // return new total count
    return new_count;
  }

  // new Type 3
  // cursor must be at Type 2
  uint64_t merge_new_type3(hashdb::lmdb_context_t& context,
                           const std::string& block_hash,
                           const uint64_t source_id,
                           const float entropy,
                           const std::string& block_label,
                           const uint64_t sub_count,
                           const file_offsets_t file_offsets,
                           hashdb::lmdb_changes_t& changes) {

    // read the existing Type 2 entry into existing2_* fields
    float existing2_entropy;
    std::string existing2_block_label;
    uint64_t existing2_count;
    uint64_t existing2_count_stored;
    decode_type2(context, existing2_entropy, existing2_block_label,
                                 existing2_count, existing2_count_stored);

    // note if metadata differs
    if (metadata_differs(entropy, existing2_entropy, block_label,
                                                   existing2_block_label)) {
      ++changes.hash_data_data_changed;
    }

    // get the set of addable file offsets
    file_offsets_t addable_file_offsets;
    size_t sub_count_stored = merge_file_offsets(file_offsets,
                  existing2_count_stored, addable_file_offsets, changes);

    // write back the updated Type 2 entry
    uint64_t new_count = existing2_count + sub_count;
    uint8_t data[max_lmdb_data_size];
    size_t data_size;
    data_size = encode_type2(entropy, block_label, new_count,
                             existing2_count_stored + sub_count_stored, data);
    overwrite_encoding(context, block_hash, data, data_size);

    // write the new Type 3 entry
    data_size = encode_type3(source_id, sub_count, addable_file_offsets, data);
    write_encoding(context, block_hash, data, data_size);

    // log changes
    ++changes.hash_data_source_inserted;
    changes.hash_data_offset_inserted += sub_count_stored;

    // return new total count
    return new_count;
  }

  // updated Type 3
  // cursor must be at Type 3 where source ID matched
  uint64_t merge_update_type3(hashdb::lmdb_context_t& context,
                              const std::string& block_hash,
                              const uint64_t source_id,
                              const float entropy,
                              const std::string& block_label,
                              const uint64_t sub_count,
                              hashdb::lmdb_changes_t& changes) {

    // read the existing Type 3 entry into existing_* fields
    uint64_t existing_source_id;
    uint64_t existing_sub_count;
    file_offsets_t existing_file_offsets;
    decode_type3(context, existing_source_id, existing_sub_count,
                 existing_file_offsets);

    // validate usage
    if (source_id != existing_source_id) {
      assert(0);
    }

    // move the cursor back to the Type 2 entry
    cursor_to_first_current(context);

    // read the existing Type 2 entry into existing2_* fields
    float existing2_entropy;
    std::string existing2_block_label;
    uint64_t existing2_count;
    uint64_t existing2_count_stored;
    decode_type2(context, existing2_entropy, existing2_block_label,
                                   existing2_count, existing2_count_stored);

    // note if metadata differs
    if (metadata_differs(entropy, existing2_entropy, block_label,
                                                   existing2_block_label)) {

      // replace the updated Type 2 entry
      uint8_t data[max_lmdb_data_size];
      size_t data_size;
      data_size = encode_type2(entropy, block_label, existing2_count,
                               existing2_count_stored, data);
      overwrite_encoding(context, block_hash, data, data_size);
      ++changes.hash_data_data_changed;


    }

    // the sub_count values should be equivalent
    if (sub_count != existing_sub_count) {
      // warn if the offset sub_count values are not equivalent
      changes.hash_data_mismatched_sub_count_detected +=
                  (sub_count > existing_sub_count) ?
                  sub_count - existing_sub_count :
                  existing_sub_count - sub_count;
    }

    // return the unchanged existing total count
    return existing2_count;
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
       max_sub_count((p_max_sub_count > max_lmdb_sub_count)
                      ? max_lmdb_sub_count : p_max_sub_count),
       env(lmdb_helper::open_env(hashdb_dir + "/lmdb_hash_data_store",
                                                                file_mode)),
       M() {

    MUTEX_INIT(&M);

    // require valid parameters
    if (byte_alignment == 0) {
      std::cerr << "invalid hash data store configuration\n";
      assert(0);
    }
    if (p_max_sub_count > max_lmdb_sub_count) {
      std::cerr << "Invalid hash data store configuration: max_sub_count "
                << p_max_sub_count << " cannot be greater than "
                << max_lmdb_sub_count << " and is truncated.\n";
    }
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
   * Insert hash with accompanying data.  Overwrite data if there
   * and changed.  Return updated source count.
   *
   * Use when inserting a new file offset.  Changes updated if
   * the file offset for the given source ID already exists.
   */
  size_t insert(const std::string& block_hash,
                const float entropy,
                const std::string& block_label,
                const uint64_t source_id,
                const uint64_t file_offset,
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

    // require that the provided file_offset is valid
    if (file_offset % byte_alignment != 0) {
      // invalid file offset so warn and do not add anything
      std::stringstream ss;
      ss << "Usage error: file offset " << file_offset
         << " does not fit evenly along step size " << byte_alignment
         << ".  Insert request aborted.\n";
      tprint(std::cerr, ss.str());
      return 0;
    }

    // warn if block_label will get truncated
    if (block_label.size() > max_lmdb_block_label_size) {
      // invalid file offset so warn and do not add anything
      std::stringstream ss;
      ss << "Invalid block_label length " << block_label.size()
         << " is greater than " << max_lmdb_block_label_size
         << " and is truncated.\n";
      tprint(std::cerr, ss.str());
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
                               source_id, entropy, block_label,
                               file_offset, changes);

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
        uint64_t existing_source_id;
        decode_source_id(context, existing_source_id);

        if (source_id == existing_source_id) {
          // update Type 1
          count = insert_update_type1(context, block_hash,
                                      source_id, entropy, block_label,
                                      file_offset, changes);

        } else {
          // new Type 2 and two new Type 3
          count = insert_new_type2(context, block_hash,
                                   source_id, entropy, block_label,
                                   file_offset, changes);
        }
      } else {

        // existing entry is Type 2

        // look for matching Type 3
        bool worked = cursor_to_type3(context, source_id);
        if (worked) {
          // update Type 2 and update Type 3
          count = insert_update_type3(context, block_hash,
                                      source_id, entropy, block_label,
                                      file_offset, changes);
        } else {
          // update Type 2 and insert new Type 3
          count = insert_new_type3(context, block_hash,
                                   source_id, entropy, block_label,
                                   file_offset, changes);
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
    if (block_label.size() > max_lmdb_block_label_size) {
      // invalid file offset so warn and do not add anything
      std::stringstream ss;
      ss << "Invalid block_label length " << block_label.size()
         << " is greater than " << max_lmdb_block_label_size
         << " and is truncated.\n";
      tprint(std::cerr, ss.str());
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
                              source_id, entropy, block_label,
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
                                     source_id, entropy, block_label,
                                     sub_count, changes);

        } else {
          // new Type 2 and two new Type 3
          count = merge_new_type2(context, block_hash,
                                  source_id, entropy, block_label,
                                  sub_count, file_offsets, changes);
        }
      } else {

        // existing entry is Type 2

        // look for matching Type 3
        bool worked = cursor_to_type3(context, source_id);
        if (worked) {
          // update Type 2 and update Type 3
          count = merge_update_type3(context, block_hash,
                                     source_id, entropy, block_label,
                                     sub_count, changes);
        } else {
          // update Type 2 and merge new Type 3
          count = merge_new_type3(context, block_hash,
                                  source_id, entropy, block_label,
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
   * Read data for the hash.  False if the hash does not exist.
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
        decode_type1(context, source_id, entropy, block_label,
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
        decode_type2(context, entropy, block_label, count, count_stored);

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
        uint64_t count_stored;
        decode_type2(context, entropy, block_label, count, count_stored);

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

