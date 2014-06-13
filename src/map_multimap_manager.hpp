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
 * The map_multimap manager provides unified accesses to the hash
 * database as a whole.
 */

#ifndef MAP_MULTIMAP_MANAGER_HPP
#define MAP_MULTIMAP_MANAGER_HPP

#include "hashdb_settings.hpp"
#include "hashdb_settings_manager.hpp"
#include "hashdb_changes.hpp"
#include "map_manager.hpp"
#include "multimap_manager.hpp"
#include "map_multimap_iterator.hpp"
#include "bloom_filter_manager.hpp"
#include "source_lookup_encoding.hpp"
#include "file_modes.h"
#include <vector>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <cassert>

/**
 * The map_multimap_manager<T> treats map_manager<T> and multimap_manager<T>
 * as a single managed database.
 */
template<typename T>
class map_multimap_manager_t {
  private:
  typedef typename map_manager_t<T>::map_iterator_t map_iterator_t;
  typedef typename multimap_manager_t<T>::multimap_iterator_t multimap_iterator_t;
//  typedef map_multimap_iterator_t<T> map_multimap_iterator_t;

  const std::string hashdb_dir;
  const file_mode_type_t file_mode;

  const hashdb_settings_t settings;
  map_manager_t<T> map_manager;
  multimap_manager_t<T> multimap_manager;
  bloom_filter_manager_t<T> bloom_filter_manager;

  // do not allow these
  map_multimap_manager_t(const map_multimap_manager_t&);
  map_multimap_manager_t& operator=(const map_multimap_manager_t& that);

  // helper
  void map_emplace(const T& key, uint64_t source_lookup_encoding) {
    std::pair<map_iterator_t, bool> emplace_pair =
                             map_manager.emplace(key, source_lookup_encoding);
    if (emplace_pair.second != true) {
      // really bad if emplace fails
      throw std::runtime_error("map emplace failure");
    }
  }

  // helper
  void multimap_emplace(const T& key, uint64_t source_lookup_encoding) {
    bool did_emplace = multimap_manager.emplace(key, source_lookup_encoding);
    if (did_emplace != true) {
      // really bad if emplace fails
      throw std::runtime_error("multimap emplace failure");
    }
  }

  // helper
  void map_change(const T& key, uint64_t source_lookup_encoding) {
    std::pair<map_iterator_t, bool> change_pair =
                             map_manager.change(key, source_lookup_encoding);
    if (change_pair.second != true) {
      // really bad if change fails
      throw std::runtime_error("map change failure");
    }
  }

  // helper
  void map_erase(const T& key) {
    bool did_erase = map_manager.erase(key);
    if (did_erase != true) {
      // really bad if erase fails
      throw std::runtime_error("map erase failure");
    }
  }

  // helper
  void multimap_erase(const T& key, uint64_t pay) {
    bool did_erase = multimap_manager.erase(key, pay);
    if (did_erase != true) {
      // really bad if erase fails
      throw std::runtime_error("multimap erase failure");
    }
  }

  // helper
  size_t multimap_erase_range(const T& key) {
    size_t count = multimap_manager.erase_range(key);
    if (count < 2) {
      // really bad if multimap state is broken
      throw std::runtime_error("multimap erase failure in count");
    }
    return count;
  }

  public:
  map_multimap_manager_t(const std::string& p_hashdb_dir,
                   file_mode_type_t p_file_mode) :
          hashdb_dir(p_hashdb_dir),
          file_mode(p_file_mode),
          settings(hashdb_settings_manager_t::read_settings(hashdb_dir)),
          map_manager(hashdb_dir, file_mode),
          multimap_manager(hashdb_dir, file_mode),
          bloom_filter_manager(hashdb_dir, file_mode,
                               settings.bloom1_is_used,
                               settings.bloom1_M_hash_size,
                               settings.bloom1_k_hash_functions,
                               settings.bloom2_is_used,
                               settings.bloom2_M_hash_size,
                               settings.bloom2_k_hash_functions) {
  }

  void emplace(const T& key, uint64_t source_lookup_encoding,
               uint32_t maximum_hash_duplicates,
               hashdb_changes_t& changes) {

    // if key not in bloom filter then emplace directly
    if (!bloom_filter_manager.is_positive(key)) {
      // key not present so add it in bloom filter and in map
      bloom_filter_manager.add_hash_value(key);
      map_emplace(key, source_lookup_encoding);
      ++changes.hashes_inserted;
      return;
    }

    // bloom filter gave positive, so see if this key is in map
    map_iterator_t map_iterator = map_manager.find(key);
    if (map_iterator == map_manager.end()) {
      // key not in map so add element to map
      map_emplace(key, source_lookup_encoding);
      ++changes.hashes_inserted;
      return;
    }

    // key was in map, so add key to multimap, potentially changing pay in map
    uint32_t count = source_lookup_encoding::get_count(map_iterator->second);
    if (count == 1) {

      // check if element is already in map
      if (map_iterator->second == source_lookup_encoding) {
        // this element is already in map
        ++changes.hashes_not_inserted_duplicate_element;
        return;
      }

      // don't add second if max duplicates is 1
      if (maximum_hash_duplicates == 1) {
        ++changes.hashes_not_inserted_exceeds_max_duplicates;
        return;
      }

      // move element in map to multimap
      multimap_emplace(key, map_iterator->second);
      map_change(key, source_lookup_encoding::get_source_lookup_encoding(2));

      // add key to multimap
      multimap_emplace(key, source_lookup_encoding);
      ++changes.hashes_inserted;

    } else {

      // check if element is already in multimap
      if (multimap_manager.has(key, source_lookup_encoding)) {
        // this element is already in multimap
        ++changes.hashes_not_inserted_duplicate_element;
        return;
      }

      // don't add if it exceeds max duplicates, 0 means disable
      if (maximum_hash_duplicates != 0 && count >= maximum_hash_duplicates) {
        ++changes.hashes_not_inserted_exceeds_max_duplicates;
        return;
      }

      // increment count in map
      map_change(key,
             source_lookup_encoding::get_source_lookup_encoding(count + 1));

      // add key to multimap
      multimap_emplace(key, source_lookup_encoding);
      ++changes.hashes_inserted;
    }
  }

