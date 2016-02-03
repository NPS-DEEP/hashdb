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
 * Manage the LMDB source data store of: key=source_id,
 * data=(file_binary_hash, filesize, file_type, low_entropy_count)
 * Threadsafe.
 */

#ifndef LMDB_SOURCE_DATA_MANAGER_HPP
#define LMDB_SOURCE_DATA_MANAGER_HPP
#include "file_modes.h"
#include "lmdb.h"
#include "lmdb_helper.h"
#include "lmdb_context.hpp"
#include "lmdb_changes.hpp"
#include "hashdb.hpp"
#include <vector>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <string>
#ifdef DEBUG
#include "to_hex.hpp"
#endif

// no concurrent writes
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "mutex_lock.hpp"

class lmdb_source_data_manager_t {

  private:
  const std::string hashdb_dir;
  const file_mode_type_t file_mode;
  MDB_env* env;
#ifdef HAVE_PTHREAD
  mutable pthread_mutex_t M;                  // mutext
#else
  mutable int M;                              // placeholder
#endif


  // do not allow copy or assignment
  lmdb_source_data_manager_t(const lmdb_source_data_manager_t&);
  lmdb_source_data_manager_t& operator=(const lmdb_source_data_manager_t&);

  // encoder for key=source_id
  std::string encode_key(const uint64_t source_id) const {

    // allocate space for the encoding
    size_t max_size = 10;

    uint8_t encoding[max_size];
    uint8_t* p = encoding;

    // encode each field
    p = lmdb_helper::encode_uint64_t(source_id, p);

    // return encoding
    return std::string(reinterpret_cast<char*>(encoding), (p-encoding));
  }

  // decoder for key=source_id
  void decode_key(const std::string& encoding, uint64_t& source_id) const {
    const uint8_t* p_start = reinterpret_cast<const uint8_t*>(encoding.c_str());
    const uint8_t* p = p_start;

    p = lmdb_helper::decode_uint64_t(p, source_id);

    // validate that the decoding was properly consumed
    if ((size_t)(p - p_start) != encoding.size()) {
      std::cerr << "decode failure: " << &p << " is not " << &p_start << "\n";
      assert(0);
    }
  }

  // encoder for data=file_binary_hash, filesize, file_type, low_entropy_count
  std::string encode_data(const std::string& file_binary_hash,
                          const uint64_t filesize,
                          const std::string& file_type,
                          const uint64_t low_entropy_count) const {

    // allocate space for the encoding
    size_t max_size = file_binary_hash.size() + 10 + 10 +
                      file_type.size() + 10 + 10;

    uint8_t encoding[max_size];
    uint8_t* p = encoding;

    // encode each field
    p = lmdb_helper::encode_string(file_binary_hash, p);
    p = lmdb_helper::encode_uint64_t(filesize, p);
    p = lmdb_helper::encode_string(file_type, p);
    p = lmdb_helper::encode_uint64_t(low_entropy_count, p);

#ifdef DEBUG
    std::string encoding_string(reinterpret_cast<char*>(encoding), (p-encoding));
    std::cout << "encoding file_binary_hash "
              << hashdb::to_hex(file_binary_hash)
              << " filesize " << filesize
              << " file_type " << file_type
              << " low_entropy_count " << low_entropy_count
              << "\nto binary data "
              << hashdb::to_hex(encoding_string)
              << " size " << encoding_string.size() << "\n";
#endif

    // return encoding
    return std::string(reinterpret_cast<char*>(encoding), (p-encoding));
  }

  // decoder for data=file_binary_hash, filesize, file_type, low_entropy_count
  void decode_data(const std::string& encoding,
                   std::string& file_binary_hash,
                   uint64_t& filesize,
                   std::string& file_type,
                   uint64_t& low_entropy_count) const {
    const uint8_t* p_start = reinterpret_cast<const uint8_t*>(encoding.c_str());
    const uint8_t* p = p_start;

    p = lmdb_helper::decode_string(p, file_binary_hash);
    p = lmdb_helper::decode_uint64_t(p, filesize);
    p = lmdb_helper::decode_string(p, file_type);
    p = lmdb_helper::decode_uint64_t(p, low_entropy_count);

#ifdef DEBUG
    std::string hex_encoding = hashdb::to_hex(encoding);
    std::cout << "decoding " << hex_encoding
              << " size " << encoding.size() << "\n to"
              << " file_binary_hash "
              << hashdb::to_hex(file_binary_hash)
              << " filesize " << filesize
              << " file_type " << file_type
              << " low_entropy_count " << low_entropy_count << "\n";
#endif

    // validate that the decoding was properly consumed
    if ((size_t)(p - p_start) != encoding.size()) {
      std::cerr << "decode failure: " << &p << " is not " << &p_start << "\n";
      assert(0);
    }
  }

  public:
  lmdb_source_data_manager_t(const std::string& p_hashdb_dir,
                            const file_mode_type_t p_file_mode) :
       hashdb_dir(p_hashdb_dir),
       file_mode(p_file_mode),
       env(lmdb_helper::open_env(hashdb_dir + "/lmdb_source_data_store",
                                                            file_mode)),
       M() {
    MUTEX_INIT(&M);
  }

