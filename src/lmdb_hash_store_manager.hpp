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
#include "lmdb_hash_store_iterator.hpp"
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "mutex_lock.hpp" // for managing pthread resources

// enable debug
#define DEBUG 1

static pthread_key_t pthread_resources_key;
static pthread_once_t pthread_resources_key_once = PTHREAD_ONCE_INIT;
static std::set<pthread_resources_t*> pthread_resource_set;
static void make_pthread_resources_key() {
  (void) pthread_key_create(&pthread_resources_key, NULL);
}

class lmdb_hash_store_manager_t {
  private:
  std::string hashdb_dir;
  file_mode_type_t file_mode;
  MDB_env* env;        // environment pointer
#ifdef HAVE_PTHREAD
  // not that M is mutable, but that pthread_mutex_lock is not declared const.
  mutable pthread_mutex_t M;                  // mutext
#else
  mutable int M;                              // placeholder
#endif

  // helper
  pthread_resources_t* get_pthread_resources() const {
    (void)pthread_once(&pthread_resources_key_once, make_pthread_resources_key);
    pthread_resources_t* pthread_resources;
    if (pthread_getspecific(pthread_resources_key) == NULL) {
      pthread_resources = new pthread_resources_t;

      // create the thread-specific txn object
      const int txn_flags = (file_mode == READ_ONLY) ? MDB_RDONLY : 0;
      int rc = mdb_txn_begin(env, NULL, txn_flags, &pthread_resources->txn);
      if (rc == MDB_PANIC) {
        std::cerr << "LMDB transaction error.  Aborting.\n";
        assert(0);
      }
      if (rc == MDB_MAP_RESIZED) {
        // reset the size as required by the environment
        rc = mdb_env_set_mapsize(env, 0);
        if (rc != 0) {
          assert(0);
        }
      } else if (rc == MDB_READERS_FULL) {
        // if this becomes a problem, CPUs>126, use mdb_env_set_maxreaders
        assert(0);
      } else if (rc == ENOMEM) {
        std::cerr << "LMDB transaction error.  No memory.  Aborting.\n";
        assert(0);
      } else if (rc != 0) {
        std::cerr << "LMDB transaction error.  Unexpected error.  Aborting.\n";
        assert(0);
      }

      // create the database handle integer
      unsigned int dbi_flags = MDB_DUPSORT | MDB_DUPFIXED;
      if (file_mode != READ_ONLY) {
        dbi_flags |= MDB_CREATE;
      }
      rc = mdb_dbi_open(pthread_resources->txn, NULL, dbi_flags, &pthread_resources->dbi);
      if (rc == MDB_NOTFOUND) {
        assert(0);
      } else if (rc == MDB_DBS_FULL) {
        assert(0);
      } else if (rc != 0) {
        assert(0);
      }

      // create a new cursor object to use with this txn
      rc = mdb_cursor_open(pthread_resources->txn,
                           pthread_resources->dbi,
                           &pthread_resources->cursor);
      if (rc != 0) {
        assert(0);
      }

      // bind the pthread variable to this pthread key
      pthread_setspecific(pthread_resources_key, pthread_resources);

      // save pointer for destructor
      MUTEX_LOCK(&M);
std::cout << "lhsm.get_pthread_resources created " << pthread_resources << "\n";
      pthread_resource_set.insert(pthread_resources);
      MUTEX_UNLOCK(&M);

    } else {
      // get the stored thread-specific resources
      pthread_resources = static_cast<pthread_resources_t*>(
                                 pthread_getspecific(pthread_resources_key));
    }

    return pthread_resources;
  }


  // helper
  void commit_and_close_resources(pthread_resources_t* resources) const {
std::cout << "commit_and_close_resources released " << resources << "\n";
    // free cursor
    mdb_cursor_close(resources->cursor);

    // do not close dbi handle

    // commit and close active transaction
    int rc = mdb_txn_commit(resources->txn);
    if (rc != 0) {
      // no disk, no memory, etc
      std::cerr << "lmdb commit failure txn_commit " << rc << "\n";
      assert(0);
    }

    // remove resources from pthread resources key
    pthread_setspecific(pthread_resources_key, NULL);

    // remove resources from resource set
    MUTEX_LOCK(&M);
    pthread_resource_set.erase(resources);
    MUTEX_UNLOCK(&M);
  }

