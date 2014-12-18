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
#include "source_lookup_encoding.hpp"
#include "hashdb_element.hpp"
#include "hash_t_selector.h"
#include "lmdb.h"
#include "lmdb_resources.h"
#include "lmdb_pthread_resources.hpp"
#include "lmdb_hash_store_iterator.hpp"

// enable debug
#define DEBUG 1

class lmdb_hash_store_manager_t {
  private:
  std::string hashdb_dir;
  file_mode_type_t file_mode;
  MDB_env* env;        // environment pointer

  // increase map size
  // NOTE: A transaction must not be open for this thread when calling this.
  void increase_map_size() {
    // increase map size by at last half again more
    MDB_envinfo stat;
    int rc = mdb_env_info(env, &stat);
    const size_t size = stat.me_mapsize;
    size_t newsize = size*2 - size/2;
    newsize = size + 10485760 - (size % 10485760);
#ifdef DEBUG
    std::cout << "lmdb emplace increase from " << size << " to " << newsize << "\n";
#endif
    rc = mdb_env_set_mapsize(env, newsize);
    if (rc != 0) {
      std::cerr << "lmdb commit failure set_mapsize " << rc << "\n";
      assert(0);
    }
  }

  // emplace but protecting against infinite invocation recursion
  void emplace(const hash_t& hash, uint64_t encoding, int recursion_count) {
    if (recursion_count>2) {
      std::cerr << "lmdb emplace recursion failure.\n";
      assert(0);
    }
    // get resources for the pthread
    pthread_resources_t* resources =
              lmdb_pthread_resources::get_pthread_resources(file_mode, env);

    // set values into the key and data
    pair_to_mdb(hash, encoding, resources->key, resources->data);

    // put
    int rc = mdb_put(resources->txn, resources->dbi,
                     &resources->key, &resources->data, MDB_NODUPDATA);
//zzstd::cout << "lhsm.emplace.rc: " << rc << "\n";
    if (rc == 0) {
      return;
    } else if (rc == MDB_KEYEXIST) {
      std::cerr << "lmdb commit failure keyexist " << rc << "\n";
      assert(0);
    } else if (rc == MDB_MAP_FULL) {
#ifdef DEBUG
    std::cout << "lmdb emplace MDB_MAP_FULL on txn " << resources->txn << "\n";
#endif

      // commit and close active resources
      lmdb_pthread_resources::commit_and_close_resources(resources);

      // increase map size
      increase_map_size();

      // try again with larger map
      emplace(hash, encoding, ++recursion_count);

    } else if (rc == MDB_TXN_FULL) {
#ifdef DEBUG
    std::cout << "lmdb emplace MDB_TXN_FULL on txn " << resources->txn << "\n";
#endif

      // commit and close active transaction
      lmdb_pthread_resources::commit_and_close_resources(resources);

      // try again with empty transaction history
      emplace(hash, encoding, ++recursion_count);
    } else {
      std::cerr << "lmdb commit failure, unexpected error " << rc << "\n";
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
                   hashdb_dir(p_hashdb_dir), file_mode(p_file_mode), env(0) {

    const std::string hash_store_dir = hashdb_dir + "/hash_store";

    // *env
    int rc = mdb_env_create(&env);
    if (rc != 0) {
      // bad failure
      assert(0);
    }

    // DB will keep its own size if its own size is larger
    rc = mdb_env_set_mapsize(env, 10485760);
    if (rc != 0) {
      // bad failure
      assert(0);
    }

    // set environment flags and establish hash_store directory
    unsigned int env_flags;
    switch(file_mode) {
      case READ_ONLY:
std::cout << "opening hash store RO " << hash_store_dir << "\n";
        env_flags = MDB_FIXEDMAP | MDB_RDONLY;
        break;
      case RW_NEW:
std::cout << "opening hash store RW_NEW " << hash_store_dir << "\n";
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
        env_flags = MDB_FIXEDMAP;
        break;
      case RW_MODIFY:
std::cout << "opening hash store RW_MODIFY " << hash_store_dir << "\n";
        env_flags = MDB_FIXEDMAP;
    }

std::cout << "opening " << hash_store_dir << "\n";
    // open the MDB environment
    rc = mdb_env_open(env, hash_store_dir.c_str(), env_flags, 0664);
    if (rc != 0) {
      // fail
      std::cerr << "Error opening database: " << rc << ".  Aborting.\n";
      exit(1);
    }
  }

  ~lmdb_hash_store_manager_t() {
    lmdb_pthread_resources::commit_and_close_all_resources();
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
              lmdb_pthread_resources::get_pthread_resources(file_mode, env);

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
              lmdb_pthread_resources::get_pthread_resources(file_mode, env);

    uint64_t encoding = 0;
    pair_to_mdb(hash, encoding, resources->key, resources->data);

    // set the cursor to this key
    int rc = mdb_cursor_get(resources->cursor,
                            &resources->key, NULL,
                            MDB_SET_RANGE);
    if (rc == 0) {
      // great, db has key
//zzstd::cout << "lhsm.erase hash.a\n";
    } else if (rc == MDB_NOTFOUND) {
      // fine, db does not have key
      return 0;
    } else if (rc != 0) {
      // program error
      std::cerr << "rc " << rc << "\n";
      assert(0);
    }

//zzstd::cout << "lhsm.erase hash.b\n";
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
//zzstd::cout << "lhsm.erase hash.b\n";
      return key_count;
    } else if (rc == MDB_NOTFOUND) {
      return 0;
    } else {
      assert(0);
    }
  }

  // lower_bound
  lmdb_hash_store_iterator_t lower_bound(const hash_t& hash) const {

    return lmdb_hash_store_iterator_t(file_mode, env, hash, true);
  }

  // equal_range
  std::pair<lmdb_hash_store_iterator_t, lmdb_hash_store_iterator_t>
                             equal_range(hash_t hash) const {

    return std::pair<lmdb_hash_store_iterator_t, lmdb_hash_store_iterator_t>(
                   lmdb_hash_store_iterator_t(file_mode, env, hash, true),
                   lmdb_hash_store_iterator_t(file_mode, env, hash, false));
  }

  // find
  bool find(const hash_t& hash, uint64_t value) const {

    // get resources for the pthread
    pthread_resources_t* resources =
              lmdb_pthread_resources::get_pthread_resources(file_mode, env);

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

  // count
  size_t count(const hash_t& hash) const {

    // get resources for the pthread
    pthread_resources_t* resources =
              lmdb_pthread_resources::get_pthread_resources(file_mode, env);

    // set key and data
    uint64_t value = 0;
    pair_to_mdb(hash, value, resources->key, resources->data);

    // set the cursor to this key
    int rc = mdb_cursor_get(resources->cursor,
                            &resources->key, &resources->data,
                            MDB_SET_RANGE);
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

    return lmdb_hash_store_iterator_t(file_mode, env, true);
  }

  // end
  lmdb_hash_store_iterator_t end() const {

    return lmdb_hash_store_iterator_t(file_mode, env, false);
  }

  size_t size() const {
    // not threadsafe

    // commit and close all active resources
    lmdb_pthread_resources::commit_and_close_all_resources();

    // now obtain statistics
    MDB_stat stat;
    int rc = mdb_env_stat(env, &stat);
    if (rc != 0) {
      // program error
      assert(0);
    }
std::cout << "lhsm.ms_entries " << stat.ms_entries << "\n";
std::cout << "lhsm.ms_psize " << stat.ms_psize << "\n";
    return stat.ms_entries;
  }
};

#endif

