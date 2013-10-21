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
 * Provides interfaces to a two-index btree store.
 * Interfaces include lookup and add, and not removal.
 */

#ifndef REPOSITORY_NAME_LOOKUP_STORE_HPP
#define REPOSITORY_NAME_LOOKUP_STORE_HPP
#include <string>
#include "indexed_string_t.hpp"
#include "boost/btree/btree_index_set.hpp"

template<typename BI_T>
class bi_store_t {
  private:

  // maps
  typedef boost::btree::btree_index_set<BI_T> btree_by_key_t;
  typedef boost::btree::btree_index_set<BI_T, default_traits, value_ordering> btree_by_value_t;

  const std::string filename_prefix;
  const file_mode_type_t file_mode;

  // pointers to map_by_index and map_by_value
  map_by_index_t* map_by_index;
  map_by_value_t* map_by_value;

  // disallow these
  bi_store_t(const bi_store_t&);
  bi_store_t& operator=(const bi_store_t&);

  public:
  bi_store_t (const std::string p_filename_prefix,
              file_mode_type_t p_file_mode) :
      filename_prefix(p_filename_prefix), file_mode_type(p_file_mode_type,
      map_by_index(0), map_by_value(0) {

    // data store filenames
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
  ~bi_store_t() {
    delete map_by_index;
    delete map_by_value;
  }

  /**
   * Get value from key or return false.
   */
  bool get_value(const BI_T::key_type& index, BI_T::value_type& value) {
    map_by_index_t::iterator it = map_by_index.find(index);
    if (it == map_by_index_t.end()) {
      return false;
    } else {
      value = it.second;
      return true;
    }
  }

  /**
   * Get key from value or return false.
   */
  bool get_key(const BI_T::value_type& value, BI_T::key_type& index) {
    map_by_value_t::iterator it = map_by_value.find(value);
    if (it == map_by_value_t.end()) {
      return false;
    } else {
      index = it.second;
      return true;
    }
  }

  /**
   * Insert element, return false if already there.
   */
  std::pair<map_by_index_t::const_iterator, bool>
  insert_element(const BI_T::key_type, const BI_T::value_type& value) {
    if (file_mode == READ_ONLY) {
      assert(0);
    }

    std::pair<map_by_index_t::const_iterator, bool> effect =
                          map_by_index.zzzzzz(index, value);

    // return true if insert happened, false if already there
    return effect.second;
  }
};

#endif
