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

#ifndef SOURCE_METADATA_MANAGER_HPP
#define SOURCE_METADATA_MANAGER_HPP

#include "hash_t_selector.h" // to define hash_t
#include "source_metadata.hpp"
#include <string>
#include "boost/btree/index_helpers.hpp"
#include "boost/btree/btree_index_set.hpp"

class source_metadata_manager_t {
  private:

  // btrees to manage multi-index lookups
  typedef typename boost::btree::btree_index_set<source_metadata_t> idx1_btree_t;
  typedef typename boost::btree::btree_index_set<source_metadata_t, default_traits, hash_ordering> idx2_btree_t;
  typedef typename boost::btree::btree_index_set<source_metadata_t, default_traits, file_size_ordering> idx2_btree_t;

  // settings
  const std::string hashdb_dir;
  const std::string dat_filename;
  const std::string idx1_filename; // source_lookup_index
  const std::string idx2_filename; // hash
  const std::string idx3_filename; // file_size
  const file_mode_type_t file_mode;
  const boost::btree::flags::bitmask btree_flags;

  idx1_btree_t idx1_btree;
  idx2_btree_t idx2_btree;
  idx3_btree_t idx3_btree;

  // convert file_mode_type to btree flags
  boost::btree::flags::bitmask get_flags(file_mode_type_t p_file_mode_type) {
    if (file_mode == READ_ONLY) return boost::btree::flags::read_only;
    if (file_mode == RW_NEW) return boost::btree::flags::truncate;
    if (file_mode == READ_ONLY) return boost::btree::flags::read_write;
  }

  // disallow these
  source_metadata_manager_t(const source_metadata_manager_t&);
  source_metadata_manager_t& operator=(const source_metadata_manager_t&);

  public:
  source_metadata_manager_t (const std::string p_hashdb_dir,
                           file_mode_type_t p_file_mode_type) :
       hashdb_dir(p_hashdb_dir),
       dat_filename(hashdb_dir + "source_metadata_store.dat"),
       idx1_filename(hashdb_dir + "source_metadata_store.idx1"),
       idx2_filename(hashdb_dir + "source_metadata_store.idx2"),
       idx3_filename(hashdb_dir + "source_metadata_store.idx3"),
       file_mode(p_file_mode_type),
       btree_flags(get_btree_flags(file_mode)),
       idx1_btree(idx1_filename, dat_filename, btree_flags),
       idx2_btree(idx2_filename, dat_filename, btree_flags),
       idx3_btree(idx3_filename, dat_filename, btree_flags) {
  }

  /**
   * Insert and return true but if source_lookup_index is already there,
   * return false.
   */
  bool insert(source_metadata_t source_metadata) {

    // btree must be writable
    if (file_mode == READ_ONLY) {
      assert(0);
    }

    // source lookup index must exist
    typename idx1_btree_t::iterator it = idx1_btree.find(
                                        source_metadata.source_lookup_index);
    if (it == idx1_btree.end()) {

      // good, add the entry
      typename idx1_btree::file_position pos;
      pos = idx1_btree.push_back(source_metadata);
      idx1_btree.insert_file_position(pos);
      idx2_btree.insert_file_position(pos);
      idx3_btree.insert_file_position(pos);
      return true;
    }
  }

  // find by source lookup index
  std::pair<source_metada_iterator_t, source_metadata_iterator_t>
                  find_by_source_lookup_index(uint64_t source_lookup_index) {
    idx1_btree_t::const_iterator_range range =
                                  idx1_btree.equal_range(source_lookup_index);
    return std::pair<source_metada_iterator_t, source_metadata_iterator_t>(
                                  source_metadata_iterator(range.first),
                                  source_metadata_iterator(range.second));
  }

  // find by hash
  std::pair<source_metada_iterator_t, source_metadata_iterator_t>
                  find_by_hash(hash_t hash) {
    idx2_btree_t::const_iterator_range range =
                                  idx2_btree.equal_range(source_lookup_index);
    return std::pair<source_metada_iterator_t, source_metadata_iterator_t>(
                                  source_metadata_iterator(range.first),
                                  source_metadata_iterator(range.second));
  }

  // find by file size
  std::pair<source_metada_iterator_t, source_metadata_iterator_t>
                  find_by_file_size(hash_t hash) {
    idx3_btree_t::const_iterator_range range =
                                  idx3_btree.equal_range(source_lookup_index);
    return std::pair<source_metada_iterator_t, source_metadata_iterator_t>(
                                  source_metadata_iterator(range.first),
                                  source_metadata_iterator(range.second));
  }
};

#endif
