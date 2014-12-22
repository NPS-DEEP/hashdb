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
 * Manage LMDB resources separately by thread.
 */

#ifndef LMDB_RESOURCE_MANAGER_HPP
#define LMDB_RESOURCE_MANAGER_HPP
#include "lmdb.h"
#include "lmdb_resources.h"
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "mutex_lock.hpp" // for managing pthread resources

class lmdb_resource_manager_t {
  private:

  mutable pthread_key_t pthread_resources_key;
  mutable std::set<pthread_resources_t*> pthread_resource_set;
#ifdef HAVE_PTHREAD
  mutable pthread_mutex_t M;                  // mutext
#else
  mutable int M;                              // placeholder
#endif

  const file_mode_type_t file_mode;
  MDB_env* env;

  void commit_and_close_txn(const pthread_resources_t* resources) const {
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
  }

  public:
  lmdb_resource_manager_t(file_mode_type_t p_file_mode, MDB_env* p_env) :
                        pthread_resources_key(),
                        pthread_resource_set(),
                        M(),
                        file_mode(p_file_mode),
                        env(p_env) {
    MUTEX_INIT(&M);

    // create the pthread key for this resource
    int status = pthread_key_create(&pthread_resources_key, NULL);
    if (status != 0) {
      std::cerr << "pthread failure\n";
      assert(0);
    }
  }

  ~lmdb_resource_manager_t() {
    commit_and_close_all_resources();
  }

#ifdef HAVE_CXX11
  // fix compiler warnings for compatible compilers
  lmdb_resource_manager_t(const lmdb_resource_manager_t& other) = delete;
  lmdb_resource_manager_t& operator=(const lmdb_resource_manager_t& other) = delete;
#endif

  // get resources, possibly creating them
  pthread_resources_t* get_pthread_resources() const {
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

      // create a cursor object to use with this txn
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
#ifdef DEBUG
      std::cout << "get_pthread_resources: " << pthread_resources << "\n";
#endif
      pthread_resource_set.insert(pthread_resources);
      MUTEX_UNLOCK(&M);

    } else {
      // get the stored thread-specific resources
      pthread_resources = static_cast<pthread_resources_t*>(
                                 pthread_getspecific(pthread_resources_key));
    }

    return pthread_resources;
  }

  // close resources for this thread
  void commit_and_close_thread_resources() const {

    // get the stored thread-specific resources
    pthread_resources_t* resources = static_cast<pthread_resources_t*>(
                                 pthread_getspecific(pthread_resources_key));
#ifdef DEBUG
    std::cout << "commit_and_close_thread_resources: " << resources << "\n";
#endif
    if (resources == NULL) {
      // resources not opened, no action
      return;
    }

    commit_and_close_txn(resources);

    // remove resources from pthread resources key
    pthread_setspecific(pthread_resources_key, NULL);

    // remove resources from resource set
    MUTEX_LOCK(&M);
    pthread_resource_set.erase(resources);
    MUTEX_UNLOCK(&M);
  }

  // close resources for all threads
  void commit_and_close_all_resources() const {
    // close resources that were opened for each thread,
    // preserving integrity of pthread_resource_set
    MUTEX_LOCK(&M);
    for (std::set<pthread_resources_t*>::iterator it =
               pthread_resource_set.begin();
               it != pthread_resource_set.end();
               ++it) {

      // close selected resource
      pthread_resources_t* resources = *it;
#ifdef DEBUG
    std::cout << "commit_and_close_all_resources, resource: " << resources << "\n";
#endif
      commit_and_close_txn(resources);

      // remove resources from pthread resources key
      pthread_setspecific(pthread_resources_key, NULL);
    }

    pthread_resource_set.clear();
    MUTEX_UNLOCK(&M);
  }
};

#endif

