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

//#define TEST_ADDER_HPP

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

#ifdef TEST_ADDER_HPP
#include "hex_helper.hpp"
#endif

//typedef std::map<std::pair<uint64_t, std::string> > old_id_to_source_t;
typedef std::pair<std::string, uint64_t> source_to_new_id_value_t;
typedef std::map<std::string, uint64_t> source_to_new_id_map_t;
typedef std::pair<std::string, uint64_t> source_offset_pair_t;
typedef std::set<source_offset_pair_t> source_offset_pairs_t;

class adder_t {
  private:

  const hashdb::scan_manager_t* const manager_a;
  hashdb::import_manager_t* const manager_b;
  source_to_new_id_map_t* const source_to_new_id_map;

  // do not allow copy or assignment
  adder_t(const adder_t&);
  adder_t& operator=(const adder_t&);

  // add record relating to source pair to B
  void add_source_pair(const std::string& binary_hash,
                       const std::string& file_binary_hash,
                       const uint64_t file_offset,
                       const uint64_t entropy,
                       const std::string& block_label) {

#ifdef TEST_ADDER_HPP
    std::cerr << "adder.add_source_pair binary_hash "
              << bin_to_hex(binary_hash) << "\n"
              << "adder.add_source_pair file_binary_hash "
              << bin_to_hex(file_binary_hash) << "\n"
              << "adder.add_source_pair file_offset " << file_offset
              << ", adder.add_source_pair entropy " << entropy
              << ", block_label " << block_label << "\n";
#endif

    // source data
    uint64_t filesize = 0;
    std::string file_type = "";
    uint64_t nonprobative_count = 0;
    hashdb::source_names_t* source_names = new hashdb::source_names_t;

    // see if source_id_b has been processed yet
    source_to_new_id_map_t::const_iterator it = source_to_new_id_map->find(
                                                        file_binary_hash);

    // get or make source_id_b
    size_t source_id_b;
    if (it == source_to_new_id_map->end()) {

      // source_id_b has not been processed so create a new source ID and
      // copy the source information

      // build new source ID
      std::pair<bool, uint64_t> pair_b = manager_b->insert_source_id(
                                                           file_binary_hash);
      source_id_b = pair_b.second;

      // remember that source_id_b will have been processed
      source_to_new_id_map->insert(source_to_new_id_value_t(file_binary_hash,
                                                           source_id_b));

      // read source_id_a
      std::pair<bool, uint64_t> pair_a = manager_a->find_source_id(
                                                         file_binary_hash);

      // make sure DB is not corrupt
      if (pair_a.first != true) {
        assert(0);
      }

      uint64_t source_id_a = pair_a.second;

      // read the source data from A
      std::string file_binary_hash_a;
      bool found_source_data = manager_a->find_source_data(source_id_a,
           file_binary_hash_a, filesize, file_type, nonprobative_count);

      // source data required
      if (!found_source_data) {
        // program error
        assert(0);
      }

      // make sure DB is not corrupt
      if (file_binary_hash != file_binary_hash_a) {
        assert(0);
      }

      // write the source data to B
      manager_b->insert_source_data(source_id_b, file_binary_hash,
                   filesize, file_type, nonprobative_count);

      // read the source names from A
      bool found_source_names = manager_a->find_source_names(source_id_a,
                                                            *source_names);
      // make sure DB is not corrupt
      if (!found_source_names) {
        assert(0);
      }

      // write the source names to B
      for (hashdb::source_names_t::const_iterator it2 = source_names->begin();
                             it2 != source_names->end(); ++it2) {
        manager_b->insert_source_name(source_id_b, it2->first, it2->second);
      }

    } else {
      source_id_b = it->second;
    }

    // insert hash data to B
    manager_b->insert_hash(binary_hash, source_id_b, file_offset,
                          entropy, block_label);
  }
  public:
  adder_t(const hashdb::scan_manager_t* const p_manager_a,
          hashdb::import_manager_t* const p_manager_b) :
                  manager_a(p_manager_a),
                  manager_b(p_manager_b),
                  source_to_new_id_map(new source_to_new_id_map_t) {
  }

  ~adder_t() {
    delete source_to_new_id_map;
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
    hashdb::id_offset_pairs_t* id_offset_pairs = new hashdb::id_offset_pairs_t;
    bool found_hash = manager_a->find_hash(binary_hash, entropy, block_label,
                                          *id_offset_pairs);
    // hash required
    if (!found_hash) {
      // program error
      assert(0);
    }

    // compose source_offset_pairs from id_offset_pairs
    for (hashdb::id_offset_pairs_t::const_iterator it =
            id_offset_pairs->begin(); it!= id_offset_pairs->end(); ++it) {
      bool found_source_data = manager_a->find_source_data(it->first,
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
#ifdef TEST_ADDER_HPP
    std::cerr << "adder.read binary_hash " << bin_to_hex(binary_hash) << "\n"
              << "adder.read entropy " << entropy
              << ", block_label " << block_label << "\n";
    for (hashdb::id_offset_pairs_t::const_iterator it =
            id_offset_pairs->begin(); it!= id_offset_pairs->end(); ++it) {
      std::cerr << "adder.read id " << it->first
                << ", offset " << it->second << "\n";
    }
    for (source_offset_pairs_t::const_iterator it =
         source_offset_pairs.begin(); it!= source_offset_pairs.end(); ++it) {
      std::cerr << "adder.read source " << bin_to_hex(it->first)
                << ", offset " << it->second << "\n";
    }
#endif

    delete id_offset_pairs;
  }

  // add hash data using file_binary_hash instead of source_id.
  void add(const std::string& binary_hash,
           const uint64_t entropy,
           const std::string& block_label,
           source_offset_pairs_t& source_offset_pairs) {

    for (source_offset_pairs_t::const_iterator it =
         source_offset_pairs.begin(); it != source_offset_pairs.end(); ++it) {
      add_source_pair(binary_hash, it->first, it->second,
                                    entropy, block_label);
    }
  }
};

#endif

