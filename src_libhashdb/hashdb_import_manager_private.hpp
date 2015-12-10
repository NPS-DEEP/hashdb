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
 * Manage the LMDB hash store.
 *
 * Lock non-thread-safe interfaces before use.
 */

#ifndef HASHDB_IMPORT_MANAGER_PRIVATE_HPP
#define HASHDB_IMPORT_MANAGER_PRIVATE_HPP
#include "globals.hpp"
#include "file_modes.h"
#include "hashdb_settings.hpp"
#include "hashdb_settings_store.hpp"
//#include "lmdb.h"
#include "lmdb_typedefs.h"
//#include "lmdb_helper.h"
#include "lmdb_hash_manager.hpp"
#include "lmdb_hash_label_manager.hpp"
#include "lmdb_source_id_manager.hpp"
#include "lmdb_source_metadata_manager.hpp"
#include "lmdb_source_name_manager.hpp"
//#include "bloom_filter_manager.hpp"
#include "logger.hpp"
#include "hashdb_changes.hpp"
#include <vector>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <string>

// no concurrent changes
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "mutex_lock.hpp"

/**
 * Manage all LMDB updates.  All interfaces are locked.
 * Several types of change events are noted in hashdb_changes_t.
 * A logger is opened for logging the command (TBD) and for logging
 * timestamps.  Upon closure, changes are written to the logger
 * and the logger is closed.
 */
class hashdb_import_manager_private_t {

  private:
  std::string hashdb_dir;
  std::string whitelist_hashdb_dir;
  bool import_low_entropy;

  // LMDB managers
  lmdb_hash_manager_t            hash_manager;
  lmdb_hash_label_manager_t      hash_label_manager;
  lmdb_source_id_manager_t       source_id_manager;
  lmdb_source_metadata_manager_t source_metadata_manager;
  lmdb_source_name_manager_t     source_name_manager;

  // logger
  logger_t logger;

  // changes
  hashdb_changes_t changes;

#ifdef HAVE_PTHREAD
  mutable pthread_mutex_t M;                  // mutext
#else
  mutable int M;                              // placeholder
#endif

  // do not allow copy or assignment
  hashdb_import_manager_private_t(const hashdb_import_manager_private_t&);
  hashdb_import_manager_private_t& operator=(
                                  const hashdb_import_manager_private_t&);

  public:
  hashdb_import_manager_private_t(const std::string& p_hashdb_dir,
                                  const std::string& p_whitelist_hashdb_dir,
                                  const bool p_import_low_entropy) :
       hashdb_dir(p_hashdb_dir),
       whitelist_hashdb_dir(p_whitelist_hashdb_dir),
       import_low_entropy(p_import_low_entropy),
       hash_manager(hashdb_dir, RW_MODIFY),
       hash_label_manager(hashdb_dir, RW_MODIFY),
       source_id_manager(hashdb_dir, RW_MODIFY),
       source_metadata_manager(hashdb_dir, RW_MODIFY),
       source_name_manager(hashdb_dir, RW_MODIFY),
       logger(hashdb_dir, "open RW"),
       changes(),
       M() {
    MUTEX_INIT(&M);
  }

  ~hashdb_import_manager_private_t() {
    // log changes and close logger
    logger.add_hashdb_changes(changes);
    logger.add_timestamp("close RW");
    logger.close();
  }

  /**
   * Initialize the environment for this file hash.  Import name if new.
   * Return true if block hashes need to be imported for this file.
   * Return false if block hashes have already been imported for this file.
   */
  bool import_source_name(const std::string& file_binary_hash,
                          const std::string& repository_name,
                          const std::string& filename) {

    MUTEX_LOCK(&M);

    // add this repository name, filename entry
    source_name_manager.insert(file_binary_hash, repository_name, filename);

    // begin a source metadata entry
    std::pair<bool, uint64_t> pair = source_metadata_manager.insert_begin(
                                                  file_binary_hash);

    // add this source_id into the key=source_id, data=file_binary_hash store
    source_id_manager.insert(pair.second, file_binary_hash);

    MUTEX_UNLOCK(&M);

    // true if block hashes need imported, false otherwise
    return pair.first;
  }

  /**
   * Import hashes from hash_data_list.  Fail on error.
   *
   * If import_low_entropy is false then skip hashes with entropy flags.
   * In the future, a non-entropy data classifier flag may be added
   * to hash_data_t.
   *
   */
  void import_source_hashes(const std::string& file_binary_hash,
                            const uint64_t filesize,
                            const hash_data_list_t& hash_data_list) {

    MUTEX_LOCK(&M);

    // get source ID for these hashes
    source_metadata_t source_metadata = source_metadata_manager.find(
                                                        file_binary_hash);

    // process each hash data entry
    for(hash_data_list_t::const_iterator it=hash_data_list.begin();
                                       it != hash_data_list.end(); ++it) {

      // skip low entropy
      if (import_low_entropy == false && it->entropy_label != "") {
        changes.hashes_not_inserted_skip_low_entropy++;
        continue;
      }

      // zzzzzzzzzzzzzzzzzz skip whitelist zzzzzzzzzzzzzzzzz

      // insert hash
      hash_manager.insert(source_metadata.source_id, *it, changes);

      // insert hash label
      if (it->entropy_label != "") {
        hash_label_manager.insert(it->binary_hash, it->entropy_label);
      }
    }

    MUTEX_UNLOCK(&M);
  }

  // Sizes of LMDB databases.
  std::string size() const {
    MUTEX_LOCK(&M);

    std::stringstream ss;
    ss << "hash:" << hash_manager.size()
       << "hash_label:" << hash_label_manager.size()
       << "source_id:" << source_id_manager.size()
       << "source_metadata:" << source_metadata_manager.size()
       << "source_name:" << source_name_manager.size();

    MUTEX_UNLOCK(&M);

    return ss.str();
  }
};

#endif

