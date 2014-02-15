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
 * Provides services for accessing hashdb as a whole.
 * Hides map_multimap_manager and bloom filter.
 * Notes hashdb changes in provided hashdb_changes.
 */

#ifndef HASHDB_MANAGER_HPP
#define HASHDB_MANAGER_HPP
#include "file_modes.h"
#include "hashdigest_types.h"
#include "map_multimap_manager.hpp"
#include "map_multimap_iterator.hpp"
#include "hashdb_iterator.hpp"
#include "hashdigest.hpp" // for string style hashdigest and hashdigest type
#include "hashdigest_types.h"
#include "hashdb_settings.hpp"
#include "hashdb_settings_manager.hpp"

class hashdb_manager_t {
  private:
  const std::string hashdb_dir;
  const file_mode_type_t file_mode;
  const hashdb_settings_t settings;

  source_lookup_index_manager_t source_lookup_index_manager;
  const hashdb_element_lookup_t hashdb_element_lookup;
  map_multimap_manager_t<md5_t>* md5_manager;
  map_multimap_manager_t<sha1_t>* sha1_manager;
  map_multimap_manager_t<sha256_t>* sha256_manager;

  public:
  hashdb_manager_t(const std::string& p_hashdb_dir, file_mode_type_t p_file_mode) :
                hashdb_dir(p_hashdb_dir),
                file_mode(p_file_mode),
                settings(hashdb_settings_manager_t::read_settings(hashdb_dir)),
                source_lookup_index_manager(hashdb_dir, file_mode),
                hashdb_element_lookup(&source_lookup_index_manager,
                                      &settings),
                md5_manager(0),
                sha1_manager(0),
                sha256_manager(0) {
std::cout << "hashdb_manager.a\n";

    // initialize the map_multimap_manager appropriate for the settings
    switch(settings.hashdigest_type) {
      case HASHDIGEST_MD5:
std::cout << "hashdb_manager.b\n";
        md5_manager = new map_multimap_manager_t<md5_t>(hashdb_dir, file_mode);
        return;
      case HASHDIGEST_SHA1:
        sha1_manager = new map_multimap_manager_t<sha1_t>(hashdb_dir, file_mode);
        return;
      case HASHDIGEST_SHA256:
        sha256_manager = new map_multimap_manager_t<sha256_t>(hashdb_dir, file_mode);
        return;
      default: assert(0);
    }
std::cout << "hashdb_manager.c\n";
  }

  ~hashdb_manager_t() {
    switch(settings.hashdigest_type) {
      case HASHDIGEST_MD5: delete md5_manager; return;
      case HASHDIGEST_SHA1: delete sha1_manager; return;
      case HASHDIGEST_SHA256: delete sha256_manager; return;
      default: assert(0);
    }
  }

  // insert
  void insert(const hashdb_element_t& hashdb_element, hashdb_changes_t& changes) {
    // validate the hashdigest type
    if (hashdb_element.hashdigest_type != hashdigest_type_to_string(settings.hashdigest_type)) {
      ++changes.hashes_not_inserted_wrong_hashdigest_type;
      return;
    }

    // validate block size
    if (hashdb_element.hash_block_size != settings.hash_block_size) {
      ++changes.hashes_not_inserted_wrong_hash_block_size;
      return;
    }

    // validate the file offset
    if (hashdb_element.file_offset % settings.hash_block_size != 0) {
      ++changes.hashes_not_inserted_file_offset_not_aligned;
      return;
    }

    // checks passed, insert or find reason not to insert

    // acquire existing or new source lookup index
    std::pair<bool, uint64_t> lookup_pair =
         source_lookup_index_manager.insert(hashdb_element.repository_name,
                                            hashdb_element.filename);
    uint64_t source_lookup_index = lookup_pair.second;

    // compose the source lookup encoding
    uint64_t encoding = source_lookup_encoding::get_source_lookup_encoding(
                       settings.source_lookup_index_bits,
                       source_lookup_index,
                       hashdb_element.file_offset / settings.hash_block_size);

    // insert or note reason not to insert
    switch(settings.hashdigest_type) {
      case HASHDIGEST_MD5:
        md5_manager->emplace(md5_t::fromhex(hashdb_element.hashdigest),
                             encoding,
                             settings.maximum_hash_duplicates,
                             changes);
        return;
      case HASHDIGEST_SHA1:
        sha1_manager->emplace(sha1_t::fromhex(hashdb_element.hashdigest),
                             encoding,
                             settings.maximum_hash_duplicates,
                             changes);
        return;
      case HASHDIGEST_SHA256:
        sha256_manager->emplace(sha256_t::fromhex(hashdb_element.hashdigest),
                             encoding,
                             settings.maximum_hash_duplicates,
                             changes);
        return;
      default: assert(0);
    }
  }

