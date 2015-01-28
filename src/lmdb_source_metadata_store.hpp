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
 * Provides source metadata lookup using LMDB.
 *
 * Locks are required around contexts that can write to preserve
 * integrity, in particular to allow grow.
 *
 * It is always a program error to supply an invalid source lookup index.
 */

#ifndef LMDB_SOURCE_METADATA_STORE_HPP
#define LMDB_SOURCE_METADATA_STORE_HPP
#include "file_modes.h"
#include <string>
#include "lmdb.h"

// no concurrent writes
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "mutex_lock.hpp"

// enable debug
// #define DEBUG 1

class lmdb_source_metadata_store_t {

  private:
  const std::string hashdb_dir;
  const file_mode_type_t file_mode;
  MDB_env* env;

#ifdef HAVE_PTHREAD
  mutable pthread_mutex_t M;                  // mutext
#else
  mutable int M;                              // placeholder
#endif

  // disallow these
  lmdb_source_metadata_store_t(const lmdb_source_metadata_store_t&);
  lmdb_source_metadata_store_t& operator=(const lmdb_source_metadata_store_t&);

  bool add(uint64_t source_lookup_index,
              const lmdb_source_metatata_t& new_source_metadata) {

    MUTEX_LOCK(&M);

    // maybe grow the DB
    lmdb_helper::maybe_grow(env);

    // get context
    lmdb_context_t context(env, true, false);
    context.open();

    // set source lookup index pointer
    point_to_uint64_t(source_lookup_index, context.key);

    // read any existing metadata
    int rc = mdb_cursor_get(context.cursor, &context.key. &context.data,
                            MDB_SET_KEY);

    if (rc == 0) {
      lmdb_source_metadata_t existing_metadata(context.data);
      bool added_more = existing_metadata.add(new_source_metadata
                                              repository_name, filename);
      if (added_more) {
        // replace the record with the fuller one
        // delete the existing record
        rc = mdb_cursor_del(context.cursor, 0);
        if (rc == 0) {
          // good, put in the fuller record
          int rc = mdb_put(context.txn, context.dbi,
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
      int rc = mdb_put(context.txn, context.dbi,
                       &context.key, &context.data, MDB_NODUPDATA);
      if (rc != 0) {
        std::cerr << "put new error: " << mdb_strerror(rc) << "\n";
        assert(0);
      }
    } else {
      std::cerr << "get error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
    MUTEX_UNLOCK(&M);
  }

  public:
  struct iteration_fields_t {
    uint64_t source_lookup_index;
    lmdb_source_metadata_t source_metadata;
    bool is_valid;
    iteration_fields_t(uint64_t p_source_lookup_index,
                       lmdb_source_metadata_t p_source_metadata,
                       bool p_is_valid) :
              source_lookup_index(p_source_lookup_index), 
              source_metadata(p_source_metadata),
              is_valid(p_is_valid) {
    }
  };

  lmdb_source_metadata_store_t (const std::string p_hashdb_dir,
                     file_mode_type_t p_file_mode_type) :

       hashdb_dir(p_hashdb_dir),
       file_mode(p_file_mode_type),
       env(),
       M() {

    MUTEX_INIT(&M);

    // the DB stage directory
    const std::string store_dir = hashdb_dir + "/lmdb_source_metadata_store";

    // open the DB environment
    env = open_env(store_dir, file_mode);
  }

  ~lmdb_source_metadata_store_t() {

    // close the MDB environment
    mdb_env_close(env);
  }

  // add repository name and filename
  bool add_repository_name_filename(uint64_t source_lookup_index,
              const std::string& repository_name,
              const std::string& filename) {

    lmdb_source_metadata_t new_source_metadata;
    new_source_metadata.add_repository_name_filename(repository_name, filename);
    return add(new_source_metadata);
  }

  // add filesize and file hashdigest
  bool add_filesize_hashdigest(uint64_t source_lookup_index,
              const std::string& filesize,
              const std::string& hashdigest) {

    lmdb_source_metadata_t new_source_metadata;
    new_source_metadata.add_fielsize_hashdigest(repository_name, filename);
    return add(new_source_metadata);
  }

  lmdb_source_metadata_t find(uint64_t source_lookup_index) const {

    // get context
    lmdb_context_t context(env, false, true);
    context.open();

    // set source lookup index pointer
    point_to_uint64_t(source_lookup_index, context.key);

    // read any existing metadata
    int rc = mdb_cursor_get(context.cursor, &context.key. &context.data,
                            MDB_SET_KEY);

    lmdb_source_metadata_t existing_metadata;
    if (rc == 0) {
      // read the metadata
      source_metadata = lmdb_source_metadata_t(context.data);
    } else {
      // error if key does not exist
      std::cerr << "no key\n";
      assert(0);
    }

    // close context
    context.close();

    return source_metadata;
  }
 
  iteration_fields_t find_first() const {

    // get context
    lmdb_context_t context(env, false, true);
    context.open();

    iteration_fields_t result;
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_FIRST);
    bool has_first;
    if (rc == 0) {
      has_first = true;
    } else if (rc == MDB_NOTFOUND) {
      has_first = false;
    } else {
      // program error
      has_first = false;
      std::cerr << "rc: " << rc << ", begin: " << is_begin << "\n";
      assert(0);
    }

    iteration_fields_t result(lmdb_helper::get_uint64_t(context.key),
            lmdb_source_metadata_store_t::lmdb_source_metadata_t(context.data),
            has_first);

    // close context
    context.close();

    return result;
  }

  /**
   * Find entry just after this one.
   */
  iteration_fields_t find_next(uint64_t source_lookup_index) const {

    // get context
    lmdb_context_t context(env, false, true);
    context.open();

    // set encoding pointer
    point_to_uint64_t(encoding, context.key);

    // set the cursor to this key, which must exist
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET);
    if (rc != 0) {
      // invalid usage
      assert(0);
    }

    // set cursor to next key, data pair
    rc = mdb_cursor_get(resources->cursor,
                        &resources->key, &resources->data, MDB_NEXT);

    bool has_next;
    if (rc == 0) {
      has_next = true;
    } else if (rc == MDB_NOTFOUND) {
      has_next = false;
    } else {
      // program error
      has_next = false;
      std::cerr << "rc: " << rc << ", begin: " << is_begin << "\n";
      assert(0);
    }

    iteration_fields_t result(lmdb_helper::get_uint64_t(context.key),
            lmdb_source_metadata_store_t::lmdb_source_metadata_t(context.data),
            has_first);

    // close context
    context.close();

    return result;
  }

  // size
  size_t size() const {
    return lmdb_helper::size(env);
  }
};

#endif

