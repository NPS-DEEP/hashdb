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
 * Manage LMDB resources in an optimized way.
 * Usage: open, use, then close resources.
 *   - for RO: keeps resources active for the thread.
 *   - for RW: open, use, then close protected by lock, and can grow DB.
 */

#ifndef LMDB_RESOURCES_MANAGER_HPP
#define LMDB_RESOURCES_MANAGER_HPP
#include "lmdb.h"
#include "lmdb_resources.h"
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "mutex_lock.hpp" // for managing pthread resources

class lmdb_resources_manager_t {
  private:

  mutable pthread_key_t pthread_resources_key;
  mutable std::set<lmdb_resources_t*> pthread_resource_set;
  lmdb_resources_t rw_resources;
  const file_mode_type_t file_mode;
  MDB_env* env;
#ifdef HAVE_PTHREAD
  mutable pthread_mutex_t M;                  // mutext
#else
  mutable int M;                              // placeholder
#endif

  // helper
  // helper, txn must not be active, callee must use lock
  void maybe_grow() {
    // http://comments.gmane.org/gmane.network.openldap.technical/11699

    // read environment info
    MDB_envinfo env_info;
    int rc = mdb_env_info(env, &env_info);
    if (rc != 0) {
      assert(0);
    }

    // get page size
    MDB_stat ms;
    rc = mdb_env_stat(env, &ms);
    if (rc != 0) {
      assert(0);
    }

    // maybe grow the DB
    if (env_info.me_mapsize / ms.ms_psize == env_info.me_last_pgno + 2) {

      // full so grow the DB, safe since this code is locked
#ifdef DEBUG
      std::cout << "Growing hash store DB from " << env_info.me_mapsize
                << " to " << env_info.me_mapsize * 2 << "\n";
#endif

      // could call mdb_env_sync(env, 1) here but it does not help

      // grow the DB
      rc = mdb_env_set_mapsize(env, env_info.me_mapsize * 2);
      if (rc != 0) {
        // grow failed
        std::cerr << "Error growing the hash store DB: " <<  mdb_strerror(rc)
                  << "\nAborting.\n";
        exit(1);
      }
    }
  }

  public:
  lmdb_resources_manager_t(file_mode_type_t p_file_mode, MDB_env* p_env) :
                        pthread_resources_key(),
                        pthread_resource_set(),
                        rw_resources(),
                        file_mode(p_file_mode),
                        env(p_env),
                        M() {
    MUTEX_INIT(&M);

    // create the pthread key for this resource
    int status = pthread_key_create(&pthread_resources_key, NULL);
    if (status != 0) {
      std::cerr << "pthread failure\n";
      assert(0);
    }
  }

#ifdef HAVE_CXX11
  // fix compiler warnings for compatible compilers
  lmdb_resources_manager_t(const lmdb_resources_manager_t& other) = delete;
  lmdb_resources_manager_t& operator=(const lmdb_resources_manager_t& other) = delete;
#endif

  ~lmdb_resources_manager_t() {
    if (file_mode == READ_ONLY) {
      // close resources that were opened for each thread
      MUTEX_LOCK(&M);
      for (std::set<lmdb_resources_t*>::iterator it =
                 pthread_resource_set.begin();
                 it != pthread_resource_set.end();
                 ++it) {

        // close selected resource
        lmdb_resources_t* resources = *it;
#ifdef DEBUG
        std::cout << "commit_and_close_all_resources, resource: " << resources
                  << "\n";
#endif

        // free cursor
        mdb_cursor_close(resources->cursor);

        // do not close dbi handle, why not close it?

        // free transaction
        mdb_txn_abort(resources->txn);

        // remove resources from pthread resources key
        pthread_setspecific(pthread_resources_key, NULL);

        // remove resources
        delete resources;
      }

      pthread_resource_set.clear();
      MUTEX_UNLOCK(&M);

    } else {
      // resources are not left open in RW mode
    }
  }

