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
 * Read operations read from A.  Write operations write to B.
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

typedef std::pair<std::string, uint64_t> source_offset_pair_t;
typedef std::set<source_offset_pair_t> source_offset_pairs_t;
typedef std::set<std::string> processed_sources_t;

class adder_t {
  private:

  const hashdb::scan_manager_t* const manager_a;
  hashdb::import_manager_t* const manager_b;
  std::set<std::string>* const processed_sources;

  // do not allow copy or assignment
  adder_t(const adder_t&);
  adder_t& operator=(const adder_t&);

  // add source data
  void add_source_data(const std::string& file_binary_hash) {
    // source data
    uint64_t filesize = 0;
    std::string file_type = "";
    uint64_t nonprobative_count = 0;

    // read
    bool found_source_data = manager_a->find_source_data(
                 file_binary_hash, filesize, file_type, nonprobative_count);
    if (found_source_data == false) {
      assert(0);
    }

    // write
    manager_b->insert_source_data(file_binary_hash, filesize, file_type,
                                nonprobative_count);
  }

  // add source names
  void add_source_names(const std::string& file_binary_hash) {
    hashdb::source_names_t* names = new hashdb::source_names_t;

    // read
    bool found_source_names = manager_a->find_source_names(
                                                file_binary_hash, *names);
    if (found_source_names == false) {
      assert(0);
    }

    // write
    for (hashdb::source_names_t::const_iterator it = names->begin();
                               it != names->end(); ++it) {
      manager_b->insert_source_name(file_binary_hash, it->first, it->second);
    }

    delete names;
  }

  // add hash and source information without reprocessing sources
  void add_hash(const std::string& binary_hash) {

    // hash data
    uint64_t entropy;
    std::string block_label;
    source_offset_pairs_t* source_offset_pairs = new source_offset_pairs_t;

    // get hash data from A
    bool found_hash = manager_a->find_hash(binary_hash, entropy, block_label,
                                          *source_offset_pairs);
    // hash required
    if (!found_hash) {
      // program error
      assert(0);
    }

    // process each source offset pair in hash
    for (source_offset_pairs_t::const_iterator it =
         source_offset_pairs->begin(); it != source_offset_pairs->end(); ++it) {

      // add hash for source
      manager_b->insert_hash(binary_hash, it->first, it->second,
                             entropy, block_label);

      if (processed_sources->find(it->first) == processed_sources->end()) {
        // add source information
        add_source_data(it->first);
        add_source_names(it->first);
        processed_sources->insert(it->first);
      } else {
        // already processed
      }
    }

    delete source_offset_pairs;
  }

  public:
  adder_t(const hashdb::scan_manager_t* const p_manager_a,
          hashdb::import_manager_t* const p_manager_b) :
                  manager_a(p_manager_a),
                  manager_b(p_manager_b),
                  processed_sources(new processed_sources_t) {
  }

  ~adder_t() {
    delete processed_sources;
  }

  // add
  void add(const std::string& binary_hash) {

    // add all hash and source information
    add_hash(binary_hash);
  }

  // add hashes and source references when the repository name matches
  void add_repository(const std::string& binary_hash,
                      const std::string& repository_name) {

    // hash data
    uint64_t entropy;
    std::string block_label;
    source_offset_pairs_t* source_offset_pairs = new source_offset_pairs_t;

    // get hash data from A
    bool found_hash = manager_a->find_hash(binary_hash, entropy, block_label,
                                          *source_offset_pairs);
    // hash required
    if (!found_hash) {
      // program error
      assert(0);
    }

    // process each source offset pair in hash
    for (source_offset_pairs_t::const_iterator it =
         source_offset_pairs->begin(); it != source_offset_pairs->end(); ++it) {

      // add hash for source
      manager_b->insert_hash(binary_hash, it->first, it->second,
                             entropy, block_label);

      if (processed_sources->find(it->first) == processed_sources->end()) {
        // add source information
        add_source_data(it->first);
        add_source_names(it->first);
        processed_sources->insert(it->first);
      } else {
        // already processed
      }
    }

    delete source_offset_pairs;
  }



};

#endif

