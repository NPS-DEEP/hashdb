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
 * Manage the LMDB source ID store of key=encoding(source_id),
 * data=file_binary_hash.
 *
 * Lock non-thread-safe interfaces before use.
 */

#ifndef LMDB_SOURCE_ID_MANAGER_HPP
#define LMDB_SOURCE_ID_MANAGER_HPP
#include "file_modes.h"
#include "lmdb.h"
#include "lmdb_helper.h"
#include "lmdb_context.hpp"
#include "lmdb_data_codec.hpp"
#include "bloom_filter_manager.hpp"
#include <vector>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <string>

class lmdb_source_id_manager_t {

  private:
  std::string hashdb_dir;
  file_mode_type_t file_mode;
  MDB_env* env;

  // do not allow copy or assignment
  lmdb_source_id_manager_t(const lmdb_source_id_manager_t&);
  lmdb_source_id_manager_t& operator=(const lmdb_source_id_manager_t&);

  public:
  lmdb_source_id_manager_t(const std::string& p_hashdb_dir,
                            const file_mode_type_t p_file_mode) :
       hashdb_dir(p_hashdb_dir),
       file_mode(p_file_mode),
       env(lmdb_helper::open_env(hashdb_dir + "/lmdb_source_id_store",
                                                            file_mode)) {
  }

  ~lmdb_source_id_manager_t() {
    // close the lmdb_hash_store DB environment
    mdb_env_close(env);
  }

  /**
   * Insert key=source_id, value=file_binary_hash, note if already there.
   */
  void insert(uint64_t source_id, const std::string& file_binary_hash) {

    // maybe grow the DB
    lmdb_helper::maybe_grow(env);

    // get context
    lmdb_context_t context(env, true, false); // writable, no duplicates
    context.open();

    // set key
    std::string encoding = lmdb_data_codec::encode_uint64_data(source_id);
    lmdb_helper::point_to_string(encoding, context.key);

    // set data
    lmdb_helper::point_to_string(file_binary_hash, context.data);

    // insert unless key exists, meaning new label will not replace old label
    int rc = mdb_put(context.txn, context.dbi,
                     &context.key, &context.data, MDB_NOOVERWRITE);

    if (rc == 0) {
      context.close();
      return;
    }

    if (rc == MDB_KEYEXIST) {
      std::cerr << "note: source ID " << source_id << " already exists.\n";
      context.close();
      return;
    }

    // invalid rc
    std::cerr << "label manager rc " << mdb_strerror(rc) << "\n";
    assert(0);
  }

  /**
   * Find file binary hash from source ID, fail if no source ID.
   */
  std::string find(uint64_t source_id) const {

    // get context
    lmdb_context_t context(env, false, false);
    context.open();

    // set key
    std::string encoding = lmdb_data_codec::encode_uint64_data(source_id);
    lmdb_helper::point_to_string(encoding, context.key);

    // set the cursor to this key
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    if (rc == 0) {
      // has source ID
      std::string label = lmdb_helper::get_string(context.data);
      context.close();
      return label;
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

