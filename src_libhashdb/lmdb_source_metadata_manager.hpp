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
 * Manage the LMDB source metadata store of key=file_binary_hash,
 * data=(source_id, filesize, positive_count).
 *
 * Lock non-thread-safe interfaces before use.
 */

#ifndef LMDB_SOURCE_METADATA_MANAGER_HPP
#define LMDB_SOURCE_METADATA_MANAGER_HPP
#include "file_modes.h"
#include "lmdb.h"
#include "lmdb_typedefs.h"
#include "lmdb_helper.h"
#include "lmdb_context.hpp"
#include "lmdb_data_codec.hpp"
#include "bloom_filter_manager.hpp"
#include <vector>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <string>

class lmdb_source_metadata_manager_t {

  private:
  std::string hashdb_dir;
  file_mode_type_t file_mode;
  MDB_env* env;

  // do not allow copy or assignment
  lmdb_source_metadata_manager_t(const lmdb_source_metadata_manager_t&);
  lmdb_source_metadata_manager_t& operator=(
                                 const lmdb_source_metadata_manager_t&);

  public:
  lmdb_source_metadata_manager_t(const std::string& p_hashdb_dir,
                            const file_mode_type_t p_file_mode) :
       hashdb_dir(p_hashdb_dir),
       file_mode(p_file_mode),
       env(lmdb_helper::open_env(hashdb_dir + "/lmdb_source_metadata_store",
                                                            file_mode)) {
  }

  ~lmdb_source_metadata_manager_t() {
    // close the lmdb_hash_store DB environment
    mdb_env_close(env);
  }

  /**
   * Insert Begin, key=file_binary_hash, value=0.
   *
   * Return pair(bool, source_id) where true means ready to begin importing
   * block hashes, false if block hashes have already been imported
   * for this source.
   * The source_id value is generated from formula: size()+1.
   */
  std::pair<bool, uint64_t> insert_begin(const std::string& file_binary_hash) {

    // maybe grow the DB
    lmdb_helper::maybe_grow(env);

    // get context
    lmdb_context_t context(env, true, false); // writable, no duplicates
    context.open();

    // set key
    lmdb_helper::point_to_string(file_binary_hash, context.key);

    // set the cursor to this key
    int get_rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    if (get_rc == 0) {
      // source ID for this file binary hash already exists
      // see if block hashes for this source have already been imported
      std::string encoding = lmdb_helper::get_string(context.data);
      lmdb_data_codec::ddd_t ddd =
                           lmdb_data_codec::decode_ddd_t_data(encoding);
      uint64_t source_id = ddd.d1;
      uint64_t filesize = ddd.d2;
      if (filesize == 0) {
        // block hashes are not marked as imported because filesize is 0
        context.close();
        return std::pair<bool, uint64_t>(true, source_id);
      } else {
        // block hashes are already marked as imported because filesize is not 0
        context.close();
        return std::pair<bool, uint64_t>(false, source_id);
      }

    } else if (get_rc == MDB_NOTFOUND) {
      // there is not a source ID for this file binary hash yet so make
      // a new one from DB size + 1
      uint64_t new_source_id = size() + 1;
      std::string encoding = lmdb_data_codec::encode_ddd_t_data(
                                                        new_source_id, 0, 0);
      lmdb_helper::point_to_string(encoding, context.data);

      int insert_rc = mdb_put(context.txn, context.dbi,
                     &context.key, &context.data, MDB_NOOVERWRITE);

      // the insert must work
      if (insert_rc != 0) {
        std::cerr << "source metadata manager insert begin failure "
                  << mdb_strerror(insert_rc) << "\n";
        assert(0);
      }

      // return that insert_start has started
      context.close();
      return std::pair<bool, uint64_t>(true, new_source_id);
    }

    // invalid rc so get failed
    std::cerr << "source metadata manager insert begin get failure "
              << mdb_strerror(get_rc) << "\n";
    assert(0);
  }

