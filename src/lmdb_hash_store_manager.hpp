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
 * Makes minimal map-like interfaces for accessing LMDB.
 * Is threadsafe by managing transaction-specifc threads.
 *
 * Each thread runs with an active open transaction and cursor
 * which are closed by the main thread on delete.
 */

#ifndef LMDB_HASH_STORE_MANAGER_HPP
#define LMDB_HASH_STORE_MANAGER_HPP

// enable debug
// #define DEBUG 1

#include "source_lookup_encoding.hpp"
#include "hashdb_element.hpp"
#include "hash_t_selector.h"
#include "lmdb.h"
#include "lmdb_resources.h"
#include "lmdb_resources_manager.hpp"
#include "lmdb_hash_store_iterator.hpp"

class lmdb_hash_store_manager_t {
  private:
  std::string hashdb_dir;
  file_mode_type_t file_mode;
  MDB_env* env;        // environment pointer
  mutable lmdb_resources_manager_t resources_manager;

  // *env
  static MDB_env* make_env() {
    MDB_env* new_env;
    int rc = mdb_env_create(&new_env);
    if (rc != 0) {
      // bad failure
      assert(0);
    }

    return new_env;
  }

  // do not allow these
  lmdb_hash_store_manager_t();
  lmdb_hash_store_manager_t(const lmdb_hash_store_manager_t&);
  lmdb_hash_store_manager_t& operator=(const lmdb_hash_store_manager_t&);

  public:
  lmdb_hash_store_manager_t(const std::string& p_hashdb_dir,
                            file_mode_type_t p_file_mode) :
                   hashdb_dir(p_hashdb_dir),
                   file_mode(p_file_mode),
                   env(make_env()),
                   resources_manager(file_mode, env) {

    const std::string hash_store_dir = hashdb_dir + "/lmdb_hash_store";

    // set environment flags and establish hash_store directory
    unsigned int env_flags;
#ifdef DEBUG
    std::cout << "lmdb_hash_store_manager_t opening: " << hash_store_dir
              << ", mode: " << file_mode_type_to_string(file_mode) << "\n";
#endif
              
    switch(file_mode) {
      case READ_ONLY:
        env_flags = MDB_RDONLY;
        break;
      case RW_NEW:
        // hash store directory must not exist yet
        if (access(hash_store_dir.c_str(), F_OK) == 0) {
          std::cerr << "Error: Database '" << hash_store_dir << "' already exists.  Aborting.\n";
          exit(1);
        }

        // create the hash store directory
#ifdef WIN32
        if(mkdir(hash_store_dir.c_str())){
          std::cerr << "Error: Could not make new hash store directory '"
                    << hash_store_dir << "'.\nCannot continue.\n";
          exit(1);
        }
#else
        if(mkdir(hash_store_dir.c_str(),0777)){
          std::cerr << "Error: Could not make new hash store directory '"
                    << hash_store_dir << "'.\nCannot continue.\n";
          exit(1);
        }
#endif
        env_flags = MDB_NOMETASYNC | MDB_NOSYNC;
        break;
      case RW_MODIFY:
        env_flags = MDB_NOMETASYNC | MDB_NOSYNC;
        break;
      default:
        assert(0);
    }

    // open the MDB environment
    int rc = mdb_env_open(env, hash_store_dir.c_str(), env_flags, 0664);
    if (rc != 0) {
      // fail
      std::cerr << "Error opening database: " << rc << ": " <<  mdb_strerror(rc)
                << "\nAborting.\n";
      exit(1);
    }
  }

