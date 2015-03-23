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
 * Provides source lookup using LMDB.
 *
 * This module is not threadsafe.
 * Locks are required around contexts that can write to preserve
 * integrity, in particular to allow grow.
 *
 * It is always a program error to supply an invalid source lookup index.
 */

#ifndef LMDB_SOURCE_METADATA_STORE_HPP
#define LMDB_SOURCE_METADATA_STORE_HPP
#include "file_modes.h"
#include "lmdb_source_data.hpp"
#include "lmdb_data_codec.hpp"
#include "lmdb_source_it_data.hpp"
#include <string>
#include "lmdb.h"

class lmdb_source_store_t {

  private:
  const std::string hashdb_dir;
  const file_mode_type_t file_mode;
  MDB_env* env;

  // disallow these
  lmdb_source_store_t(const lmdb_source_store_t&);
  lmdb_source_store_t& operator=(const lmdb_source_store_t&);

  public:
  lmdb_source_store_t (const std::string p_hashdb_dir,
                     file_mode_type_t p_file_mode_type) :

       hashdb_dir(p_hashdb_dir),
       file_mode(p_file_mode_type),
       env() {

    // the DB stage directory
    const std::string store_dir = hashdb_dir + "/lmdb_source_store";

    // open the DB environment
    env = lmdb_helper::open_env(store_dir, file_mode);
  }

  ~lmdb_source_store_t() {

    // close the MDB environment
    mdb_env_close(env);
  }