  /**
   * Insert End, key=file_binary_hash, value=source_id.
   *
   * Fail if not already there.  Warn to stderr and do not insert
   * if already there and filesize is not zero.
   */
  void insert_end(const std::string& file_binary_hash,
                  const uint64_t source_id, const uint64_t filesize,
                  const uint64_t positive_count) {

    // maybe grow the DB
    lmdb_helper::maybe_grow(env);

    // get context
    lmdb_context_t context(env, true, false); // writable, no duplicates
    context.open();

    // set key
    lmdb_helper::point_to_string(file_binary_hash, context.key);

    // set the cursor to this key
    int get_rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    // fail if not already there
    if (get_rc != 0) {
      std::cerr << "source metadata manager get failure: not started\n"
                << mdb_strerror(get_rc) << "\n";
      assert(0);
    }

    // warn and disregard if key is there and filesize is not zero
    std::string begin_encoding = lmdb_helper::get_string(context.data);
    lmdb_data_codec::ddd_t ddd =
                           lmdb_data_codec::decode_ddd_t_data(begin_encoding);
    uint64_t begin_filesize = ddd.d2;
    if (begin_filesize != 0) {
      std::cerr << "Note: source metadata insert end but data already there\n";
      context.close();
      return;
    }

    // insert end
    std::string encoding = lmdb_data_codec::encode_ddd_t_data(
                                        source_id, filesize, positive_count);
    lmdb_helper::point_to_string(encoding, context.data);
    int insert_rc = mdb_put(context.txn, context.dbi,
                            &context.key, &context.data, 0);

    // the insert must work
    if (insert_rc != 0) {
      std::cerr << "source metadata manager insert end failure "
                << mdb_strerror(insert_rc) << "\n";
      assert(0);
    }

    // insert_end has ended the insertion
    context.close();
    return;
  }

  /**
   * Find source metadata from the file binary hash, fail if no entry.
   */
  source_metadata_t find(const std::string& file_binary_hash) const {

    // get context
    lmdb_context_t context(env, false, false);
    context.open();

    // set key
    lmdb_helper::point_to_string(file_binary_hash, context.key);

    // set the cursor to this key
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    if (rc == 0) {
      // has source key
      std::string encoding = lmdb_helper::get_string(context.data);
      lmdb_data_codec::ddd_t ddd =
                           lmdb_data_codec::decode_ddd_t_data(encoding);
      context.close();
      return source_metadata_t(file_binary_hash, ddd.d1, ddd.d2, ddd.d3);
    }

    // invalid rc
    std::cerr << "LMDB find error: " << mdb_strerror(rc) << "\n";
    std::cerr << "Program error: The file hash was not found.  Aborting.\n";
    assert(0);
  }

  /**
   * Return first file_binary_hash.
   */
  source_metadata_t find_begin() const {

    // get context
    lmdb_context_t context(env, false, true);
    context.open();

    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_FIRST);

    if (rc == MDB_NOTFOUND) {
      // no values for this hash
      context.close();
      return source_metadata_t("", 0, 0, 0);
    }

    if (rc == 0) {
      // get this key
      std::string file_binary_hash = lmdb_helper::get_string(context.key);

      // get this data
      std::string encoding = lmdb_helper::get_string(context.data);
      lmdb_data_codec::ddd_t ddd =
                           lmdb_data_codec::decode_ddd_t_data(encoding);
      context.close();
      return source_metadata_t(file_binary_hash, ddd.d1, ddd.d2, ddd.d3);
    }

    // invalid rc
    std::cerr << "LMDB find_begin error: " << mdb_strerror(rc) << "\n";
    assert(0);
  }

  /**
   * Return next file_binary_hash or data with file_binary_hash set to "".
   * Fail if already at end.
   */
  source_metadata_t find_next(
                       const source_metadata_t last_source_metadata) const {

    if (last_source_metadata.file_binary_hash == "") {
      // program error to ask for next when at end
      std::cerr << "find_next: already at end\n";
      assert(0);
    }

    // get context
    lmdb_context_t context(env, false, true);
    context.open();

    // set the cursor to last entry
    lmdb_helper::point_to_string(
                         last_source_metadata.file_binary_hash, context.key);
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    // the last hash must exist
    if (rc != 0) {
      std::cerr << "LMDB find_next error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    // move cursor to this key
    rc = mdb_cursor_get(context.cursor, &context.key, &context.data, MDB_NEXT);

    if (rc == MDB_NOTFOUND) {
      // at end of DB
      context.close();
      return source_metadata_t("", 0, 0, 0);
    }

    if (rc == 0) {
      // get this key
      std::string file_binary_hash = lmdb_helper::get_string(context.key);

      // get this data
      std::string encoding = lmdb_helper::get_string(context.data);
      lmdb_data_codec::ddd_t ddd =
                           lmdb_data_codec::decode_ddd_t_data(encoding);
      context.close();
      return source_metadata_t(file_binary_hash, ddd.d1, ddd.d2, ddd.d3);
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