  // emplace
  void emplace(const hash_t& hash, uint64_t encoding) {

    // open resources
    lmdb_resources_t* resources = resources_manager.open_rw_resources();
 
    // set values into the key and data
    pair_to_mdb(hash, encoding, resources->key, resources->data);

    // emplace
    int rc = mdb_put(resources->txn, resources->dbi,
                     &resources->key, &resources->data, MDB_NODUPDATA);
    if (rc != 0) {
      std::cerr << "LMDB emplace error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    // close resources
    resources_manager.close_rw_resources();
  }

  // erase hash, encoding pair
  bool erase(const hash_t& hash, uint64_t encoding) {

    // open resources
    lmdb_resources_t* resources = resources_manager.open_rw_resources();
 
    // set values into the key and data
    pair_to_mdb(hash, encoding, resources->key, resources->data);

    // erase
    int rc = mdb_del(resources->txn, resources->dbi,
                     &resources->key, &resources->data);
    bool status;
    if (rc == 0) {
      status = true;
    } else if (rc == MDB_NOTFOUND) {
      status = false;
    } else {
      std::cerr << "LMDB erase error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    // close resources
    resources_manager.close_rw_resources();

    return status;
  }

  // erase hash, return count erased
  size_t erase(const hash_t& hash) {

    // open resources
    lmdb_resources_t* resources = resources_manager.open_rw_resources();

    // set values into the key and data
    uint64_t encoding = 0;
    pair_to_mdb(hash, encoding, resources->key, resources->data);

    // set the cursor to this key
    int rc = mdb_cursor_get(resources->cursor, &resources->key,
                            NULL, MDB_SET_RANGE);

    size_t key_count;
    if (rc == 0) {

      // DB has key so look up the key count
      rc = mdb_cursor_count(resources->cursor, &key_count);
      if (rc != 0) {
        std::cerr << "LMDB erase count error: " << mdb_strerror(rc) << "\n";
        assert(0);
      }

    } else if (rc == MDB_NOTFOUND) {
      // DB does not have key
      key_count = 0;

    } else {
      std::cerr << "LMDB erase cursor get error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    if (key_count > 0) {

      // delete if there
      rc = mdb_del(resources->txn, resources->dbi, 
                   &resources->key, NULL);
      if (rc != 0) {
        std::cerr << "LMDB erase delete error: " << mdb_strerror(rc) << "\n";
        assert(0);
      }
    }

    // close resources
    resources_manager.close_rw_resources();

    return key_count;
  }

  // lower_bound
  lmdb_hash_store_iterator_t lower_bound(const hash_t& hash) const {

    return lmdb_hash_store_iterator_t(&resources_manager, hash, true);
  }

  // equal_range
  std::pair<lmdb_hash_store_iterator_t, lmdb_hash_store_iterator_t>
                             equal_range(const hash_t& hash) const {

    return std::pair<lmdb_hash_store_iterator_t, lmdb_hash_store_iterator_t>(
             lmdb_hash_store_iterator_t(&resources_manager, hash, true),
             lmdb_hash_store_iterator_t(&resources_manager, hash, false));
  }

  // find specific hash, value pair
  bool find(const hash_t& hash, uint64_t value) const {

    // get resources
    lmdb_resources_t* resources = resources_manager.open_resources();

    // set key and data
    pair_to_mdb(hash, value, resources->key, resources->data);

    // set the cursor to this key,data pair
    int rc = mdb_cursor_get(resources->cursor,
                            &resources->key, &resources->data,
                            MDB_GET_BOTH);
    bool has_pair;
    if (rc == 0) {
      has_pair = true;
    } else if (rc == MDB_NOTFOUND) {
      has_pair = false;
    } else {
      // program error
      assert(0);
    }

    // close resources
    resources_manager.close_resources();

    return has_pair;
  }

  // count of entries with this hash value
  size_t count(const hash_t& hash) const {

    // open resources
    lmdb_resources_t* resources = resources_manager.open_resources();

    // set key and data
    uint64_t value = 0;
    pair_to_mdb(hash, value, resources->key, resources->data);

    // set the cursor to this key
    int rc = mdb_cursor_get(resources->cursor,
                            &resources->key, &resources->data,
                            MDB_SET_KEY);
    size_t key_count = 0;
    if (rc == 0) {
      rc = mdb_cursor_count(resources->cursor, &key_count);
      if (rc != 0) {
        std::cerr << "LMDB count error: " << mdb_strerror(rc) << "\n";
        assert(0);
      }
    } else if (rc == MDB_NOTFOUND) {
      // fine, key count is zero
    } else {
      std::cerr << "LMDB get error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    // close resources
    resources_manager.close_resources();

    return key_count;
  }

  // begin
  lmdb_hash_store_iterator_t begin() const {

    return lmdb_hash_store_iterator_t(&resources_manager, true);
  }

  // end
  lmdb_hash_store_iterator_t end() const {

    return lmdb_hash_store_iterator_t(&resources_manager, false);
  }

  // size number of entries in DB
  size_t size() const {
    // not locked

    // obtain statistics
    MDB_stat stat;
    int rc = mdb_env_stat(env, &stat);
    if (rc != 0) {
      // program error
      assert(0);
    }
#ifdef DEBUG
    std::cout << "lmdb_hash_store_manager ms_entries size: " << stat.ms_entries
              << "\n";
#endif
    return stat.ms_entries;
  }
};

#endif

