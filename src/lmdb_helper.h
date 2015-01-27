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
 * Provide support for LMDB operations.
 */

#ifndef LMDB_HELPER_H
#define LMDB_HELPER_H
#include "file_modes.h"
#include "lmdb.h"

class lmdb_helper {
  public:
  static MDB_env* open_env(const std::string& store_dir,
                           file_mode_type_t file_mode) {

    // create the DB environment
    MDB_env* env;
    int rc = mdb_env_create(&env);
    if (rc != 0) {
      // bad failure
      assert(0);
    }

    // set flags for open
    unsigned int env_flags
    switch(file_mode) {
      case READ_ONLY:
        env_flags = MDB_RDONLY;
        break;
      case RW_NEW:
        // store directory must not exist yet
        if (access(store_dir.c_str(), F_OK) == 0) {
          std::cerr << "Error: Database '" << store_dir
                    << "' already exists.  Aborting.\n";
          exit(1);
        }

        // create the store directory
#ifdef WIN32
        if(mkdir(store_dir.c_str())){
          std::cerr << "Error: Could not make new store directory '"
                    << store_dir << "'.\nCannot continue.\n";
          exit(1);
        }
#else
        if(mkdir(store_dir.c_str(),0777)){
          std::cerr << "Error: Could not make new store directory '"
                    << store_dir << "'.\nCannot continue.\n";
          exit(1);
        }
#endif
        // NOTE: These flags improve performance significantly so use them.
        // No sync means no requisite disk action after every transaction.
        // writemap suppresses checking but improves Windows performance.
        env_flags = MDB_NOMETASYNC | MDB_NOSYNC | MDB_WRITEMAP;
        break;
      case RW_MODIFY:
        env_flags = MDB_NOMETASYNC | MDB_NOSYNC | MDB_WRITEMAP;
        break;
      default:
        env_flags = 0; // satisfy mingw32-g++ compiler
        assert(0);
    }

    // open the MDB environment
    int rc = mdb_env_open(env, store_dir.c_str(), env_flags, 0664);
    if (rc != 0) {
      // fail
      std::cerr << "Error opening store: " << store_dir
                << ": " <<  mdb_strerror(rc) << "\nAborting.\n";
      exit(1);
    }

    return env;
  }

  static void maybe_grow(MDB_env* env) {
    // http://comments.gmane.org/gmane.network.openldap.technical/11699
    // also see mdb_env_set_mapsize

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
        std::cerr << "Error growing DB: " <<  mdb_strerror(rc)
                  << "\nAborting.\n";
        exit(1);
      }
    }
  }

  // size
  static size_t size() {

    // obtain statistics
    MDB_stat stat;
    int rc = mdb_env_stat(env, &stat);
    if (rc != 0) {
      // program error
      std::cerr << "size failure: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
#ifdef DEBUG
    std::cout << "size: " << stat.ms_entries << "\n";
#endif
    return stat.ms_entries;
  }

  static uint64_t get_uint64_t(const MDB_val* val) {
    if (val->mv_size != sizeof(uint64_t)) {
      // fatal, corrupt DB
      std::cout << "data " << data.mv_size
                << " not " << sizeof(uint64_t) << "\n";
      assert(0);
    }
    const uint64_t num = static_cast<uint64_t*>(val.mv_data);
    return num;
  }

  static inline void point_to_uint64_t(const uint64_t* number, MDB_val* val) {
    val.mv_size = sizeof(uint64_t);
    val.mv_data = const_cast<void*>(static_cast<const void*>(&hash));
  }

  static hash_t get_hash_t(const MDB_val* val) {
    if (val->mv_size != sizeof(hash_t)) {
      // fatal, corrupt DB
      std::cout << "data " << data.mv_size
                << " not " << sizeof(hash_t) << "\n";
      assert(0);
    }
    return hash_t(*static_cast<hash_t*>(key.mv_data));
  }

  static inline void point_to_hash(const hash_t* hash, MDB_val* val) {
    val.mv_size = sizeof(hash_t);
    val.mv_data = const_cast<void*>(static_cast<const void*>(&hash));
  }

  // provide an allocated copy of data that goes away upon loss of scope
  class char_copy_t {
    private:
    const size_t size;
    const char* data;
    // copy sized data
    char_copy_t(size_t p_size, const char*p_data) :
                         size(p_size), data(new char[p_size]) {
      std::memcpy(data, p_data);
    }
    // copy
    char_copy_t(const char_copy_t& other): size(other.size), data(0) {
      std::memcpy(data, other.data);
    }
    // equals
    char_copy_t& operator=(const char_copy_t& other) {
      delete data;
      data = new char[other.size];
      std::memcpy(data, other.data);
    }

    ~char_copy_t() {
      delete char_copy_t;
    }
  }
};

#endif

