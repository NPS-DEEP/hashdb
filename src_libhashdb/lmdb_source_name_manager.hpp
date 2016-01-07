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
 * Manage the LMDB source name store.  Threadsafe.
 */

#ifndef LMDB_SOURCE_NAME_MANAGER_HPP
#define LMDB_SOURCE_NAME_MANAGER_HPP
#include "file_modes.h"
#include "lmdb.h"
#include "lmdb_helper.h"
#include "lmdb_context.hpp"
#include <vector>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <string>

// no concurrent writes
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "mutex_lock.hpp"

class lmdb_source_name_manager_t {

  private:
  typedef std::pair<std::string, std::string> source_name_t;
  typedef std::set<source_name_t> source_names_t;

  std::string hashdb_dir;
  file_mode_type_t file_mode;
  source_names_t* source_names;
  MDB_env* env;
#ifdef HAVE_PTHREAD
  mutable pthread_mutex_t M;                  // mutext
#else
  mutable int M;                              // placeholder
#endif

  // do not allow copy or assignment
  lmdb_source_name_manager_t(const lmdb_source_name_manager_t&);
  lmdb_source_name_manager_t& operator=(const lmdb_source_name_manager_t&);

  // note: same as in lmdb_source_data_manager
  // encoder for key=source_id
  static std::string encode_key(const uint64_t source_id) {

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
  static void decode_key(uint64_t& source_id) {
    const uint8_t* p_start = reinterpret_cast<const uint8_t*>(encoding.c_str());
    const uint8_t* p = p_start;

    p = lmdb_helper::decode_uint64_t(p, source_id);

    // validate that the decoding was properly consumed
    if ((size_t)(p - p_start) != encoding.size()) {
      std::cerr << "decode failure: " << &p << " is not " << &p_start << "\n";
      assert(0);
    }
  }

  // encoder for data=set(repository_name, filename)
  static std::string encode_data(const source_names_t& source_names) {

    // allocate space for the encoding, slower but not wrong
    size_t max_size = 0;
    for (std::set<std::string>::const_iterator it = source_names.begin();
                                         it != source_names.end(); ++it) {
      size_t += it->first.size();
      size_t += it->second.size();
    }
    size_t += 10 * source_names.size();

    uint8_t encoding[max_size];
    uint8_t* p = encoding;

    // encode each field
    for (std::set<std::string>::const_iterator it = source_names.begin();
                                         it != source_names.end(); ++it) {
      p = lmdb_helper::encode_string(it->first, p);
      p = lmdb_helper::encode_string(it->second, p);
    }

    // return encoding
    return std::string(reinterpret_cast<char*>(encoding), (p-encoding));
  }

  // decoder for data=set(repository_name, filename)
  static void decode_data(std::string& encoding,
                          source_names_t& source_names) {
    source_names.clear();
    const uint8_t* p_start = reinterpret_cast<const uint8_t*>(encoding.c_str());
    const uint8_t* p_stop = p_start + encoding.size();
    const uint8_t* p = p_start;

    while(p < p_stop) {
      source_name_t pair;
      p = lmdb_helper::decode_uint64_t(p, pair.first);
      p = lmdb_helper::decode_uint64_t(p, pair.second);
      source_names.insert(pair);
    }

    // validate that the decoding was properly consumed
    if ((size_t)(p - p_start) != encoding.size()) {
      std::cerr << "decode failure: " << &p << " is not " << &p_start << "\n";
      assert(0);
    }
  }

  public:
  lmdb_source_name_manager_t(const std::string& p_hashdb_dir,
                      const file_mode_type_t p_file_mode) :
       hashdb_dir(p_hashdb_dir),
       file_mode(p_file_mode),
       settings(read_settings(hashdb_dir)),
       id_offset_pairs(new id_offset_pairs_t),
       env(lmdb_helper::open_env(hashdb_dir + "/lmdb_source_name_store",
                                                                file_mode)),
       M() {

    MUTEX_INIT(&M);

    // eror if settings is not initialized
    if (settings.sector_size == 0) {
      std::cerr << "Error: settings not initialized: " << settings << "\n";
      assert(0);
    }
  }

  ~lmdb_source_name_manager_t() {
    // close the lmdb_hash_store DB environment
    mdb_env_close(env);
    delete id_offset_pairs
  }

  /**
   * Insert repository_name, filename pair.
   * Return true if inserted, false if already there.
   * Fail on invalid source ID.
   */
  bool insert(const uint64_t source_id,
              const std::string& repository_name,
              const std::string& filename) {

    MUTEX_LOCK(&M);

    // maybe grow the DB
    lmdb_helper::maybe_grow(env);

    // get context
    lmdb_context_t context(env, true, false);
    context.open();

    // set key
    std::string encoding = encode_key(source_id);
    lmdb_helper::point_to_string(encoding, context.key);

    // read the existing hash data record
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    // the key does not need to be there, but the read attempt must not fail
    if (rc != 0 && rc != MDB_NOTFOUND) {
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    // set source_names
    if (rc == 0) {

      // read existing sources into set
      encoding = lmdb_helper::get_string(context.data);
      decode_data(encoding, source_names);

    } else {
      // new set
      source_names.clear();
    }

    // prepare the source name pair
    source_name_t pair(repository_name, filename);

    // note if already there
    bool has_source = source_names.find(pair) != source_names.end();

    // insert new pair
    if (!has_source) {
      source_names.insert(pair);
    }

    // write with source inserted
    encoding = encode_data(source_names);
    lmdb_helper::point_to_string(encoding, context.data);
    rc = mdb_put(context.txn, context.dbi,
                 &context.key, &context.data, MDB_NODUPDATA);

    // the write must work
    if (rc != 0) {
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    // hash source inserted
    context.close();
    MUTEX_UNLOCK(&M);
    return has_source;
  }

  /**
   * Find source names, fail on invalid source ID.
   */
  void find(const uint64_t source_id,
            source_names_t& source_names) const {

    // get context
    lmdb_context_t context(env, false, false);
    context.open();

    // set key
    std::string encoding = encode_key(source_id);
    lmdb_helper::point_to_string(encoding, context.key);

    // set the cursor to this key
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    if (rc == 0) {
      // key found
      std::string encoding = lmdb_helper::get_string(context.data);
      decode_data(encoding, source_names);
      context.close();
      return;

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

