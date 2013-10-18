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
 * Provides interfaces to an indexed string lookup store.
 */

#ifndef REPOSITORY_NAME_LOOKUP_STORE_HPP
#define REPOSITORY_NAME_LOOKUP_STORE_HPP
#include <string>
#include "indexed_string_t.hpp"
#include "boost/btree/btree_index_set.hpp"

/**
 * This is a really simple class: it only supports lookups and adding;
 * it does not support removal.
 */
class indexed_string_lookup_store_t {
  private:

  // maps
  typedef boost::btree::btree_index_set<indexed_string_t> map_by_index_t;
  typedef boost::btree::btree_index_set<indexed_string_t, default_traits, value_ordering> map_by_value_t;

  const std::string filename_prefix;
  const file_mode_type_t file_mode;

  // pointers to map_by_index and map_by_value
  map_by_index_t* map_by_index;
  map_by_value_t* map_by_value;

  // disallow these
  indexed_string_lookup_store_t(const indexed_string_lookup_store_t&);
  indexed_string_lookup_store_t& operator=(const indexed_string_lookup_store_t&);

  public:
  indexed_string_lookup_store_t (const std::string p_filename_prefix,
                           file_mode_type_t p_file_mode) :
      filename_prefix(p_filename_prefix), file_mode_type(p_file_mode_type,
      map_by_index(0), map_by_value(0) {

    // derive store filenames
    std::string dat_filename = filename_prefix + ".dat";
    std::string idx1_filename = filename_prefix + ".idx1";
    std::string idx2_filename = filename_prefix + ".idx2";

    // instantiate the lookup store based on file mode
    if (file_mode == READ_ONLY) {

      // READ_ONLY
      map_by_index = new map_by_index_t(idx1_filename, dat_filename,
                                        boost::btree::flags::read_only);
      map_by_value = new map_by_value_t(idx2_filename, idx1_filename,
                                        boost::btree::flags::read_only,
                                        -1, index_ordering);
    } else if (file_mode == RW_NEW) {

      // RW_NEW
      map_by_index = new map_by_index_t(idx1_filename, dat_filename,
                                        boost::btree::flags::truncate);
      map_by_value = new map_by_value_t(idx2_filename, idx1_filename,
                                        boost::btree::flags::truncate,
                                        -1, index_ordering);
    } else if (file_mode == RW_MODIFY) {

      // RW_MODIFY
      map_by_index = new map_by_index_t(idx1_filename, dat_filename,
                                        boost::btree::flags::read_write);
      map_by_value = new map_by_value_t(idx2_filename, idx1_filename,
                                        boost::btree::flags::read_write,
                                        -1, index_ordering);
    }
  }

  /**
   * Close and release resources.
   */
  ~indexed_string_lookup_store_t() {
    delete map_by_index;
    delete map_by_value;
  }

  /**
   * Get string value from index or return false.
   */
  bool get_value(uint64_t index, std::string& value) {
    map_by_index_t::iterator it = map_by_index.find(index);
    if (it == map_by_index_t.end()) {
      value = "";
      return false;
    } else {
      value = it.second;
      return true;
    }
  }

  /**
   * Get index from string value or return false.
   */
  bool get_index(const std::string& value, uint64_t& index) {
    map_by_value_t::iterator it = map_by_value.find(value);
    if (it == map_by_value_t.end()) {
      index = 0;
      return false;
    } else {
      index = it.second;
      return true;
    }
  }

  /**
   * Insert element, return false if already there.
   */
  bool insert_element(uint64_t index, const std::string& value) {
    std::pair<map_by_index_t::const_iterator, bool> effect =
                          map_by_index.emplace(index, value);

    // return true if insert happened, false if already there
    return effect.second;
  }
};

#endif