  /**
   * Open locked resources when in RW mode.
   */
  lmdb_resources_t* open_rw_resources() {
    // mode must not be RO
    if (file_mode == READ_ONLY) {
      assert(0);
    }

    // other threads must wait until this resource is clsoed
    MUTEX_LOCK(&M);

    // first, see if DB needs to grow
    maybe_grow();

    // create the txn object
    int rc = mdb_txn_begin(env, NULL, 0, &rw_resources.txn);
    if (rc != 0) {
      std::cerr << "LMDB txn error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    // create the database handle integer
    unsigned int dbi_flags = MDB_DUPSORT | MDB_DUPFIXED | MDB_CREATE;
    rc = mdb_dbi_open(rw_resources.txn, NULL, dbi_flags, &rw_resources.dbi);
    if (rc != 0) {
      std::cerr << "LMDB dbi error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    // create a cursor object to use with this txn
    rc = mdb_cursor_open(rw_resources.txn,
                         rw_resources.dbi,
                         &rw_resources.cursor);
    if (rc != 0) {
      std::cerr << "LMDB cursor error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    return &rw_resources;
  }

  /**
   * Close locked resources when in RW mode.
   */
  void close_rw_resources() {
    // mode must not be RO
    if (file_mode == READ_ONLY) {
      assert(0);
    }

    // free cursor
    mdb_cursor_close(rw_resources.cursor);

    // do not close dbi handle, why not close it?

    // close transaction
    mdb_txn_commit(rw_resources.txn);

    // free the lock
    MUTEX_UNLOCK(&M);
  }

  /**
   * Get thread-specific resources when in RO mode.
   */
  lmdb_resources_t* get_ro_resources() {
    // mode must be RO
    if (file_mode != READ_ONLY) {
      assert(0);
    }

    // get resources, possibly creating them
    lmdb_resources_t* pthread_resources;
    if (pthread_getspecific(pthread_resources_key) == NULL) {
      pthread_resources = new lmdb_resources_t;

      // create the thread-specific txn object
      int rc = mdb_txn_begin(env, NULL, MDB_RDONLY, &pthread_resources->txn);
      if (rc != 0) {
        std::cerr << "LMDB txn error: " << mdb_strerror(rc) << "\n";
        assert(0);
      }

      // create the database handle integer
      unsigned int dbi_flags = MDB_DUPSORT | MDB_DUPFIXED;
      rc = mdb_dbi_open(pthread_resources->txn, NULL, dbi_flags, &pthread_resources->dbi);
      if (rc != 0) {
        std::cerr << "LMDB dbi error: " << mdb_strerror(rc) << "\n";
        assert(0);
      }

      // create a cursor object to use with this txn
      rc = mdb_cursor_open(pthread_resources->txn,
                           pthread_resources->dbi,
                           &pthread_resources->cursor);
      if (rc != 0) {
        std::cerr << "LMDB cursor error: " << mdb_strerror(rc) << "\n";
        assert(0);
      }

      // bind the pthread variable to this pthread key
      pthread_setspecific(pthread_resources_key, pthread_resources);

      // save pointer for destructor
      MUTEX_LOCK(&M);
#ifdef DEBUG
      std::cout << "get_pthread_resources: " << pthread_resources << "\n";
#endif
      pthread_resource_set.insert(pthread_resources);
      MUTEX_UNLOCK(&M);

    } else {
      // get the stored thread-specific resources
      pthread_resources = static_cast<lmdb_resources_t*>(
                                 pthread_getspecific(pthread_resources_key));
    }

    return pthread_resources;
  }

  /**
   * convenience method
   */
  lmdb_resources_t* open_resources() {
    if (file_mode == READ_ONLY) {
      return get_ro_resources();
    } else {
      return open_rw_resources();
    }
  }

  /**
   * convenience method
   */
  void close_resources() {
    if (file_mode == READ_ONLY) {
      // no action
    } else {
      close_rw_resources();
    }
  }
};

#endif

