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
#include "mutex_lock.hpp" // for managing pthread resources

// static resources for supporting thread-specific transaction objects
static pthread_key_t pthread_txn_key;
static pthread_once_t pthread_txn_key_once = PTHREAD_ONCE_INIT;
static std::vector<MDB_txn*> pthread_txns;
static void make_pthread_txn_key() {
  (void) pthread_key_create(&pthread_txn_key, NULL);
}

inline MDB_txn* get_pthread_txn(MDB_env* env, int txn_flags) {
  (void) pthread_once(&pthread_txn_once, make_txn_key);
  if (pthread_getspecific(pthread_txn_key) == NULL) {
    // create the thread-specific txn object
    MDB_txn* pthread_txn;
    int rc = mdb_txn_begin(env, NULL, txn_flags, &txn);
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
    }
    if (rc == MDB_READERS_FULL) {
      // if this becomes a problem, CPUs>126, use mdb_env_set_maxreaders
      assert(0);
    }
    if (rc == ENOMEM) {
      std::cerr << "LMDB transaction error.  No memory.  Aborting.\n";
      assert(0);
    }
    if (rc != 0) {
      std::cerr << "LMDB transaction error.  Unexpected error.  Aborting.\n";
      assert(0);
    }

    // bind the txn to this pthread key
    pthread_setspecific(pthread_txn_key, pthread_txn);

    // save pointer for destructor
    MUTEX_LOCK(&M);
    pthread_txns.push_back(pthread_txn);
    MUTEX_UNLOCK(&M);

  } else {
    // get the stored thread-specific txn object
    pthread_txn = static_cast<MDB_txn*>(pthread_getspecific(pthread_txn_key);
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

  public:
  lmdb_hash_store_manager_t(const std::string& p_hashdb_dir,
                            file_mode_type_t p_file_mode) :
                   hashdb_dir(p_hashdb_dir), file_mode(p_file_mode), env(0) {

    std::string db_name = hashdb_dir + "/hash_store"

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
  
  void insert(hash_t hash, uint64_t encoding) {
    int rc;
    // get the txn for the pthread
    int txn_flags = (file_mode == READ_ONLY) ? MDB_RDONLY : 0;
    MDB_txn* txn = get_pthread_txn(MDB_env* env, int txn_flags);

    // obtain, possibly creating, the database handle integer
    rc = mdb_dbi_open(pthread_txn, NULL, 0, &dbi);
    if (rc == MDB_NOTFOUND) {
      assert(0);
    }
    if (rc == MDB_DBS_FULL) {
      assert(0);
    }
    if (rc != 0) {
      assert(0);
    }

    // allocate space for the key and data
    MDB_val key;
    MDB_val data;

    // set values into the key and data
    pair_to_hash_store(hash, encoding, key, data);

    // put
    rc = mdb_put(txn, dbi, &key, &data, MDB_NOOVERWRITE)
    if (rc == MDB_KEYEXIST) {
      assert(0);
    }
    if (rc == MDB_MAP_FULL) {
      // increase size

    }


};

#endif

