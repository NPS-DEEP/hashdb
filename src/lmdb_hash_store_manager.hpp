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
 */

#ifndef LMDB_HASH_STORE_MANAGER_HPP
#define LMDB_HASH_STORE_MANAGER_HPP
#include "source_lookup_encoding.hpp"
#include "hashdb_element.hpp"
#include "hash_t_selector.h"
#include "liblmdb/lmdb.h"
#include "lmdb_resources.h"
#include "mutex_lock.hpp" // for managing pthread resources

// enable debug
#define DEBUG 1

static pthread_key_t pthread_resources_key;
static pthread_once_t pthread_resources_key_once = PTHREAD_ONCE_INIT;
static std::set<pthread_resources_t*> pthread_resource_set;
static void make_pthread_resources_key() {
  (void) pthread_key_create(&pthread_resources_key, NULL);
}

inline pthread_resources_t* get_pthread_resources(MDB_env* env,
                                               file_mode_type_t file_mode) {
  (void) pthread_once(&pthread_resources_once, make_txn_key);
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
    rc = mdb_dbi_open(pthread_resources->txn, NULL, 0, &pthread_resources->dbi);
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
    pthread_setspecific(pthread_txn_key, pthread_resources);

    // save pointer for destructor
    MUTEX_LOCK(&M);
    pthread_resource_set.insert(pthread_resources);
    MUTEX_UNLOCK(&M);

  } else {
    // get the stored thread-specific resources
    pthread_resources = static_cast<pthread_resources_t*>(
                                 pthread_getspecific(pthread_resources_key);
  }

  return pthread_txn;
}

//// macros for use with LMDB
//#define E(expr) CHECK((rc = (expr)) == MDB_SUCCESS, #expr)
//#define RES(err, expr) ((rc = expr) == (err) || (CHECK(!rc, #expr), 0))
//#define CHECK(test, msg) ((test) ? (void)0 : ((void)fprintf(stderr, \
//	"%s:%d: %s: %s\n", __FILE__, __LINE__, msg, mdb_strerror(rc)), abort()))

// key and data types for hash_store
// zz may pass as template instead using T_K, T_D
typedef hash_t key_t;
typedef uint64_t data_t;
typedef std::pair<key_t, data_t> pair_t;

class lmdb_hash_store_manager_t {
  private:
  std::string hashdb_dir;
  file_mode_type_t file_mode;
  MDB_env* env;        // environment pointer

  // helper
  void commit_and_close_resources(pthread_resources_t* resources) {
    // free cursor
    mdb_cursor_close(resources->cursor);

    // do not close dbi handle

    // commit and close active transaction
    int rc = mdb_txn_commit(resources->txn);
    if (rc != 0) {
      // no disk, no memory, etc
      std::cerr << "lmdb commit failure txn_commit " << rc << "\n";
      asert(0);
    }

    // remove txn from set
    MUTEX_LOCK(&M);
    pthread_resource_set.erase(resources);
    MUTEX_UNLOCK(&M);
  }

  // helper
  // NOTE: A transaction must not be open for this thread when calling this.
  void increase_map_size() {
    // increase map size by at last half again more
    MDB_env* env_info;
    mdb_env_info(env, env_info);
    const size_t size = env_info->mapsize;
    size_t newsize = size*2 - size/2
    newsize = size + 10485760 - (size % 10485760)
#ifdef DEBUG
    std::cout << "lmdb insert increase from " << size << " to " << newsize << "\n";
#endif
    int rc = mdb_env_set_mapsize(env, newsize);
    if (rc != 0) {
      std::cerr << "lmdb commit failure set_mapsize " << rc << "\n";
      assert(0);
    }
  }

  // inset protecting against infinite invocation recursion
  void insert(const hash_t& hash, uint64_t encoding, int iterate_count) {
    if (iterate_count>2) {
      std::cerr << "lmdb insert recursion failure.\n";
      assert(0);
    }
    int rc;
    // get resources for the pthread
    pthread_resources_t resources =
                          get_pthread_resources(resources->env, file_mode);

    // allocate space for the key and data
    MDB_val key;
    MDB_val data;

    // set values into the key and data
    pair_to_hash_store(hash, encoding, key, data);

    // put
    int rc = mdb_put(pthread_txn, dbi, &key, &data, MDB_NODUPDATA);
    if (rc == MDB_KEYEXIST) {
      std::cerr << "lmdb commit failure keyexist " << rc << "\n";
      assert(0);
    } else if (rc == MDB_MAP_FULL) {

      // commit and close active resources
      commit_and_close_resources(resources);

      // increase map size
      increase_map_size();

      // open new resources for thread
      resources = get_pthread_txn(MDB_env* env, int txn_flags);

      // try again with larger map
      insert(hash, encoding, ++iterate_count);

    } else if (rc == MDB_TXN_FULL) {

      // commit and close active transaction
      commit_and_close_resources(resources);

      // open new resources for thread
      resources = get_pthread_txn(MDB_env* env, int txn_flags);

      // try again with empty transaction history
      insert(hash, encoding, ++iterate_count);
    } else {
      std::cerr << "lmdb commit failure, unexpected error " << rc << "\n";
      assert(0);
    }
  }

  public:
  lmdb_hash_store_manager_t(const std::string& p_hashdb_dir,
                            file_mode_type_t p_file_mode) :
                   hashdb_dir(p_hashdb_dir), file_mode(p_file_mode), env(0) {

    const std::string db_name = hashdb_dir + "/hash_store"

    if (env != NULL) {
      // program error, already opened
      assert(0);
    }

    int rc;

    // *env
    rc = mdb_env_create(&env);
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

    // open the DB
    unsigned int flags
    switch(file_mode) {
      case READ_ONLY:
        flags = MDB_RDONLY | MDB_DUPSORT;
        break;
      case RW_NEW:
        if (access(db_name.c_str(), F_OK) == 0) {
          // fail
          std::cerr << "Error: Database '" << db_name << "' already exists.  Aborting.\n";
          exit(1);
        }
        flags = MDB_DUPSORT;
        break;
      case RW_NEW:
        flags = MDB_DUPSORT;
    }

    rc = mdb_env_open(env, db_name.c_str(), flags, 0664);
    if (rc != 0) {
      // fail
      std::cerr << "Error opening database: " << rc << ".  Aborting.\n";
      exit(1);
    }
  }
  
  // insert
  void insert(const hash_t& hash, uint64_t encoding) {
    // call insert with recursion check
    insert(hash, encoding, 0);
  }

  // lower_bound
  lmdb_hash_store_iterator_t lower_bound(hash_t hash) {
    return lmdb_hash_store_iterator_t(resources, hash, true);
  }

  // equal_range
  std::pair<lmdb_hash_store_iterator_t, lmdb_hash_store_iterator_t>
                             equal_range(hash_t hash) {
    return std::pair<lmdb_hash_store_iterator_t, lmdb_hash_store_iterator_t>(
                   lmdb_hash_store_iterator_t(resources, hash, true),
                   lmdb_hash_store_iterator_t(resources, hash, false));
  }

  lmdb_hash_store_iterator_t begin() {
    return lmdb_hash_store_iterator_t(resources, true);
  }

  lmdb_hash_store_iterator_t end() {
    return lmdb_hash_store_iterator_t(resources, false);
  }

  size_t size() {
//zz get information then print size?
  }

};

#endif

