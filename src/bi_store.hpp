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
 * Provides simple lookup and add interfaces for a two-index btree store.
 */

#ifndef BI_STORE_HPP
#define BI_STORE_HPP

// this process of getting WIN32 defined was inspired
// from i686-w64-mingw32/sys-root/mingw/include/windows.h.
// All this to include winsock2.h before windows.h to avoid a warning.
#if (defined(__MINGW64__) || defined(__MINGW32__)) && defined(__cplusplus)
#  ifndef WIN32
#    define WIN32
#  endif
#endif
#ifdef WIN32
  // including winsock2.h now keeps an included header somewhere from
  // including windows.h first, resulting in a warning.
  #include <winsock2.h>
  #include <io.h>
#endif


#include <string>
//#include "indexed_string_t.hpp"
#include "file_modes.h"
#include "boost/btree/btree_index_set.hpp"

template<typename BI_T>
class bi_store_t {
  private:

  // btree
  typedef typename boost::btree::btree_index_set<BI_T> index_by_key_t;
  typedef typename boost::btree::btree_index_set<BI_T, default_traits, typename BI_T::value_ordering> index_by_value_t;

  const std::string filename_prefix;
  const file_mode_type_t file_mode;

  // pointers to index_by_key and index_by_value btrees
  index_by_key_t* index_by_key;
  index_by_value_t* index_by_value;

  // disallow these
  bi_store_t(const bi_store_t&);
  bi_store_t& operator=(const bi_store_t&);

  public:
  bi_store_t (const std::string p_filename_prefix,
              file_mode_type_t p_file_mode) :
      filename_prefix(p_filename_prefix), file_mode(p_file_mode),
      index_by_key(0), index_by_value(0) {

    // data store filenames
    std::string dat_filename  = filename_prefix + ".dat";
    std::string idx1_filename = filename_prefix + ".idx1";
    std::string idx2_filename = filename_prefix + ".idx2";

    // instantiate the lookup store based on file mode
    if (file_mode == READ_ONLY) {

      // files should exist but just check .dat
      if (access(dat_filename.c_str(), F_OK) != 0) {
        std::cerr << "Error: Could not find hashdb file '" << dat_filename
                  << "'\nCannot continue.\n";
        exit(1);
      }

      // READ_ONLY
      index_by_key   = new index_by_key_t(idx1_filename, dat_filename,
                                          boost::btree::flags::read_only);
      index_by_value = new index_by_value_t(idx2_filename, index_by_key->file(),
                                          boost::btree::flags::read_only,
                                          -1, typename BI_T::value_ordering());
    } else if (file_mode == RW_NEW) {

      // RW_NEW
      index_by_key   = new index_by_key_t(idx1_filename, dat_filename,
                                          boost::btree::flags::truncate);
      index_by_value = new index_by_value_t(idx2_filename, index_by_key->file(),
                                          boost::btree::flags::truncate,
                                          -1, typename BI_T::value_ordering());
    } else if (file_mode == RW_MODIFY) {

      // RW_MODIFY
      index_by_key   = new index_by_key_t(idx1_filename, dat_filename,
                                          boost::btree::flags::read_write);
      index_by_value = new index_by_value_t(idx2_filename, index_by_key->file(),
                                          boost::btree::flags::read_write,
                                          -1, typename BI_T::value_ordering());
    }
  }

  /**
   * Close and release resources.
   */
  ~bi_store_t() {
    delete index_by_key;
    delete index_by_value;
  }

  /**
   * Get size.
   */
  uint64_t size() const {
    return index_by_key->size();
  }

  /**
   * Get value from key or return false.
   */
  bool get_value(const typename BI_T::key_type& key, typename BI_T::value_type& value) const {
    typename index_by_key_t::iterator it = index_by_key->find(key);
    if (it == index_by_key->end()) {
      return false;
    } else {
      value = it->value;
      return true;
    }
  }

  /**
   * Get key from value or return false.
   */
  bool get_key(const typename BI_T::value_type& value, typename BI_T::key_type& key) const {
    typename index_by_value_t::iterator it = index_by_value->find(value);
    if (it == index_by_value->end()) {
      return false;
    } else {
      key = it->key;
      return true;
    }
  }

  /**
   * Insert new element returning new key, else assert program error.
   */
  typename BI_T::key_type insert_value(const typename BI_T::value_type& value) {

    // btree must be writable
    if (file_mode == READ_ONLY) {
      assert(0);
    }

    // key must not exist
    typename BI_T::key_type key;
    bool status1 = get_key(value, key);
    if (status1 == true) {
      assert(0);
    }

    // get new key, which is size + 1
    key = index_by_value->size() + 1;

/*
//zzzzzzzzz
// NOTE: crbegin is broken.  Insetead, offer new key as size() + 1,
//       which is fine.

    // get new key, which is last added key + 1
    if (index_by_value->empty()) {
      // no elements, use 1 as first element index
      key = 1;
    } else {
      // use 1 + last key
      typename index_by_value_t::const_reverse_iterator rit = index_by_value->crbegin();
typename BI_T::key_type k((*rit).key);
typename BI_T::value_type v((*rit).value);
std::cout << "bi_store_t insert_value count : " << index_by_value->size() << ", key: " << k << ", value: " << v << "\n";
//      key = rit->key + 1;
      key = (*rit).key + 1;
    }
std::cout << "bi_store_t insert_value key: " << key << ", insert_value value: " << value << "\n";
*/

    // add new element
    typename index_by_key_t::file_position pos;
    pos = index_by_key->push_back(BI_T(key, value));
    index_by_key->insert_file_position(pos);
    index_by_value->insert_file_position(pos);

    return key;
  }

  // begin iterator by key
  typename index_by_key_t::iterator index_by_key_begin() const {
    return index_by_key->begin();
  }

  // end iterator by key
  typename index_by_key_t::iterator index_by_key_end() const {
    return index_by_key->end();
  }
};

#endif
