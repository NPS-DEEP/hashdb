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
 * Provides services for accessing the multimap, including tracking changes.
 */

#ifndef HASHDB_MANAGER_HPP
#define HASHDB_MANAGER_HPP
#include "file_modes.h"
#include "hashdb_settings.hpp"
#include "hashdb_settings_manager.hpp"
#include "source_lookup_index_manager.hpp"
#include "hashdb_iterator.hpp"
#include "hashdb_changes.hpp"
#include "bloom_filter_manager.hpp"
#include "source_lookup_encoding.hpp"
#include <vector>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <cassert>

template<typename T>  // hash type
class hashdb_manager_t {

  public:
  typedef boost::btree::btree_multimap<T, uint64_t> multimap_t;
  typedef typename multimap_t::const_iterator multimap_iterator_t;
  typedef typename std::pair<multimap_iterator_t, multimap_iterator_t>
                                               multimap_iterator_range_t;

  private:
  const std::string hashdb_dir;
  const file_mode_type_t file_mode;

  // settings
  const hashdb_settings_t settings;

  // multimap
  multimap_t multimap;

  // bloom filter manager
  bloom_filter_manager_t<T> bloom_filter_manager;

  // source lookup support
  source_lookup_index_manager_t source_lookup_index_manager;

  // do not allow copy or assignment
  hashdb_manager_t(const hashdb_manager_t&);
  hashdb_manager_t& operator=(const hashdb_manager_t&);

  public:
  hashdb_manager_t(const std::string& p_hashdb_dir,
                   file_mode_type_t p_file_mode) :
                hashdb_dir(p_hashdb_dir),
                file_mode(p_file_mode),
                settings(hashdb_settings_manager_t::read_settings(hashdb_dir)),
                multimap(hashdb_dir + "/hash_store",
                         file_mode_type_to_btree_flags_bitmask(file_mode)),
                bloom_filter_manager(hashdb_dir, file_mode,
                               settings.bloom1_is_used,
                               settings.bloom1_M_hash_size,
                               settings.bloom1_k_hash_functions,
                               settings.bloom2_is_used,
                               settings.bloom2_M_hash_size,
                               settings.bloom2_k_hash_functions),
                source_lookup_index_manager(hashdb_dir, file_mode) {
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
                       hashdb_element.file_offset);

    // do not emplace if count > max count allowed
    if (settings.maximum_hash_duplicates > 0
             && bloom_filter_manager.is_positive(hashdb_element.key)
             && multimap.count(hashdb_element.key) >= settings.maximum_hash_duplicates) {
      ++changes.hashes_not_inserted_exceeds_max_duplicates;
      return;
    }

    // add the element, even if an identical (key, value) pair is already there
    multimap.emplace(hashdb_element.key, encoding);
    ++changes.hashes_inserted;

    // add hash to bloom filter, too, even if already there
    bloom_filter_manager.add_hash_value(hashdb_element.key);
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
                       hashdb_element.file_offset);

    // find and remove the uniquely identified element
    multimap_iterator_range_t it = multimap.equal_range(hashdb_element.key);
    multimap_iterator_t lower = it.first;
    const multimap_iterator_t upper = it.second;
    for (; lower != upper; ++lower) {
      if (lower->second == encoding) {
        // found it so erase it
        multimap.erase(lower);
        ++changes.hashes_removed;
        return;
      }
    }

    // the key with the source lookup encoding was not found
    ++changes.hashes_not_removed_no_element;
    return;
  }

  // remove key
  void remove_key(const T& key, hashdb_changes_t& changes) {

    // erase elements of key
    uint32_t count = multimap.erase(key);
    
    if (count == 0) {
      // no key
      ++changes.hashes_not_removed_no_hash;
    } else {
      changes.hashes_removed += count;
    }
  }

  // find
  std::pair<hashdb_iterator_t<T>, hashdb_iterator_t<T> > find(const T& key) const {

    // get the multimap iterator pair
    std::pair<multimap_iterator_t, multimap_iterator_t>
                                        it_pair(multimap.equal_range(key));

    // return the hashdb_iterator pair for this key
    return std::pair<hashdb_iterator_t<T>, hashdb_iterator_t<T> >(
               hashdb_iterator_t<T>(&source_lookup_index_manager,
                                    settings.hash_block_size,
                                    it_pair.first),
               hashdb_iterator_t<T>(&source_lookup_index_manager,
                                    settings.hash_block_size,
                                    it_pair.second));
  }

  // find_count
  uint32_t find_count(const T& key) const {
    // if key not in bloom filter then clearly count=0
    if (!bloom_filter_manager.is_positive(key)) {
      // key not present in bloom filter
      return 0;
    } else {
      // return count from multimap
      return multimap.count(key);
    }
  }

  // begin
  hashdb_iterator_t<T> begin() const {
    return hashdb_iterator_t<T>(&source_lookup_index_manager,
                                settings.hash_block_size,
                                multimap.begin());
  }

  // end
  hashdb_iterator_t<T> end() const {
    return hashdb_iterator_t<T>(&source_lookup_index_manager,
                                settings.hash_block_size,
                                multimap.end());
  }

  // multimap size
  size_t map_size() const {
    return multimap.size();
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