  /**
   * Return true if record is new or record changes.
   */
  bool add(uint64_t source_lookup_index,
              const lmdb_source_data_t& new_source_data) {

    // maybe grow the DB
    lmdb_helper::maybe_grow(env);

    // for validation, get size before
    size_t size_before = lmdb_helper::size(env);

    // get context
    lmdb_context_t context(env, true, false);
    context.open();

    // set key
    std::string key_encoding = lmdb_data_codec::encode_uint64_data(source_lookup_index);
    lmdb_helper::point_to_string(key_encoding, context.key);

    // read any existing data
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    bool changed = false;
    bool added = false;
    if (rc == 0) {
      // there is existing data

      // get existing source data
      std::string encoding = lmdb_helper::get_string(context.data);
      lmdb_source_data_t source_data =
                            lmdb_data_codec::decode_source_data(encoding);

      // add in new source data
      changed = source_data.add(new_source_data);
      if (changed) {
        // replace the record with the fuller one
        // delete the existing record
        rc = mdb_cursor_del(context.cursor, 0);
        if (rc == 0) {
          // more so put in the fuller record
          std::string data_encoding =
                              lmdb_data_codec::encode_source_data(source_data);
          lmdb_helper::point_to_string(data_encoding, context.data);
          rc = mdb_put(context.txn, context.dbi,
                       &context.key, &context.data, MDB_NODUPDATA);
          if (rc != 0) {
            std::cerr << "put error: " << mdb_strerror(rc) << "\n";
            assert(0);
          }

        } else {
          std::cerr << "delete error: " << mdb_strerror(rc) << "\n";
          assert(0);
        }
      } else {
        // value stays the same, so no action
      }
    } else if (rc == MDB_NOTFOUND) {
      // the key and value are new
      added = true;

      // set data
      std::string data_encoding = lmdb_data_codec::encode_source_data(new_source_data);
      lmdb_helper::point_to_string(data_encoding, context.data);
      rc = mdb_put(context.txn, context.dbi,
                   &context.key, &context.data, MDB_NODUPDATA);
      if (rc != 0) {
        std::cerr << "put new error: " << mdb_strerror(rc) << "\n";
        assert(0);
      }

    } else {
      std::cerr << "get error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
    context.close();

    // Don't trust rc, make sure DB really grew
    if (changed) {
      size_t size_after = lmdb_helper::size(env);
      if (size_before != size_after) {
        std::cerr << "source store change error: before: " << size_before
                  << ", after: " << size_after << "\n";
        std::cerr << "Aborting.\n";
        assert(0);
      }
    }
    if (added) {
      size_t size_after = lmdb_helper::size(env);
      if (size_before+1 != size_after) {
        std::cerr << "source store insert error: before: " << size_before
                  << ", after: " << size_after << "\n";
        std::cerr << "Aborting.\n";
        assert(0);
      }
    }

    return added|changed;
  }

  /**
   * Find or fail.
   */
  lmdb_source_data_t find(uint64_t source_lookup_index) const {

    // get context
    lmdb_context_t context(env, false, true);
    context.open();

    // set key
    std::string encoding = lmdb_data_codec::encode_uint64_data(source_lookup_index);
    lmdb_helper::point_to_string(encoding, context.key);

    // read any existing data
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    lmdb_source_data_t source_data;
    if (rc == 0) {
      // read the data
      std::string source_encoding = lmdb_helper::get_string(context.data);
      source_data = lmdb_data_codec::decode_source_data(source_encoding);
    } else {
      // program error
      std::cerr << "find: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    // close context
    context.close();

    return source_data;
  }
 
  lmdb_source_it_data_t find_first() const {

    // get context
    lmdb_context_t context(env, false, true);
    context.open();

    // read first data if it exists
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_FIRST);

    uint64_t source_lookup_index = 0;
    lmdb_source_data_t source_data;
    bool has_first = false;
    if (rc == 0) {
      has_first = true;

      // read the key and data
      std::string index_encoding = lmdb_helper::get_string(context.key);
      source_lookup_index = lmdb_data_codec::decode_uint64_data(index_encoding);
      std::string encoding = lmdb_helper::get_string(context.data);
      source_data = lmdb_data_codec::decode_source_data(encoding);
    } else if (rc == MDB_NOTFOUND) {
      // no data yet
    } else {
      // program error
      has_first = false;
      std::cerr << "find_first: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    // close context
    context.close();

    return lmdb_source_it_data_t(source_lookup_index, source_data, has_first);
  }

  /**
   * Find entry just after this one.
   */
  lmdb_source_it_data_t find_next(uint64_t source_lookup_index) const {

    // get context
    lmdb_context_t context(env, false, true);
    context.open();

    // set key
    std::string encoding = lmdb_data_codec::encode_uint64_data(source_lookup_index);
    lmdb_helper::point_to_string(encoding, context.key);

    // set the cursor to this key, which must exist
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET);
    if (rc != 0) {
      // invalid usage
      std::cerr << "find_next: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    // set cursor to next key, data pair
    rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_NEXT);

    bool has_next;
    lmdb_source_data_t source_data;
    if (rc == 0) {
      has_next = true;
      std::string source_encoding = lmdb_helper::get_string(context.data);
      source_data = lmdb_data_codec::decode_source_data(source_encoding);
    } else if (rc == MDB_NOTFOUND) {
      has_next = false;
    } else {
      // program error
      has_next = false;
      std::cerr << "has_next: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    // close context
    context.close();

    std::string index_encoding = lmdb_helper::get_string(context.key);
    uint64_t next_index = lmdb_data_codec::decode_uint64_data(index_encoding);
    return lmdb_source_it_data_t(next_index, source_data, has_next);
  }

  bool has(uint64_t source_lookup_index) const {

    // get context
    lmdb_context_t context(env, false, true);
    context.open();

    // set key
    std::string encoding = lmdb_data_codec::encode_uint64_data(source_lookup_index);
    lmdb_helper::point_to_string(encoding, context.key);

    // read any existing data
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    bool has_index = false;
    if (rc == 0) {
      has_index = true;
    } else if (rc == MDB_NOTFOUND) {
      // no action
    } else {
      std::cerr << "has: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    // close context
    context.close();

    return has_index;
  }
 
  // size
  size_t size() const {
    return lmdb_helper::size(env);
  }
};

#endif