  void commit_and_close_all_resources() const {
    // on this main thread, close resources that were opened for each thread
    while (true) {
      std::set<pthread_resources_t*>::iterator it =
                pthread_resource_set.begin();
      if (it == pthread_resource_set.end()) {
        break;
      }

      // close selected resource
      pthread_resources_t* resources = *it;
      commit_and_close_resources(resources);
    }
  }
  
  // helper
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

  // inset protecting against infinite invocation recursion
  void emplace(const hash_t& hash, uint64_t encoding, int recursion_count) {
    if (recursion_count>2) {
      std::cerr << "lmdb emplace recursion failure.\n";
      assert(0);
    }
    // get resources for the pthread
    pthread_resources_t* resources = get_pthread_resources();

    // set values into the key and data
    pair_to_mdb(hash, encoding, resources->key, resources->data);

    // put
    int rc = mdb_put(resources->txn, resources->dbi,
                     &resources->key, &resources->data, MDB_NODUPDATA);
std::cout << "lhsm.emplace.rc: " << rc << "\n";
    if (rc == 0) {
      return;
    } else if (rc == MDB_KEYEXIST) {
      std::cerr << "lmdb commit failure keyexist " << rc << "\n";
      assert(0);
    } else if (rc == MDB_MAP_FULL) {

      // commit and close active resources
      commit_and_close_resources(resources);

      // increase map size
      increase_map_size();

      // try again with larger map
      emplace(hash, encoding, ++recursion_count);

    } else if (rc == MDB_TXN_FULL) {

      // commit and close active transaction
      commit_and_close_resources(resources);

      // try again with empty transaction history
      emplace(hash, encoding, ++recursion_count);
    } else {
      std::cerr << "lmdb commit failure, unexpected error " << rc << "\n";
      assert(0);
    }
  }

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
    commit_and_close_all_resources();
  }
  
  // emplace
  void emplace(const hash_t& hash, uint64_t encoding) {
    // call emplace with recursion check
    emplace(hash, encoding, 0);
  }

  // erase hash, encoding pair
  bool erase(const hash_t& hash, uint64_t encoding) {

    // get resources for the pthread
    pthread_resources_t* resources = get_pthread_resources();

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
    pthread_resources_t* resources = get_pthread_resources();

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
                 &resources->key, &resources->data);

    if (rc == 0) {
      return key_count;
    } else if (rc == MDB_NOTFOUND) {
      return 0;
    } else {
      assert(0);
    }
  }

  // lower_bound
  lmdb_hash_store_iterator_t lower_bound(hash_t hash) const {

    // get resources for the pthread
    pthread_resources_t* resources = get_pthread_resources();

    return lmdb_hash_store_iterator_t(resources, hash, true);
  }

  // equal_range
  std::pair<lmdb_hash_store_iterator_t, lmdb_hash_store_iterator_t>
                             equal_range(hash_t hash) const {

    // get resources for the pthread
    pthread_resources_t* resources = get_pthread_resources();

    return std::pair<lmdb_hash_store_iterator_t, lmdb_hash_store_iterator_t>(
                   lmdb_hash_store_iterator_t(resources, hash, true),
                   lmdb_hash_store_iterator_t(resources, hash, false));
  }

  // count
  size_t count(hash_t hash) const {

    // get resources for the pthread
    pthread_resources_t* resources = get_pthread_resources();

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

    // get resources for the pthread
    pthread_resources_t* resources = get_pthread_resources();

    return lmdb_hash_store_iterator_t(resources, true);
  }

  // end
  lmdb_hash_store_iterator_t end() const {

    // get resources for the pthread
    pthread_resources_t* resources = get_pthread_resources();

    return lmdb_hash_store_iterator_t(resources, false);
  }

  size_t size() const {

    // commit and close all active resources
    commit_and_close_all_resources();

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

