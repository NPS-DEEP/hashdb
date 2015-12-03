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

#ifndef LMDB_HASH_MANAGER_HPP
#define LMDB_HASH_MANAGER_HPP
#include "globals.hpp"
#include "file_modes.h"
#include "hashdb_settings.hpp"
#include "hashdb_settings_store.hpp"
#include "lmdb.h"
#include "lmdb_typedefs.h"
#include "lmdb_helper.h"
#include "lmdb_context.hpp"
#include "lmdb_data_codec.hpp"
#include "bloom_filter_manager.hpp"
#include "hashdb_changes.hpp"
#include <vector>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <string>

class lmdb_hash_manager_t {

  private:
  std::string hashdb_dir;
  file_mode_type_t file_mode;
  hashdb_settings_t settings;
  bloom_filter_manager_t bloom_filter_manager;
  MDB_env* env;

  // do not allow copy or assignment
  lmdb_hash_manager_t(const lmdb_hash_manager_t&);
  lmdb_hash_manager_t& operator=(const lmdb_hash_manager_t&);

  // reader
  void find_array_at_cursor(lmdb_context_t& context,
                            id_offset_pairs_t& id_offset_pairs) const {

    // find data for hash starting at cursor
    std::string binary_hash = lmdb_helper::get_string(context.key);
    std::string encoding = lmdb_helper::get_string(context.data);
    std::pair<uint64_t, uint64_t> pair =
                        lmdb_data_codec::decode_uint64_uint64_data(encoding);

    // convert offset index to byte offset
    pair.second *= settings.sector_size;

    // add to array
    id_offset_pairs.push_back(pair);

    // move cursor forward to find more data for this hash
    int rc = 0;
    while (true) {

      // set cursor to next key, data pair
      rc = mdb_cursor_get(context.cursor,
                          &context.key, &context.data, MDB_NEXT);

      if (rc == 0) {

        // read next data
        std::string next_binary_hash = lmdb_helper::get_string(context.key);
        std::string next_encoding = lmdb_helper::get_string(context.data);

        if (next_binary_hash == binary_hash) {
          // same hash so use it
          std::pair<uint64_t, uint64_t> next_pair =
                                  lmdb_data_codec::decode_uint64_uint64_data(
                                                                next_encoding);

          // convert offset index to byte offset
          next_pair.second *= settings.sector_size;

          // add to array
          id_offset_pairs.push_back(next_pair);

        } else {
          // different hash so done
          break;
        }
      } else {
        break;
      }
    }

    if (rc != 0 && rc != MDB_NOTFOUND) {
      // bad state
      std::cerr << "LMDB find error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
  }

  public:
  lmdb_hash_manager_t(const std::string& p_hashdb_dir,
                      const file_mode_type_t p_file_mode) :
       hashdb_dir(p_hashdb_dir),
       file_mode(p_file_mode),
       settings(hashdb_settings_t(hashdb_settings_store_t::read_settings(
                                                               hashdb_dir))),
       bloom_filter_manager(hashdb_dir,
                            file_mode,
                            settings.bloom_is_used,
                            settings.bloom_M_hash_size,
                            settings.bloom_k_hash_functions),
       env(lmdb_helper::open_env(hashdb_dir + "/lmdb_hash_store", file_mode)) {
  }

  ~lmdb_hash_manager_t() {
    // close the lmdb_hash_store DB environment
    mdb_env_close(env);
  }

  /**
   * Insert hash data list.  Log insertion changes.
   */
  void insert(const uint64_t source_id,
              const hash_data_list_t& hash_data_list,
              hashdb_changes_t& changes) {

    // insert each hash_data entry
    for(hash_data_list_t::const_iterator it=hash_data_list.begin();
                                       it != hash_data_list.end(); ++it) {

      // validate the byte alignment
      if (it->file_offset % settings.sector_size != 0) {
        ++changes.hashes_not_inserted_invalid_sector_size;
        continue;
      }
      size_t offset_index = it->file_offset / settings.sector_size;

      // maybe grow the DB
      lmdb_helper::maybe_grow(env);

      // get context
      lmdb_context_t context(env, true, true);
      context.open();

      // set key
      lmdb_helper::point_to_string(it->binary_hash, context.key);

      // set data
      std::string encoding = lmdb_data_codec::encode_uint64_uint64_data(
                                                    source_id, offset_index);
      lmdb_helper::point_to_string(encoding, context.data);

      // see if this entry exists yet
      // set the cursor to this key,data pair
      int rc = mdb_cursor_get(context.cursor,
                              &context.key, &context.data,
                              MDB_GET_BOTH);
      bool has_pair = false;
      if (rc == 0) {
        has_pair = true;
      } else if (rc == MDB_NOTFOUND) {
        // not found
      } else {
        // program error
        has_pair = false; // satisfy mingw32-g++ compiler
        std::cerr << "LMDB insert error: " << mdb_strerror(rc) << "\n";
        assert(0);
      }
 
      if (has_pair) {
        // this exact entry already exists
        ++changes.hashes_not_inserted_duplicate_element;
        context.close();
        continue;
      }

      // insert the entry since all the checks passed
      rc = mdb_put(context.txn, context.dbi,
                     &context.key, &context.data, MDB_NODUPDATA);
      if (rc != 0) {
        std::cerr << "LMDB insert error: " << mdb_strerror(rc) << "\n";
        assert(0);
      }
      ++changes.hashes_inserted;

      context.close();

      // add hash to bloom filter, too, even if already there
      bloom_filter_manager.add_hash_value(it->binary_hash);
    }
  }






  /**
   * Clear id_offset_pairs then populate it with matches.
   * Empty response means no match.
   */
  void find(const std::string& binary_hash,
                         id_offset_pairs_t& id_offset_pairs) const {

    // clear any existing (source ID, file offset) pairs
    id_offset_pairs.clear();

    // get context
    lmdb_context_t context(env, false, true);
    context.open();

    // set key
    lmdb_helper::point_to_string(binary_hash, context.key);

    // set the cursor to this key
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    if (rc == MDB_NOTFOUND) {
      // no hash
      context.close();
      return;
    }

    if (rc == 0) {
      // one or more hashes
      find_array_at_cursor(context, id_offset_pairs);
      context.close();
      return;
    }

    // invalid rc
    std::cerr << "LMDB find error: " << mdb_strerror(rc) << "\n";
    assert(0);
  }

  /**
   * Return first hash and its matches.
   */
  std::string find_begin(id_offset_pairs_t& id_offset_pairs) const {

    // clear any existing (source ID, file offset) pairs
    id_offset_pairs.clear();

    // get context
    lmdb_context_t context(env, false, true);
    context.open();

    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_FIRST);

    if (rc == MDB_NOTFOUND) {
      // no values for this hash
      context.close();
      return std::string("");
    }

    if (rc == 0) {
      // get this hash
      std::string binary_hash = lmdb_helper::get_string(context.key);

      // one or more values for this hash
      find_array_at_cursor(context, id_offset_pairs);
      context.close();
      return binary_hash;
    }

    // invalid rc
    std::cerr << "LMDB find_begin error: " << mdb_strerror(rc) << "\n";
    assert(0);
  }

