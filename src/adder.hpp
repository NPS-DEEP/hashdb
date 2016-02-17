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
 * Facilitate adding from database A to B.
 */

#ifndef ADDER_HPP
#define ADDER_HPP
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

//typedef std::map<std::pair<uint64_t, std::string> > old_id_to_source_t;
typedef std::pair<std::string, uint64_t> source_to_new_id_t;
typedef std::map<source_to_new_id_t> source_to_new_id_map_t;
typedef std::pair<std::string, uint64_t> source_offset_pair_t;
typedef std::set<source_offset_pair_t> source_offset_pairs_t;

class adder_t {

  private const hashdb::scan_manager_t* manager_a;
  private const hashdb::scan_manager_t* manager_b;
  private source_to_new_id_map_t* source_to_new_id_map;

  // do not allow copy or assignment
  adder_t(const adder_t&);
  adder_t& operator=(const adder_t&);

  // add hash data but accept file_binary_hash instead of source_id.
  void add_source_pair(const std::string& binary_hash,
                       const std::string& file_binary_hash,
                       const uint64_t file_offset,
                       const uint64_t entropy,
                       const std::string& block_label) {

    // source data
    std::string file_binary_hash = "";
    uint64_t filesize = 0;
    std::string file_type = "";
    uint64_t nonprobative_count = 0;
    hashdb::source_names_t* source_names = new hashdb::source_names_t;

    // get or make source ID for B
    source_to_new_id_map_t::const_iterator it = source_to_new_id_map.find(
                                                        file_binary_hash);
    size_t new_source_id;
    if (it == source_to_new_id_map.end()) {
      // copy the source information
      // build new source ID
      std::pair<bool, uint64_t> pair_b = manager_b.insert_source_id(
                                                           file_binary_hash);
      new_source_id = pair_b.second;

      // remember the source to ID pair
      source_to_new_id_map.insert(source_to_new_id_t(file_binary_hash,
                                                     new_source_id));

      // copy the source data
      bool found_source_data = manager_a.find_source_data(new_source_id,
                 file_binary_hash, filesize, file_type, low_entropy_count);

      // source data required
      if (!found_source_data) {
        // program error
        assert(0);
      }

      // copy the source names
      bool found_source_names = manager_a.find_source_names(new_source_id,
                                                            source_names);
      for (hashdb::source_names_t::const_iterator it2 = source_names->begin();
                             it2 != source_names->end(); ++it2) {
        manager_b.insert_source_name(new_source_id, it2->first, it2->second);
      }

    } else {
      new_source_id = it->second;
    }

    // insert hash data into B
    manager_b.insert_hash(binary_hash, new_source_id, file_offset,
                          entropy, block_label);
  }
  public:
  ~adder_t() {
    delete source_to_new_id_map;
  }


  adder_t(const hashdb::scan_manager_t& p_manager_a,
          const hashdb::import_manager_t& p_manager_b) :
                  manager_a(*p_manager_a),
                  manager_b(*p_manager_b),
                  source_to_new_id_map(new source_to_new_id_map_t) {
  }

  // read hash data but return file_binary_hash instead of source_id.
  void read(const std::string& binary_hash,
            uint64_t& entropy,
            std::string& block_label,
            source_offset_pairs_t& source_offset_pairs) const {

    source_offset_pairs.clear();

    // source data
    std::string file_binary_hash = "";
    uint64_t filesize = 0;
    std::string file_type = "";
    uint64_t nonprobative_count = 0;

    // get hash data from A
    bool found_hash = manager_a.find_hash(binary_hash, entropy, block_label,
                                          *id_offset_pairs);
    // hash required
    if (!found_hash) {
      // program error
      assert(0);
    }

    // compose source_offset_pairs from id_offset_pairs
    for (hashdb::id_offset_pairs_t::const_iterator it =
            id_offset_pairs->begin(); it!= id_offset_pairs->end(); ++it) {
      bool found_source_data = manager_a.find_source_data(it->first,
                  file_binary_hash, filesize, file_type, nonprobative_count);
 
      // source data required
      if (!found_source_data) {
        // program error
        assert(0);
      }

      // add source_offset
      source_offset_pairs.insert(source_offset_pair_t(file_binary_hash,
                                                      it->second));
    }

    delete id_offset_pairs;
  }

  // add hash data using file_binary_hash instead of source_id.
  void add(const std::string& binary_hash,
           const uint64_t entropy,
           const std::string& block_label,
           source_offset_pairs_t& source_offset_pairs) const {

    for (source_offset_pairs_t::const_iterator it =
         source_offset_pairs.begin(); it != source_offset_pairs.end(); ++it) {
      add_source_pair(binary_hash, it.first, it.second, entropy, block_label);
    }
  }



}

#endif

