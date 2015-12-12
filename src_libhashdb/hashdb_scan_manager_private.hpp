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
 * Manage scanning the LMDB hash store.
 */

#ifndef HASHDB_SCAN_MANAGER_PRIVATE_HPP
#define HASHDB_SCAN_MANAGER_PRIVATE_HPP
#include "file_modes.h"
#include "hashdb.hpp"
#include "lmdb_hash_manager.hpp"
#include "lmdb_hash_label_manager.hpp"
#include "lmdb_source_id_manager.hpp"
#include "lmdb_source_metadata_manager.hpp"
#include "lmdb_source_name_manager.hpp"
#include <vector>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <string>

/**
 * Manage LMDB scans.
 */
class hashdb_scan_manager_private_t {

  private:
  std::string hashdb_dir;

  // LMDB managers
  lmdb_hash_manager_t            hash_manager;
  lmdb_hash_label_manager_t      hash_label_manager;
  lmdb_source_id_manager_t       source_id_manager;
  lmdb_source_metadata_manager_t source_metadata_manager;
  lmdb_source_name_manager_t     source_name_manager;

  // do not allow copy or assignment
  hashdb_scan_manager_private_t(const hashdb_scan_manager_private_t&);
  hashdb_scan_manager_private_t& operator=(
                                  const hashdb_scan_manager_private_t&);

  public:
  hashdb_scan_manager_private_t(const std::string& p_hashdb_dir) :
       hashdb_dir(p_hashdb_dir),
       hash_manager(hashdb_dir, RW_MODIFY),
       hash_label_manager(hashdb_dir, RW_MODIFY),
       source_id_manager(hashdb_dir, RW_MODIFY),
       source_metadata_manager(hashdb_dir, RW_MODIFY),
       source_name_manager(hashdb_dir, RW_MODIFY) {
  }

  ~hashdb_scan_manager_private_t() {
  }

  /**
   * Find offset pairs associated with this hash.
   * An empty list means no match.
   */
  void find_offset_pairs(const std::string& binary_hash,
                         hashdb::id_offset_pairs_t& id_offset_pairs) const {
    return hash_manager.find(binary_hash, id_offset_pairs);
  }

  /**
   * Find source names associated with this source file's hash.
   * An empty list means no match.
   */
  void find_source_names(const std::string& file_binary_hash,
                         hashdb::source_names_t& source_names) const {
    return source_name_manager.find(file_binary_hash, source_names);
  }

  /**
   * Find source file binary hash from source ID.
   * Fail if the requested source ID is not found.
   */
  std::string find_file_binary_hash(const uint64_t source_id) const {
    return source_id_manager.find(source_id);
  }

  /**
   * Return first hash and its matches.
   * Hash is "" and id_offset_pairs is empty when DB is empty.
   * Please use the heap for id_offset_pairs since it can get large.
   */
  std::string hash_begin(hashdb::id_offset_pairs_t& id_offset_pairs) const {
    return hash_manager.find_begin(id_offset_pairs);
  }

  /**
   * Return next hash and its matches or "" and no pairs if at end.
   * Fail if called and already at end.
   * Please use the heap for id_offset_pairs since it can get large.
   */
  std::string hash_next(const std::string& last_binary_hash,
                        hashdb::id_offset_pairs_t& id_offset_pairs) const {
    return hash_manager.find_next(last_binary_hash, id_offset_pairs);
  }

  /**
   * Return first file_binary_hash and its metadata.
   */
  std::pair<std::string, hashdb::source_metadata_t> source_begin() const {
    return source_metadata_manager.find_begin();
  }

  /**
   * Return next file_binary_hash and its metadata or "" and zeros if at end.
   * Fail if called and already at end.
   */
  std::pair<std::string, hashdb::source_metadata_t> source_next(
                           const std::string& last_file_binary_hash) const {
    return source_metadata_manager.find_next(last_file_binary_hash);
  }

  /**
   * Return sizes of LMDB databases.
   */
  std::string size() const {
    std::stringstream ss;
    ss << "hash:" << hash_manager.size()
       << ", hash_label:" << hash_label_manager.size()
       << ", source_id:" << source_id_manager.size()
       << ", source_metadata:" << source_metadata_manager.size()
       << ", source_name:" << source_name_manager.size();
    return ss.str();
  }

};

#endif

