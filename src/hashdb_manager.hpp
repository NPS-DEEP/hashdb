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
#include "map_multimap_manager.hpp"
#include "map_multimap_iterator.hpp"
#include "hashdb_settings.hpp"
#include "hashdb_settings_manager.hpp"
#include "source_lookup_index_manager.hpp"
#include "hashdb_element_lookup.hpp"
#include "hashdb_iterator.hpp"

template<typename T>  // hash type
class hashdb_manager_t {
  private:
  const std::string hashdb_dir;
  const file_mode_type_t file_mode;
  const hashdb_settings_t settings;

  source_lookup_index_manager_t source_lookup_index_manager;
  const hashdb_element_lookup_t<T> hashdb_element_lookup;
  map_multimap_manager_t<T> map_multimap_manager;

  // do not allow copy or assignment
  hashdb_manager_t(const hashdb_manager_t&);
  hashdb_manager_t& operator=(const hashdb_manager_t&);

  public:
  hashdb_manager_t(const std::string& p_hashdb_dir, file_mode_type_t p_file_mode) :
                hashdb_dir(p_hashdb_dir),
                file_mode(p_file_mode),
                settings(hashdb_settings_manager_t::read_settings(hashdb_dir)),
                source_lookup_index_manager(hashdb_dir, file_mode),
                hashdb_element_lookup(&source_lookup_index_manager,
                                      &settings),
                map_multimap_manager(hashdb_dir, file_mode) {
  }

  // insert
  void insert(const hashdb_element_t<T>& hashdb_element, hashdb_changes_t& changes) {

    // validate block size
    if (hashdb_element.hash_block_size != settings.hash_block_size) {
      ++changes.hashes_not_inserted_mismatched_hash_block_size;
      return;
    }

    // validate the file offset
    if (hashdb_element.file_offset % settings.hash_block_size != 0) {
      ++changes.hashes_not_inserted_file_offset_not_aligned;
      return;
    }

    // checks passed, insert or have reason not to insert

    // acquire existing or new source lookup index
    std::pair<bool, uint64_t> lookup_pair =
         source_lookup_index_manager.insert(hashdb_element.repository_name,
                                            hashdb_element.filename);
    uint64_t source_lookup_index = lookup_pair.second;

    // compose the source lookup encoding
    uint64_t encoding = source_lookup_encoding::get_source_lookup_encoding(
                       source_lookup_index,
                       hashdb_element.file_offset / settings.hash_block_size);

    // insert or note reason not to insert
    map_multimap_manager.emplace(hashdb_element.key,
                                 encoding,
                                 settings.maximum_hash_duplicates,
                                 changes);
  }

  // remove
  void remove(const hashdb_element_t<T>& hashdb_element, hashdb_changes_t& changes) {

    // validate block size
    if (hashdb_element.hash_block_size != settings.hash_block_size) {
      ++changes.hashes_not_removed_mismatched_hash_block_size;
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
                       source_lookup_index,
                       hashdb_element.file_offset / settings.hash_block_size);

    // checks passed, remove or show reason not to remove
    map_multimap_manager.remove(hashdb_element.key, encoding, changes);
  }

  // remove key
  void remove_key(const T& key, hashdb_changes_t& changes) {
    map_multimap_manager.remove_key(key, changes);
  }

  // find
  std::pair<hashdb_iterator_t<T>, hashdb_iterator_t<T> > find(const T& key) const {

    // get the map_multimap iterator pair
    std::pair<map_multimap_iterator_t<T>,
              map_multimap_iterator_t<T> >
                                it_pair(map_multimap_manager.find(key));

    // return the hashdb_iterator pair for this key
    return std::pair<hashdb_iterator_t<T>, hashdb_iterator_t<T> >(
               hashdb_iterator_t<T>(it_pair.first, hashdb_element_lookup),
               hashdb_iterator_t<T>(it_pair.second, hashdb_element_lookup));
  }

  // find_count
  uint32_t find_count(const T& key) const {
    return map_multimap_manager.find_count(key);
  }

  // begin
  hashdb_iterator_t<T> begin() const {
    return hashdb_iterator_t<T>(map_multimap_manager.begin(),
                                              hashdb_element_lookup);
  }

  // end
  hashdb_iterator_t<T> end() const {
    return hashdb_iterator_t<T>(map_multimap_manager.end(),
                                              hashdb_element_lookup);
  }

  // map size
  size_t map_size() const {
    return map_multimap_manager.map_size();
  }

  // multimap size
  size_t multimap_size() const {
    return map_multimap_manager.map_size();
  }

  // source lookup store size
  size_t source_lookup_store_size() const {
    return source_lookup_index_manager.source_lookup_store_size();
  }

  // repository name lookup store size
  size_t repository_name_lookup_store_size() const {
    return source_lookup_index_manager.repository_name_lookup_store_size();
  }

  // filename lookup store size
  size_t filename_lookup_store_size() const {
    return source_lookup_index_manager.filename_lookup_store_size();
  }
};

#endif