  // remove
  void remove(const hashdb_element_t& hashdb_element, hashdb_changes_t& changes) {
    // validate the hashdigest type
    if (hashdb_element.hashdigest_type != hashdigest_type_to_string(settings.hashdigest_type)) {
      ++changes.hashes_not_removed_wrong_hashdigest_type;
      return;
    }

    // validate block size
    if (hashdb_element.hash_block_size != settings.hash_block_size) {
      ++changes.hashes_not_removed_wrong_hash_block_size;
      return;
    }

    // validate the file offset
    if (hashdb_element.file_offset % settings.hash_block_size != 0) {
      ++changes.hashes_not_removed_file_offset_not_aligned;
      return;
    }

    // find source lookup index
    std::pair<bool, uint64_t> lookup_pair =
         source_lookup_index_manager.find(hashdb_element.repository_name,
                                          hashdb_element.filename);
    if (lookup_pair.first == false) {
      ++changes.hashes_not_removed_no_element; // because there was no source
      return;
    }

    uint64_t source_lookup_index = lookup_pair.second;

    // compose the source lookup encoding
    uint64_t encoding = source_lookup_encoding::get_source_lookup_encoding(
                       settings.source_lookup_index_bits,
                       source_lookup_index,
                       hashdb_element.file_offset / settings.hash_block_size);

    // checks passed, remove or find reason not to remove
    switch(settings.hashdigest_type) {
      case HASHDIGEST_MD5:
        md5_manager->remove(md5_t::fromhex(hashdb_element.hashdigest),
                            encoding,
                            changes);
        return;
      case HASHDIGEST_SHA1:
        sha1_manager->remove(sha1_t::fromhex(hashdb_element.hashdigest),
                            encoding,
                            changes);
        return;
      case HASHDIGEST_SHA256:
        sha256_manager->remove(sha256_t::fromhex(hashdb_element.hashdigest),
                            encoding,
                            changes);
        return;
      default: assert(0);
    }
  }

  // remove key
  void remove_key(const hashdigest_t& hashdigest, hashdb_changes_t& changes) {
    // validate the hashdigest type
    if (hashdigest.hashdigest_type != hashdigest_type_to_string(settings.hashdigest_type)) {
      ++changes.hashes_not_removed_wrong_hashdigest_type;
      return;
    }

    switch(settings.hashdigest_type) {
      case HASHDIGEST_MD5:
        md5_manager->remove_key(md5_t::fromhex(hashdigest.hashdigest),
                            changes);
        return;
      case HASHDIGEST_SHA1:
        sha1_manager->remove_key(sha1_t::fromhex(hashdigest.hashdigest),
                            changes);
        return;
      case HASHDIGEST_SHA256:
        sha256_manager->remove_key(sha256_t::fromhex(hashdigest.hashdigest),
                            changes);
        return;
      default: assert(0);
    }
  }

  // has key
  bool has_key(const hashdigest_t& hashdigest) {
    // validate the hashdigest type
    if (hashdigest.hashdigest_type != hashdigest_type_to_string(settings.hashdigest_type)) {
      return false;
    }

    switch(settings.hashdigest_type) {
      case HASHDIGEST_MD5:
        return md5_manager->has_key(md5_t::fromhex(hashdigest.hashdigest));
      case HASHDIGEST_SHA1:
        return sha1_manager->has_key(sha1_t::fromhex(hashdigest.hashdigest));
      case HASHDIGEST_SHA256:
        return sha256_manager->has_key(sha256_t::fromhex(hashdigest.hashdigest));
      default: assert(0);
    }
  }

  // begin
  hashdb_iterator_t begin() const {
    switch(settings.hashdigest_type) {
      case HASHDIGEST_MD5:
        return hashdb_iterator_t(md5_manager->begin(),
                                              hashdb_element_lookup);
      case HASHDIGEST_SHA1:
        return hashdb_iterator_t(sha1_manager->begin(),
                                              hashdb_element_lookup);
      case HASHDIGEST_SHA256:
        return hashdb_iterator_t(sha256_manager->begin(),
                                              hashdb_element_lookup);
      default: assert(0);
    }
  }

  // end
  hashdb_iterator_t end() const {
    switch(settings.hashdigest_type) {
      case HASHDIGEST_MD5:
        return hashdb_iterator_t(md5_manager->end(),
                                              hashdb_element_lookup);
      case HASHDIGEST_SHA1:
        return hashdb_iterator_t(sha1_manager->end(),
                                              hashdb_element_lookup);
      case HASHDIGEST_SHA256:
        return hashdb_iterator_t(sha256_manager->end(),
                                              hashdb_element_lookup);
      default: assert(0);
    }
  }

  // quick easy statistic
  size_t map_size() const {
    switch(settings.hashdigest_type) {
      case HASHDIGEST_MD5:
        return md5_manager->map_size();
      case HASHDIGEST_SHA1:
        return sha1_manager->map_size();
      case HASHDIGEST_SHA256:
        return sha256_manager->map_size();
      default: assert(0);
    }
  }

  // quick easy statistic
  size_t multimap_size() const {
    switch(settings.hashdigest_type) {
      case HASHDIGEST_MD5:
        return md5_manager->multimap_size();
      case HASHDIGEST_SHA1:
        return sha1_manager->multimap_size();
      case HASHDIGEST_SHA256:
        return sha256_manager->multimap_size();
      default: assert(0);
    }
  }

};

#endif

