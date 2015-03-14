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
 * Provides services for modifying the DB, including tracking changes.
 *
 * This module provides thread safety by locking every interface,
 * protecting settings and DB integrity since actions read, check state,
 * then write.
 */

#ifndef LMDB_RW_MANAGER_HPP
#define LMDB_RW_MANAGER_HPP
#include "hashdb_settings.hpp"
#include "hashdb_changes.hpp"
#include "globals.hpp"
#include <vector>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include "lmdb_hash_store.hpp"
#include "lmdb_hash_it_data.hpp"
#include "lmdb_name_store.hpp"
#include "lmdb_source_store.hpp"
#include "lmdb_source_data.hpp"
#include "lmdb_source_it_data.hpp"
#include "lmdb_helper.h"
#include "bloom_filter_manager.hpp"
#include "hashdb_settings_store.hpp"

// no concurrent changes
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "mutex_lock.hpp"

class lmdb_rw_manager_t {

  private:
  const std::string hashdb_dir;
  public:
  const hashdb_settings_t settings;
  hashdb_changes_t changes;
  private:

  // bloom filter manager
  bloom_filter_manager_t bloom_filter_manager;

  // DB stores
  lmdb_hash_store_t hash_store;
  lmdb_name_store_t name_store;
  lmdb_source_store_t source_store;

  // do not allow copy or assignment
  lmdb_rw_manager_t(const lmdb_rw_manager_t&);
  lmdb_rw_manager_t& operator=(const lmdb_rw_manager_t&);

#ifdef HAVE_PTHREAD
  mutable pthread_mutex_t M;                  // mutext
#else
  mutable int M;                              // placeholder
#endif

  public:
  lmdb_rw_manager_t(const std::string& p_hashdb_dir) :

          hashdb_dir(p_hashdb_dir),
          settings(hashdb_settings_store_t::read_settings(hashdb_dir)),
          changes(),
          bloom_filter_manager(hashdb_dir,
                               RW_MODIFY,
                               settings.hash_truncation,
                               settings.bloom_is_used,
                               settings.bloom_M_hash_size,
                               settings.bloom_k_hash_functions),
          hash_store(hashdb_dir, RW_MODIFY,
                     settings.byte_alignment,
                     settings.hash_truncation),
          name_store(hashdb_dir, RW_MODIFY),
          source_store(hashdb_dir, RW_MODIFY),
          M() {
    MUTEX_INIT(&M);
  }

  void insert(const std::string& binary_hash,
              uint64_t file_offset,
              uint32_t hash_block_size,
              lmdb_source_data_t source_data,
              const std::string& hash_label) {

    MUTEX_LOCK(&M);

    // validate the byte alignment
    if (file_offset % settings.byte_alignment != 0) {
      ++changes.hashes_not_inserted_invalid_byte_alignment;
      MUTEX_UNLOCK(&M);
      return;
    }

    // validate block size
    if (settings.hash_block_size != 0 &&
        (hash_block_size != settings.hash_block_size)) {
      ++changes.hashes_not_inserted_mismatched_hash_block_size;
      MUTEX_UNLOCK(&M);
      return;
    }

    // acquire existing or new source lookup index
    const std::pair<bool, uint64_t> lookup_pair =
         name_store.insert(source_data.repository_name, source_data.filename);
    const uint64_t source_lookup_index = lookup_pair.second;

    // if the hash may exist then check against duplicates and max count
    if (bloom_filter_manager.is_positive(binary_hash)) {

      // disregard if key, value exists
      if (hash_store.find(binary_hash,
                          source_lookup_index,
                          file_offset,
                          hash_label)) {
        // this exact entry already exists
        ++changes.hashes_not_inserted_duplicate_element;
        MUTEX_UNLOCK(&M);
        return;
      }

      // disregard if above max duplicates
      if (settings.maximum_hash_duplicates > 0) {
        const size_t count = hash_store.find_count(binary_hash);
        if (count >= settings.maximum_hash_duplicates) {
          // at maximum for this hash
          ++changes.hashes_not_inserted_exceeds_max_duplicates;
          MUTEX_UNLOCK(&M);
          return;
        }
      }
    }

    // add the entry since all the checks passed
    hash_store.insert(binary_hash,
                      source_lookup_index,
                      file_offset,
                      hash_label);
    ++changes.hashes_inserted;

    // add source data in case it isn't there yet
    source_store.add(source_lookup_index, source_data);

    // add hash to bloom filter, too, even if already there
    bloom_filter_manager.add_hash_value(binary_hash);

    MUTEX_UNLOCK(&M);
  }

  // remove
  void remove(const std::string& binary_hash,
              uint64_t file_offset,
              uint32_t hash_block_size,
              lmdb_source_data_t source_data,
              const std::string& hash_label) {

    MUTEX_LOCK(&M);

    // validate the byte alignment
    if (file_offset % settings.byte_alignment != 0) {
      ++changes.hashes_not_removed_invalid_byte_alignment;
      MUTEX_UNLOCK(&M);
      return;
    }

    // validate hash block size
    if (settings.hash_block_size != 0 &&
        (hash_block_size != settings.hash_block_size)) {
      ++changes.hashes_not_removed_mismatched_hash_block_size;
      MUTEX_UNLOCK(&M);
      return;
    }

    // find source lookup index
    std::pair<bool, uint64_t> lookup_pair =
         name_store.find(source_data.repository_name, source_data.filename);
    if (lookup_pair.first == false) {
      ++changes.hashes_not_removed_no_element; // because there was no source
      MUTEX_UNLOCK(&M);
      return;
    }
    uint64_t source_lookup_index = lookup_pair.second;

    // remove the distinct identified element
    bool did_erase = hash_store.erase(binary_hash,
                                      source_lookup_index,
                                      file_offset,
                                      hash_label);
    if (did_erase) {
      ++changes.hashes_removed;
    } else {
      // the key with the source lookup encoding was not found
      ++changes.hashes_not_removed_no_element;
    }

    MUTEX_UNLOCK(&M);
  }

  // remove hash
  void remove_hash(const std::string& binary_hash) {

    MUTEX_LOCK(&M);

    // erase elements of hash
    uint32_t count = hash_store.erase(binary_hash);
    
    if (count == 0) {
      // no hash
      ++changes.hashes_not_removed_no_hash;
    } else {
      changes.hashes_removed += count;
    }

    MUTEX_UNLOCK(&M);
  }

  // add source data
  void add_source_data(const lmdb_source_data_t& source_data) {

    // repository name and filename are required
    if (source_data.repository_name == "" ||
        source_data.filename == "") {
      // invalid input
      std::cerr << "Invalid input, name required.\n";
      return;
    }

    MUTEX_LOCK(&M);

    // get the source lookup index, possibly creating it
    std::pair<bool, uint64_t> pair = name_store.insert(
              source_data.repository_name, source_data.filename);

    // add the source data
    source_store.add(pair.second, source_data);

    MUTEX_UNLOCK(&M);
  }

  size_t size() const {
    MUTEX_LOCK(&M);
    size_t hash_store_size = hash_store.size();
    MUTEX_UNLOCK(&M);
    return hash_store_size;
  }
};

#endif

