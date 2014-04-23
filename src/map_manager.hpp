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
 * Provides interfaces to the hash map store using glue to the actual
 * storage maps used.
 */

#ifndef MAP_MANAGER_HPP
#define MAP_MANAGER_HPP

#include <string>
#include <cstdio>
#include <cassert>
#include "file_modes.h"
#include <boost/btree/btree_map.hpp>

/**
 * Provides interfaces to the hash map store that use glue to the actual
 * storage maps used.
 */
template<class T>  // hash type used as key in maps
class map_manager_t {

  public:
  typedef boost::btree::btree_map<T, uint64_t> map_t;
  typedef map_t::const_iterator map_iterator_t;

  private:
  // map_manager properties
  const std::string filename;
  const file_mode_type_t file_mode;

  // btree map
  map_t<T, uint64_t> map;

  // disallow copy and assignment
  map_manager_t(const map_manager_t&);
  map_manager_t& operator=(const map_manager_t&);

  public:

  /**
   * Create a hash store of the given map type and file mode type.
   */
  map_manager_t (const std::string& p_hashdb_dir,
                 file_mode_type_t p_file_mode,
                 map_type_t p_map_type) :
       filename(p_hashdb_dir + "/hash_store"),
       file_mode(p_file_mode),
       map(filename, file_mode_type_to_btree_flags_bitmask(file_mode)) {
  }

  // emplace
  // return pair from composed map iterator and actual map's bool
  std::pair<typename map_t::const_iterator, bool>
  emplace(const T& key, const uint64_t& pay) {

    if (file_mode == READ_ONLY) {
      throw std::runtime_error("Error: emplace called in RO mode");
    }

    return map.emplace(key, pay);
  }

  // erase
  size_t erase(const KEY_T& key) {
    if (file_mode == READ_ONLY) {
      throw std::runtime_error("Error: erase called in RO mode");
    }

    size_t num_erased = map.erase(key);
    return num_erased;
  }

  // change
  std::pair<typename map_t::const_iterator, bool>
  change(const KEY_T& key, const uint64_t& pay) {
    if (file_mode == READ_ONLY) {
      throw std::runtime_error("Error: change called in RO mode");
    }

    // get original key
    typename map_t::const_iterator itr = map.find(key);
    if (itr == map.end()) {
      // the old element did not exist
      return std::pair<typename map_t::const_iterator, bool>(map.end(), false);
    }
    if (itr->second == pay) {
      // the payload value is the same
      return std::pair<typename map_t::const_iterator, bool>(itr, false);
    }

    // erase the old element
    size_t num_erased = erase(key);
    if (num_erased != 1) {
      assert(0);
      std::exit(1);
//      // erase failed
    } else {
      // put in new
      return map.emplace(key, pay);
    }
  }

  // find
  typename map_t::const_iterator find(const KEY_T& key) const {
    typename map_t::const_iterator itr = map.find(key);
    return itr;
  }

  // find_count
  uint32_t find_count(const KEY_T& key) const {
    typename map_t::const_iterator itr = map.find(key);
    if (itr == map.end()) {
      return 0;
    } else {
      return source_lookup_encoding::get_count(itr->second);
    }
  }

  // number of elements
  size_t size() {
    return map.size();
  }

  // begin
  typename map_t::const_iterator begin() const {
    return map.begin();
  }

  // end
  typename map_t::const_iterator end() const {
    return map.end();
  }
};
#endif