  /**
   * Return next hash and its matches or "" and no pairs.
   * Empty response means no next.
   */
  std::string find_next(const std::string& last_binary_hash,
                         id_offset_pairs_t& id_offset_pairs) const {

    if (last_binary_hash == "") {
      // program error to ask for next when at end
      std::cerr << "find_next: already at end\n";
      assert(0);
    }

    // clear any existing (source ID, file offset) pairs
    id_offset_pairs.clear();

    // get context
    lmdb_context_t context(env, false, true);
    context.open();

    // set the cursor to last hash
    lmdb_helper::point_to_string(last_binary_hash, context.key);
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    // the last hash must exist
    if (rc != 0) {
      std::cerr << "LMDB find_next error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    // move cursor to this hash
    rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                        MDB_NEXT_NODUP);

    if (rc == MDB_NOTFOUND) {
      // no values for this hash
      context.close();
      return std::string("");
    }

    if (rc == 0) {
      // get this hash
      std::string binary_hash = lmdb_helper::get_string(context.key);

      // one or more values for this hash
      find_array_at_cursor(context, id_offset_pairs);
      context.close();
      return binary_hash;
    }

    // invalid rc
    std::cerr << "LMDB find error: " << mdb_strerror(rc) << "\n";
    assert(0);
  }

  // call this from a lock to prevent getting an unstable answer.
  size_t size() const {
    return lmdb_helper::size(env);
  }
};

#endif