  ~lmdb_source_data_manager_t() {
    // close the lmdb_hash_store DB environment
    mdb_env_close(env);

    MUTEX_DESTROY(&M);
  }

  /**
   * Insert unless there and same.
   */
  void insert(const uint64_t source_id,
              const std::string& file_binary_hash,
              const uint64_t filesize,
              const std::string& file_type,
              const uint64_t low_entropy_count,
              lmdb_changes_t& changes) {

    MUTEX_LOCK(&M);

    // maybe grow the DB
    lmdb_helper::maybe_grow(env);

    // get context
    lmdb_context_t context(env, true, false); // writable, no duplicates
    context.open();

    // set key
    std::string encoding = encode_key(source_id);
    lmdb_helper::point_to_string(encoding, context.key);

    // see if source data is already there
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    if (rc == MDB_NOTFOUND) {

      // perform insert
      encoding = encode_data(file_binary_hash, filesize, file_type,
                             low_entropy_count);
      lmdb_helper::point_to_string(encoding, context.data);
      rc = mdb_put(context.txn, context.dbi,
                   &context.key, &context.data, MDB_NODUPDATA);

      // the add request must work
      if (rc != 0) {
        std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
        assert(0);
      }
 
      // new source data inserted
      context.close();
      ++changes.source_data_inserted;
      MUTEX_UNLOCK(&M);
      return;

    } else if (rc == 0) {
      // already there, note and change if different
      std::string p_file_binary_hash;
      uint64_t p_filesize;
      std::string p_file_type;
      uint64_t p_low_entropy_count;

      encoding = lmdb_helper::get_string(context.data);
      decode_data(encoding, p_file_binary_hash, p_filesize, p_file_type,
                  p_low_entropy_count);

      if (file_binary_hash != p_file_binary_hash ||
          filesize != p_filesize ||
          file_type != p_file_type ||
          low_entropy_count != p_low_entropy_count) {
        ++changes.source_data_changed;

        // overwrite
        encoding = encode_data(file_binary_hash, filesize, file_type,
                               low_entropy_count);
        lmdb_helper::point_to_string(encoding, context.data);
        rc = mdb_put(context.txn, context.dbi,
                     &context.key, &context.data, MDB_NODUPDATA);

        // the overwrite must work
        if (rc != 0) {
          std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
          assert(0);
        }
      } else { 
        ++changes.source_data_same;
      }

      // closeure for already there
      context.close();
      MUTEX_UNLOCK(&M);
      return;

    } else {
      // invalid rc
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
  }

  /**
   * Find data, false on no source ID.
   */
  bool find(const uint64_t source_id,
            std::string& file_binary_hash,
            uint64_t& filesize,
            std::string& file_type,
            uint64_t& low_entropy_count) const {

    // get context
    lmdb_context_t context(env, false, false); // not writable, no duplicates
    context.open();

    // set key
    std::string encoding = encode_key(source_id);
    lmdb_helper::point_to_string(encoding, context.key);

    // set the cursor to this key
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    if (rc == 0) {
      // hash found
      encoding = lmdb_helper::get_string(context.data);
      decode_data(encoding, file_binary_hash, filesize, file_type,
                  low_entropy_count);
      context.close();
      return true;

    } else if (rc == MDB_NOTFOUND) {
      file_binary_hash = "";
      filesize = 0;
      file_type = "";
      low_entropy_count = 0;
      context.close();
      return false;

    } else {
      // invalid rc
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
  }

  /**
   * Return first source ID.
   */
  std::pair<bool, uint64_t> find_begin() const {

    // get context
    lmdb_context_t context(env, false, false);
    context.open();

    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_FIRST);

    if (rc == 0) {
      // return the key
      context.close();
      uint64_t source_id;
      std::string encoding = lmdb_helper::get_string(context.key);
      decode_key(encoding, source_id);
      return std::pair<bool, uint64_t>(true, source_id);

    } else if (rc == MDB_NOTFOUND) {
      // no key
      context.close();
      return std::pair<bool, uint64_t>(false, 0);

    } else {
      // invalid rc
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
  }

  /**
   * Return next source ID.  Error if no next.
   */
  std::pair<bool, uint64_t> find_next(const uint64_t last_source_id) const {

    // get context
    lmdb_context_t context(env, false, false);
    context.open();

    // set the cursor to last key
    std::string encoding = encode_key(last_source_id);
    lmdb_helper::point_to_string(encoding, context.key);
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    // the last key must exist
    if (rc != 0) {
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    // move cursor to this hash
    rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                        MDB_NEXT);

    if (rc == MDB_NOTFOUND) {
      // no values for this key
      context.close();
      return std::pair<bool, uint64_t>(false, 0);

    } else if (rc == 0) {
      uint64_t source_id;
      encoding = lmdb_helper::get_string(context.key);
      decode_key(encoding, source_id);
      context.close();
      return std::pair<bool, uint64_t>(true, source_id);

    } else {
      // invalid rc
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
  }

  // call this from a lock to prevent getting an unstable answer.
  size_t size() const {
    return lmdb_helper::size(env);
  }
};

#endif