  void remove(const T& key, uint64_t source_lookup_encoding,
              hashdb_changes_t& changes) {

    // approach depends on count
    map_iterator_t map_iterator = map_manager.find(key);
    if (map_iterator == map_manager.end()) {
      // no key
      ++changes.hashes_not_removed_no_element;
      return;
    }

    uint32_t count = source_lookup_encoding::get_count(map_iterator->second);
    if (count == 1) {
      // remove element from map if pay matches
      if (map_iterator->second == source_lookup_encoding) {
        // matches
        map_erase(key);
        ++changes.hashes_removed;
      } else {
        // the one element in map does not match
        ++changes.hashes_not_removed_no_element;
      }

    } else if (count == 2) {
      // remove element from multimap if pay matches
      bool did_erase = multimap_manager.erase(key, source_lookup_encoding);
      if (did_erase) {
        ++changes.hashes_removed;

        // also move last remaining element in multimap to map
        std::pair<multimap_iterator_t, multimap_iterator_t>
                              equal_range(multimap_manager.equal_range(key));
        map_change(equal_range.first->first, equal_range.first->second);
        multimap_erase(equal_range.first->first, equal_range.first->second);

        // lets also verify that multimap is now empty for this key
        if (multimap_manager.has_range(key)) {
          throw std::runtime_error("corrupted multimap state failure");
        }
      } else {
        // pay wasn't in multimap either
        ++changes.hashes_not_removed_no_element;
      }
    } else {
      // count > 2 so just try to remove element from multimap
      bool did_erase2 = multimap_manager.erase(key, source_lookup_encoding);
      if (did_erase2 == true) {
        ++changes.hashes_removed;
      } else {
        // pay wasn't in multimap either
        ++changes.hashes_not_removed_no_element;
      }
    }
  }

  void remove_key(const T& key, hashdb_changes_t& changes) {
    // approach depends on count
    map_iterator_t map_iterator = map_manager.find(key);
    if (map_iterator == map_manager.end()) {
      // no key
      ++changes.hashes_not_removed_no_hash;
      return;
    }

    uint32_t count = source_lookup_encoding::get_count(map_iterator->second);
    if (count == 1) {
      // remove element from map
      map_erase(key);
    } else {
      // remove multiple elements from multimap
      size_t range_count = multimap_erase_range(key);

      if (count != range_count) {
        // really bad if the count does not line up
        throw std::runtime_error("multimap remove key failure");
      }

      // remove element whose pay was count from the map
      map_erase(key);
    }
    changes.hashes_removed += count;
  }

  // find
  std::pair<map_multimap_iterator_t<T>, map_multimap_iterator_t<T> >
          find(const T& key) const {
    map_iterator_t map_it = map_manager.find(key);

    if (map_it == map_manager.end()) {
      // begin is at end
      return std::pair<map_multimap_iterator_t<T>, map_multimap_iterator_t<T> >
       (map_multimap_iterator_t<T>(&map_manager, &multimap_manager, map_it),
       (map_multimap_iterator_t<T>(&map_manager, &multimap_manager, map_it)));

    } else {
      // end is at the next entry in the map iterator
      map_iterator_t end_it(map_it);
      ++end_it;
      return std::pair<map_multimap_iterator_t<T>, map_multimap_iterator_t<T> >
       (map_multimap_iterator_t<T>(&map_manager, &multimap_manager, map_it),
       (map_multimap_iterator_t<T>(&map_manager, &multimap_manager, end_it)));
    }
  }

  // find_count
  uint32_t find_count(const T& key) const {
    // if key not in bloom filter then clearly count=0
    if (!bloom_filter_manager.is_positive(key)) {
      // key not present in bloom filter
      return 0;
    }

    // check for presence in map
    return map_manager.find_count(key);
  }

/*
  bool has_key(const T& key) {
    // if key not in bloom filter then check directly
    if (!bloom_filter_manager.is_positive(key)) {
      // key not present in bloom filter
      return false;
    }

    // check for presence in map
    return map_manager.has(key);
  }
*/

  map_multimap_iterator_t<T> begin() const {
    return map_multimap_iterator_t<T>(&map_manager, &multimap_manager,
                                   map_manager.begin());
  }

  map_multimap_iterator_t<T> end() const {
    return map_multimap_iterator_t<T>(&map_manager, &multimap_manager,
                                   map_manager.end());
  }

  size_t map_size() const {
    return map_manager.size();
  }

  size_t multimap_size() const {
    return multimap_manager.size();
  }
};

#endif

