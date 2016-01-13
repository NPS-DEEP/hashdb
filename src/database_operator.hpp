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
 * Provides hashdb commands.
 */

#ifndef DATABASE_OPERATOR_HPP
#define DATABASE_OPERATOR_HPP
#include "../src_libhashdb/hashdb.hpp"

// Standard includes
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <map>

//enum add_mode_t {ADD,
//                 ADD_MULTIPLE,
//                 ADD_REPOSITORY,
//                 INTERSECT,
//                 INTERSECT_HASH,
//                 SUBTRACT,
//                 SUBTRACT_HASH,
//                 SUBTRACT_REPOSITORY,
//                 DEDUPLICATE};

namespace database_operator {

  // add
  void add(const std::string& binary_hash,
           const hashdb::scan_manager_t& manager_a,
           hashdb::import_manager_t& manager_b) {

    // state
    std::set<uint64_t>* source_ids = new std::set<uint64_t>;

    // hash data
    std::string low_entropy_label = "";
    uint64_t entropy = 0;
    std::string block_label = "";
    hashdb::id_offset_pairs_t* id_offset_pairs = new hashdb::id_offset_pairs_t;

    // source data
    std::string file_binary_hash = "";
    uint64_t filesize = 0;
    std::string file_type = "";
    uint64_t low_entropy_count = 0;
    hashdb::source_names_t* source_names = new hashdb::source_names_t;

    // get hash data from A
    manager_a.find_hash(binary_hash, low_entropy_label,
                        entropy, block_label, *id_offset_pairs);

    // add hash from A to B
    bool hash_added = manager_b.insert_hash(binary_hash);

    // add hash data from A to B
    if (hash_added == true) {
      manager_b.insert_hash_data(
                       binary_hash, low_entropy_label, entropy, block_label);
    }

    // add hash sources from A to B

    // process each source ID, file offset pair for hash
    for (hashdb::id_offset_pairs_t::const_iterator it =
              id_offset_pairs->begin(); it != id_offset_pairs->end(); ++it) {

      // get source data from A
      manager_a.find_source_data(it->first, file_binary_hash,
                                 filesize, file_type, low_entropy_count);

      // find or create source ID for source data in B
      std::pair<bool, uint64_t> pair_b = manager_b.insert_source_file_hash(
                                                           file_binary_hash);

      // add the id_offst_pair
      manager_b.insert_hash_source(binary_hash, pair_b.second, it->second);

      // do not re-add source data
      if (source_ids->find(pair_b.second) != source_ids->end()) {
        continue;
      }

      // if the source ID is new, copy the source data
      if (pair_b.first == true) {
        manager_b.insert_source_data(pair_b.second, file_binary_hash,
                                     filesize, file_type, low_entropy_count);
      }

      // insert source names in B, there or not, in case there are new names
      manager_a.find_source_names(it->first, *source_names);
      for (hashdb::source_names_t::const_iterator it2 = source_names->begin();
           it2 != source_names->end(); ++it2) {
        manager_b.insert_source_name(pair_b.second, it2->first, it2->second);
      }

      // remember this source ID
      source_ids->insert(pair_b.second);
    }

    // clean up
    delete source_ids;
    delete id_offset_pairs;
    delete source_names;
  }
}

#endif

