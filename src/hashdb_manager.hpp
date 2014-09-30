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
#include "source_lookup_index_manager.hpp"
#include "hashdb_iterator.hpp"
#include "hashdb_changes.hpp"
#include "bloom_filter_manager.hpp"
#include "source_lookup_encoding.hpp"
#include "source_metadata.hpp"
#include "source_metadata_element.hpp"
#include "source_metadata_manager.hpp"
#include "hash_t_selector.h"
#include <vector>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <cassert>

class hashdb_manager_t {

  public:
  typedef boost::btree::btree_multimap<hash_t, uint64_t> multimap_t;
  typedef multimap_t::const_iterator multimap_iterator_t;
  typedef std::pair<multimap_iterator_t, multimap_iterator_t>
                                               multimap_iterator_range_t;

  const std::string hashdb_dir;
  const file_mode_type_t file_mode;
  const hashdb_settings_t settings;

  private:
  // multimap
  multimap_t multimap;

  // bloom filter manager
  bloom_filter_manager_t bloom_filter_manager;

  // source lookup support
  source_lookup_index_manager_t source_lookup_index_manager;

  // source metadata lookup support
  source_metadata_manager_t source_metadata_manager;

  // do not allow copy or assignment
  hashdb_manager_t(const hashdb_manager_t&);
  hashdb_manager_t& operator=(const hashdb_manager_t&);

  public:
  hashdb_manager_t(const std::string& p_hashdb_dir,
                   file_mode_type_t p_file_mode) :
                hashdb_dir(p_hashdb_dir),
                file_mode(p_file_mode),
                settings(hashdb_settings_store_t::read_settings(hashdb_dir)),
                multimap(hashdb_dir + "/hash_store",
                         file_mode_type_to_btree_flags_bitmask(file_mode)),
                bloom_filter_manager(hashdb_dir, file_mode,
                               settings.bloom1_is_used,
                               settings.bloom1_M_hash_size,
                               settings.bloom1_k_hash_functions,
                               settings.bloom2_is_used,
                               settings.bloom2_M_hash_size,
                               settings.bloom2_k_hash_functions),
                source_lookup_index_manager(hashdb_dir, file_mode),
                source_metadata_manager(hashdb_dir, file_mode) {
  }

  // insert
  void insert(const hashdb_element_t& hashdb_element, hashdb_changes_t& changes) {

    // validate block size
    if (settings.hash_block_size != 0 &&
        (hashdb_element.hash_block_size != settings.hash_block_size)) {
      ++changes.hashes_not_inserted_mismatched_hash_block_size;
      return;
    }

    // validate the byte alignment, see configure.ac for HASHDB_BYTE_ALIGNMENT
    if (hashdb_element.file_offset % HASHDB_BYTE_ALIGNMENT != 0) {
      ++changes.hashes_not_inserted_invalid_byte_alignment;
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

    // if the key may exist then check against duplicates and max count
    if (bloom_filter_manager.is_positive(hashdb_element.key)) {
      size_t count = 0;
      multimap_iterator_t it = multimap.lower_bound(hashdb_element.key);
      while (it != multimap.end() && it->first == hashdb_element.key) {
        if (it->second == encoding) {
          // this exact element already exists
          ++changes.hashes_not_inserted_duplicate_element;
          return;
        }
        ++count;
        ++it;
      }

      // do not exceed max count allowed
      if (settings.maximum_hash_duplicates > 0 &&
                               count >= settings.maximum_hash_duplicates) {
        // at maximum for this hash
        ++changes.hashes_not_inserted_exceeds_max_duplicates;
        return;
      }
    }

    // add the element since all the checks passed
    multimap.emplace(hashdb_element.key, encoding);
    ++changes.hashes_inserted;

    // add hash to bloom filter, too, even if already there
    bloom_filter_manager.add_hash_value(hashdb_element.key);
  }

  // insert source metadata
  void insert_source_metadata(
                 const source_metadata_element_t& source_metadata_element,
                 hashdb_changes_t& changes) {

    // acquire existing or new source lookup index
    std::pair<bool, uint64_t> lookup_pair =
         source_lookup_index_manager.insert(
                                 source_metadata_element.repository_name,
                                 source_metadata_element.filename);
    uint64_t source_lookup_index = lookup_pair.second;

    // create the source metadata element
    source_metadata_t source_metadata(source_lookup_index,
                                      source_metadata_element.file_hash,
                                      source_metadata_element.file_size);

    // insert the metadata into the source metadata store
    bool status = source_metadata_manager.insert(source_metadata);

    // log change
    if (status == true) {
      ++changes.source_metadata_inserted;
    } else {
      ++changes.source_metadata_not_inserted_already_present;
    }
  }

  // remove
  void remove(const hashdb_element_t& hashdb_element, hashdb_changes_t& changes) {

    // validate block size
    if (settings.hash_block_size != 0 &&
        (hashdb_element.hash_block_size != settings.hash_block_size)) {
      ++changes.hashes_not_removed_mismatched_hash_block_size;
      return;
    }

    // validate the byte alignment, see configure.ac for HASHDB_BYTE_ALIGNMENT
    if (hashdb_element.file_offset % HASHDB_BYTE_ALIGNMENT != 0) {
      ++changes.hashes_not_removed_invalid_byte_alignment;
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

    // find and remove the distinct identified element
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
  void remove_key(const hash_t& key, hashdb_changes_t& changes) {

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
  std::pair<hashdb_iterator_t, hashdb_iterator_t > find(const hash_t& key) const {

    // get the multimap iterator pair
    std::pair<multimap_iterator_t, multimap_iterator_t>
                                        it_pair(multimap.equal_range(key));

    // return the hashdb_iterator pair for this key
    return std::pair<hashdb_iterator_t, hashdb_iterator_t >(
               hashdb_iterator_t(&source_lookup_index_manager,
                                 settings.hash_block_size,
                                 it_pair.first),
               hashdb_iterator_t(&source_lookup_index_manager,
                                 settings.hash_block_size,
                                 it_pair.second));
  }

  /**
   * Obtain source metadata given repository name and filename.
   * Return true and metadata else false and empty metadata.
   */
  std::pair<bool, source_metadata_t> find_source_metadata(
                                     const std::string& repository_name,
                                     const std::string& filename) {

    // find the source lookup index associated with repository name and filename
    std::pair<bool, uint64_t> lookup_pair =
         source_lookup_index_manager.find(repository_name, filename);

    if (lookup_pair.first == false) {
      // source lookup index not defined for this lookup pair
      return std::pair<bool, source_metadata_t>(
                                    false, source_metadata_t());
    } else {
      return source_metadata_manager.find(lookup_pair.second);
    }
  }

  // find_count
  uint32_t find_count(const hash_t& key) const {
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
  hashdb_iterator_t begin() const {
    return hashdb_iterator_t(&source_lookup_index_manager,
                             settings.hash_block_size,
                             multimap.begin());
  }

  // end
  hashdb_iterator_t end() const {
    return hashdb_iterator_t(&source_lookup_index_manager,
                             settings.hash_block_size,
                             multimap.end());
  }

  // begin source lookup index iterator
  source_lookup_index_iterator_t begin_source_lookup_index() {
    return source_lookup_index_manager.begin();
  }

  // end source metadata
  source_lookup_index_iterator_t end_source_lookup_index() {
    return source_lookup_index_manager.end();
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

  // source metadata lookup store size
  size_t source_metadata_lookup_store_size() const {
    return source_metadata_manager.size();
  }
};

#endif

