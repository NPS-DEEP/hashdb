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
 * Manage the LMDB source name multimap of key=binary_file_hash,
 * value=(repository name, filename).
 *
 * Lock non-thread-safe interfaces before use.
 */

#ifndef LMDB_SOURCE_NAME_MANAGER_HPP
#define LMDB_SOURCE_NAME_MANAGER_HPP
#include "file_modes.h"
#include "lmdb.h"
#include "lmdb_helper.h"
#include "lmdb_context.hpp"
#include "lmdb_data_codec.hpp"
#include "hashdb.hpp"
#include <vector>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <string>

class lmdb_source_name_manager_t {

  private:
  std::string hashdb_dir;
  file_mode_type_t file_mode;
  MDB_env* env;

  // do not allow copy or assignment
  lmdb_source_name_manager_t(const lmdb_source_name_manager_t&);
  lmdb_source_name_manager_t& operator=(const lmdb_source_name_manager_t&);

  // reader
  void find_array_at_cursor(lmdb_context_t& context,
                            hashdb::source_names_t& source_names) const {

    // find data for hash starting at cursor
    std::string file_binary_hash = lmdb_helper::get_string(context.key);
    std::string encoding = lmdb_helper::get_string(context.data);

    // add to array
    source_names.push_back(lmdb_data_codec::decode_ss_t_data(encoding));

    // move cursor forward to find more data for this hash
    int rc = 0;
    while (true) {

      // set cursor to next key, data pair
      rc = mdb_cursor_get(context.cursor,
                          &context.key, &context.data, MDB_NEXT);

      if (rc == 0) {

        // read next data
        std::string next_file_binary_hash =
                                    lmdb_helper::get_string(context.key);
        std::string next_encoding = lmdb_helper::get_string(context.data);

        if (next_file_binary_hash == file_binary_hash) {
          // same file hash so use it
          source_names.push_back(lmdb_data_codec::decode_ss_t_data(
                                 next_encoding));

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
  lmdb_source_name_manager_t(const std::string& p_hashdb_dir,
                            const file_mode_type_t p_file_mode) :
       hashdb_dir(p_hashdb_dir),
       file_mode(p_file_mode),
       env(lmdb_helper::open_env(hashdb_dir + "/lmdb_source_name_store",
                                                            file_mode)) {
  }

  ~lmdb_source_name_manager_t() {
    // close the lmdb_hash_store DB environment
    mdb_env_close(env);
  }

  /**
   * Insert key, value pair unless the pair is already there.
   */
  void insert(const std::string file_binary_hash,
              const std::string repository_name,
              const std::string filename) {

    // maybe grow the DB
    lmdb_helper::maybe_grow(env);

    // get context
    lmdb_context_t context(env, true, true); // writable, duplicates
    context.open();

    // set key
    lmdb_helper::point_to_string(file_binary_hash, context.key);

    // set data
    std::string encoding = lmdb_data_codec::encode_ss_t_data(
                                                 repository_name, filename);
    lmdb_helper::point_to_string(encoding, context.data);

    // insert unless key, data pair is already there
    int rc = mdb_put(context.txn, context.dbi,
                     &context.key, &context.data, MDB_NODUPDATA);

    if (rc == 0 || rc == MDB_KEYEXIST) {
      // data inserted or data already there
      context.close();
      return;
    }

    // invalid rc
    std::cerr << "label manager rc " << mdb_strerror(rc) << "\n";
    assert(0);
  }

  /**
   * Clear id_offset_pairs then populate it with matches.
   * Empty response means no match.
   */
  void find(const std::string& file_binary_hash,
            hashdb::source_names_t& source_names) const {

    // clear existing source names vector
    source_names.clear();

    // get context
    lmdb_context_t context(env, false, true); // read only, duplicates
    context.open();

    // set key
    lmdb_helper::point_to_string(file_binary_hash, context.key);

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
      find_array_at_cursor(context, source_names);
      context.close();
      return;
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

