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
 * Makes minimal map-like interfaces out of LMDB.
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
#include "lmdb_resource_manager.hpp"
#include "lmdb_hash_store_iterator.hpp"

class lmdb_hash_store_manager_t {
  private:
  std::string hashdb_dir;
  file_mode_type_t file_mode;
  MDB_env* env;        // environment pointer
  lmdb_resource_manager_t lmdb_resource_manager;

  // *env
  static MDB_env* make_env() {
    MDB_env* new_env;
    int rc = mdb_env_create(&new_env);
    if (rc != 0) {
      // bad failure
      assert(0);
    }

    // mapsize is a limit to protect the OS environment, see
    // http://comments.gmane.org/gmane.network.openldap.technical/11699
    rc = mdb_env_set_mapsize(new_env, 0x001000000000);
    if (rc != 0) {
      // bad failure
      assert(0);
    }

    return new_env;
  }

  // emplace but protecting against infinite invocation recursion
  void emplace(const hash_t& hash, uint64_t encoding, int recursion_count) {
    if (recursion_count>2) {
      std::cerr << "emplace recursion failure.\n";
      assert(0);
    }
    // get resources for the pthread
    pthread_resources_t* resources =
              lmdb_resource_manager.get_pthread_resources();

    // set values into the key and data
    pair_to_mdb(hash, encoding, resources->key, resources->data);

    // put
    int rc = mdb_put(resources->txn, resources->dbi,
                     &resources->key, &resources->data, MDB_NODUPDATA);
    if (rc == 0) {
      return;
    } else if (rc == MDB_KEYEXIST) {
      std::cerr << "emplace failure keyexist " << rc << "\n";
      assert(0);
    } else if (rc == MDB_MAP_FULL) {
      // NOTE: mapsize is a limit to protect the OS environment,
      // see http://comments.gmane.org/gmane.network.openldap.technical/11699
      std::cerr << "emplace MDB_MAP_FULL on resources " << resources << "\n"
                << "error: hashdb hardcoded max lmdb map size reached.\n";
      exit(1);

    } else if (rc == MDB_TXN_FULL) {
      // NOTE: http://www.openldap.org/lists/openldap-technical/201410/msg00092.html
#ifdef DEBUG
      std::cout << "emplace MDB_TXN_FULL on resources " << resources << "\n";
#endif
std::cout << "emplace MDB_TXN_FULL on resources " << resources << "\n";

      // commit and close active transaction
      lmdb_resource_manager.commit_and_close_thread_resources();

      // try again with empty transaction history
      emplace(hash, encoding, ++recursion_count);
    } else {
      std::cerr << "emplace failure, unexpected error " << rc << "\n";
      assert(0);
    }
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
                   lmdb_resource_manager(file_mode, env) {

    const std::string hash_store_dir = hashdb_dir + "/lmdb_hash_store";

    // set environment flags and establish hash_store directory
    unsigned int env_flags;
#ifdef DEBUG
    std::cout << "lmdb_hash_store_manager_t opening: " << hash_store_dir
              << ", mode: " << file_mode_type_to_string(file_mode) << "\n";
#endif
              
    switch(file_mode) {
      case READ_ONLY:
//        env_flags = MDB_FIXEDMAP | MDB_RDONLY;
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
//        env_flags = MDB_FIXEDMAP;
        env_flags = 0;
        break;
      case RW_MODIFY:
//        env_flags = MDB_FIXEDMAP;
        env_flags = 0;
    }

    // open the MDB environment
    int rc = mdb_env_open(env, hash_store_dir.c_str(), env_flags, 0664);
    if (rc != 0) {
      // fail
      std::cerr << "Error opening database: " << rc << ".  Aborting.\n";
      exit(1);
    }
  }

  // emplace
  void emplace(const hash_t& hash, uint64_t encoding) {
    // call emplace with recursion check
    emplace(hash, encoding, 0);
  }

  // erase hash, encoding pair
  bool erase(const hash_t& hash, uint64_t encoding) {

    // get resources for the pthread
    pthread_resources_t* resources =
              lmdb_resource_manager.get_pthread_resources();

    pair_to_mdb(hash, encoding, resources->key, resources->data);

    int rc = mdb_del(resources->txn, resources->dbi, 
                     &resources->key, &resources->data);

    if (rc == 0) {
      return true;
    } else if (rc == MDB_NOTFOUND) {
      return false;
    } else {
      assert(0);
    }
  }

  // erase hash, return count erased
  size_t erase(const hash_t& hash) {

    // get resources for the pthread
    pthread_resources_t* resources =
              lmdb_resource_manager.get_pthread_resources();

    uint64_t encoding = 0;
    pair_to_mdb(hash, encoding, resources->key, resources->data);

    // set the cursor to this key
    int rc = mdb_cursor_get(resources->cursor,
                            &resources->key, NULL,
                            MDB_SET_RANGE);
    if (rc == 0) {
      // great, db has key
    } else if (rc == MDB_NOTFOUND) {
      // fine, db does not have key
      return 0;
    } else if (rc != 0) {
      // program error
      std::cerr << "rc " << rc << "\n";
      assert(0);
    }

    // get the key count
    size_t key_count;
    rc = mdb_cursor_count(resources->cursor, &key_count);
    if (rc != 0) {
      // program error
      assert(0);
    }

    rc = mdb_del(resources->txn, resources->dbi, 
                 &resources->key, NULL);

    if (rc == 0) {
      return key_count;
    } else if (rc == MDB_NOTFOUND) {
      return 0;
    } else {
      assert(0);
    }
  }

  // lower_bound
  lmdb_hash_store_iterator_t lower_bound(const hash_t& hash) const {

    return lmdb_hash_store_iterator_t(lmdb_resource_manager, hash, true);
  }

  // equal_range
  std::pair<lmdb_hash_store_iterator_t, lmdb_hash_store_iterator_t>
                             equal_range(const hash_t& hash) const {

    return std::pair<lmdb_hash_store_iterator_t, lmdb_hash_store_iterator_t>(
             lmdb_hash_store_iterator_t(lmdb_resource_manager, hash, true),
             lmdb_hash_store_iterator_t(lmdb_resource_manager, hash, false));
  }

  // find specific hash, value pair
  bool find(const hash_t& hash, uint64_t value) const {

    // get resources for the pthread
    pthread_resources_t* resources =
              lmdb_resource_manager.get_pthread_resources();

    // set key and data
    pair_to_mdb(hash, value, resources->key, resources->data);

    // set the cursor to this key,data pair
    int rc = mdb_cursor_get(resources->cursor,
                            &resources->key, &resources->data,
                            MDB_GET_BOTH);
    if (rc == 0) {
      return true;
    } else if (rc == MDB_NOTFOUND) {
      return false;
    } else {
      // program error
      assert(0);
    }
  }

  // count of entries with this hash value
  size_t count(const hash_t& hash) const {

    // get resources for the pthread
    pthread_resources_t* resources =
              lmdb_resource_manager.get_pthread_resources();

    // set key and data
    uint64_t value = 0;
    pair_to_mdb(hash, value, resources->key, resources->data);

    // set the cursor to this key
    int rc = mdb_cursor_get(resources->cursor,
                            &resources->key, &resources->data,
                            MDB_SET_KEY);
    if (rc == MDB_NOTFOUND) {
      return 0;
    } else if (rc != 0) {
      // program error
      assert(0);
    }

    // get the count
    size_t key_count;
    rc = mdb_cursor_count(resources->cursor, &key_count);
    if (rc != 0) {
      // program error
      assert(0);
    }
    return key_count;
  }

  // begin
  lmdb_hash_store_iterator_t begin() const {

    return lmdb_hash_store_iterator_t(lmdb_resource_manager, true);
  }

  // end
  lmdb_hash_store_iterator_t end() const {

    return lmdb_hash_store_iterator_t(lmdb_resource_manager, false);
  }

  // size number of entries in DB
  size_t size() const {
    // not threadsafe

    // commit and close all active resources
    lmdb_resource_manager.commit_and_close_all_resources();

    // now obtain statistics
    MDB_stat stat;
    int rc = mdb_env_stat(env, &stat);
    if (rc != 0) {
      // program error
      assert(0);
    }
#ifdef DEBUG
    std::cout << "lmdb_hash_store_manager ms_entries size: " << stat.ms_entries << "\n";
#endif
    return stat.ms_entries;
  }
};

#endif

