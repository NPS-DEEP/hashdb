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
 * Manage the LMDB hash label store
 *
 * Lock non-thread-safe interfaces before use.
 */

#ifndef LMDB_HASH_LABEL_MANAGER_HPP
#define LMDB_HASH_LABEL_MANAGER_HPP
#include "globals.hpp"
#include "file_modes.h"
#include "lmdb.h"
#include "lmdb_typedefs.h"
#include "lmdb_helper.h"
#include "lmdb_context.hpp"
#include "lmdb_data_codec.hpp"
#include <vector>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <string>

class lmdb_hash_label_manager_t {

  private:
  std::string hashdb_dir;
  file_mode_type_t file_mode;
  MDB_env* env;

  // do not allow copy or assignment
  lmdb_hash_label_manager_t(const lmdb_hash_label_manager_t&);
  lmdb_hash_label_manager_t& operator=(const lmdb_hash_label_manager_t&);

  public:
  lmdb_hash_label_manager_t(const std::string& p_hashdb_dir,
                            const file_mode_type_t p_file_mode) :
       hashdb_dir(p_hashdb_dir),
       file_mode(p_file_mode),
       env(lmdb_helper::open_env(hashdb_dir + "/lmdb_hash_label_store",
                                                            file_mode)) {
  }

  ~lmdb_hash_label_manager_t() {
    // close the lmdb_hash_store DB environment
    mdb_env_close(env);
  }

  /**
   * Insert label for hash unless some label has already been inserted for it.
   */
  void insert(const std::string binary_hash, const std::string entropy_label) {

    // skip if no entropy label
    if (entropy_label == "") {
      return;
    }

    // maybe grow the DB
    lmdb_helper::maybe_grow(env);

    // get context
    lmdb_context_t context(env, true, false); // writable, no duplicates
    context.open();

    // set key
    lmdb_helper::point_to_string(binary_hash, context.key);

    // set data
    lmdb_helper::point_to_string(entropy_label, context.data);

    // insert unless key exists, meaning new label will not replace old label
    int rc = mdb_put(context.txn, context.dbi,
                     &context.key, &context.data, MDB_NOOVERWRITE);

    if (rc == 0 || rc == MDB_KEYEXIST) {
      context.close();
      return;
    }

    // invalid rc
    std::cerr << "label manager rc " << mdb_strerror(rc) << "\n";
    assert(0);
  }

  /**
   * Return label else "".
   */
  std::string find(const std::string& binary_hash) const {

    // get context
    lmdb_context_t context(env, false, false);
    context.open();

    // set key
    lmdb_helper::point_to_string(binary_hash, context.key);

    // set the cursor to this key
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    if (rc == MDB_NOTFOUND) {
      // no hash
      context.close();
      return std::string("");
    }

    if (rc == 0) {
      // has hash
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

