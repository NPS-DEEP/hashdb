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
 * Provides interfaces to the btree multimap.
 */

#ifndef MULTIMAP_MANAGER_HPP
#define MULTIMAP_MANAGER_HPP
#include <string>
#include <cstdio>
#include <cassert>
#include "file_modes.h"

/**
 * Provides interfaces to the btree multimap.
 */
template<typename T>  // hash type used as key in multimaps
class multimap_manager_t {

  public:
  typedef boost::btree::btree_multimap<T, uint64_t> multimap_t;
  typedef multimap_t::const_iterator multimap_iterator_t;
  typedef std::pair<multimap_iterator_t, multimap_iterator_t> multimap_iterator_range_t;

  private:

  // multimap_manager properties
  const std::string filename;
  const file_mode_type_t file_mode;

  // multimap
  multimap_t multimap;

  // disallow copy and assignment
  multimap_manager_t(const multimap_manager_t&);
  multimap_manager_t& operator=(const multimap_manager_t&);

  public:

  /**
   * Create a hash store of the given map type and file mode type.
   */
  multimap_manager_t (const std::string& p_hashdb_dir,
                 file_mode_type_t p_file_mode) :
       filename(p_hashdb_dir + "/hash_duplicates_store"),
       file_mode(p_file_mode),
       multimap(filename, file_mode_type_to_btree_flags_bitmask(file_mode){
  }

  // emplace
  bool emplace(const KEY_T& key, const uint64_t& pay) {
    if (file_mode == READ_ONLY) {
      throw std::runtime_error("Error: emplace called in RO mode");
    }

    // see if element already exists
    typename multimap_iterator_t it = find(key, pay);
    if (it != multimap.end()) {
      return false;
    }

    // insert the element
    multimap.emplace(key, pay);
    return true;
  }

  // erase
  bool erase(const KEY_T& key, const uint64_t& pay) {
    if (file_mode == READ_ONLY) {
      throw std::runtime_error("Error: erase called in RO mode");
    }

    // find the uniquely identified element
    multimap_iterator_range_t it = map.equal_range(key);
    typename multimap_iterator_t lower = it.first;
    if (lower == map->end()) {
      return false;
    }
    const typename multimap_iterator_t upper = it.second;
    for (; lower != upper; ++lower) {
      if (lower->second == pay) {
        // found it so erase it
        map.erase(lower);
        return true;
      }
    }
    // pay is not a member in range of key
    return false;
  }

  // erase range
  size_t erase_range(const KEY_T& key) {
    if (file_mode == READ_ONLY) {
      throw std::runtime_error("Error: erase_range called in RO mode");
    }

    return map.erase(key);
  }

  // find
  typename multimap_iterator_t find(const KEY_T& key, const uint64_t& pay) const {
    // find the uniquely identified element
    multimap_iterator_range_t it = multimap.equal_range(key);
    typename multimap_iterator_t lower = it.first;
    if (lower == multimap.end()) {
      return map.end();
    }
    const typename multimap_iterator_t upper = it.second;
    for (; lower != upper; ++lower) {
      if (lower->second == pay) {
        // found it
        return lower;
      }
    }
    // if here, pay was not found
    return map.end();
  }

  // equal_range for key
  multimap_iterator_range_t equal_range(const KEY_T& key) const {
    multimap_iterator_range_t it = multimap.equal_range(key);
    return it;
  }

  // has
  bool has(const KEY_T& key, const uint64_t& pay) const {
    // find the uniquely identified element
    multimap_iterator_range_t it = multimap.equal_range(key);
    typename multimap_iterator_t lower = it.first;
    if (lower == map.end()) {
      return false;
    }
    const typename multimap_iterator_t upper = it.second;
    for (; lower != upper; ++lower) {
      if (lower->second == pay) {
        // found it
        return true;
      }
    }
    // if here, pay was not found
    return false;
  }

  // has range
  bool has_range(const KEY_T& key) const {
    // find the key
    multimap_iterator_t it = multimap.find(key);
    if (it == map.end()) {
      return false;
    } else {
      // found at least one
      return true;
    }
  }

  // begin
  typename multimap_iterator_t begin() const {
    return multimap.begin();
  }

  // end
  typename multimap_iterator_t end() const {
    return multimap.end();
  }

  // number of elements
  size_t size() {
    return multimap.size();
  }

  // count for key
  size_t count(const KEY_T& key) const {
    return multimap.count(key);
  }
};
#endif
