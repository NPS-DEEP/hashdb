// Author:  Bruce Allen
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

#include "../src_libhashdb/hashdb.hpp"

// Standard includes
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <map>

typedef std::pair<std::string, uint64_t> source_offset_pair_t;
typedef std::set<source_offset_pair_t> source_offset_pairs_t;
typedef std::set<std::string> sources_t;

class adder_t {
  private:

  const hashdb::scan_manager_t* const manager_a;
  progress_tracker_t* const tracker;
  hashdb::import_manager_t* const manager_b;
  std::set<std::string>* const processed_sources;
  std::set<std::string>* const repository_sources;
  std::set<std::string>* const non_repository_sources;
  const std::string repository_name;

  // do not allow copy or assignment
  adder_t(const adder_t&);
  adder_t& operator=(const adder_t&);

  // add source data
  void add_source_data(const std::string& file_binary_hash) {
    // source data
    uint64_t filesize = 0;
    std::string file_type = "";
    uint64_t zero_count = 0;
    uint64_t nonprobative_count = 0;

    // read
    bool found_source_data = manager_a->find_source_data(
                                    file_binary_hash, filesize, file_type,
                                    zero_count, nonprobative_count);
    if (found_source_data == false) {
      assert(0);
    }

    // write
    manager_b->insert_source_data(file_binary_hash, filesize, file_type,
                                  zero_count, nonprobative_count);
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

  // add repository source names
  void add_repository_source_names(const std::string& file_binary_hash) {
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
      if (it->first == repository_name) {
        manager_b->insert_source_name(file_binary_hash, it->first, it->second);
      }
    }

    delete names;
  }

  // add non-repository source names
  void add_non_repository_source_names(const std::string& file_binary_hash) {
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
      if (it->first != repository_name) {
        manager_b->insert_source_name(file_binary_hash, it->first, it->second);
      }
    }

    delete names;
  }

  void classify_repository_source(const std::string& file_binary_hash) {
    hashdb::source_names_t* names = new hashdb::source_names_t;

    // repository_name must be defined
    if (repository_name == "") {
      assert(0);
    }

    // read names
    manager_a->find_source_names(file_binary_hash, *names);

    // look for repository_name
    for (hashdb::source_names_t::const_iterator it = names->begin();
                               it != names->end(); ++it) {
      if (it->first == repository_name) {
        // the source has the repository name
        repository_sources->insert(file_binary_hash);
      } else {
        // the source has a non-repository name
        non_repository_sources->insert(file_binary_hash);
      }
    }
    delete names;
  }

  public:
  // add A into B
  adder_t(const hashdb::scan_manager_t* const p_manager_a,
          progress_tracker_t* p_tracker,
          hashdb::import_manager_t* const p_manager_b) :
                  manager_a(p_manager_a),
                  tracker(p_tracker),
                  manager_b(p_manager_b),
                  processed_sources(new sources_t),
                  repository_sources(new sources_t),
                  non_repository_sources(new sources_t),
                  repository_name("") {
  }

  // add A into B contingent on repository_name
  adder_t(const hashdb::scan_manager_t* const p_manager_a,
          progress_tracker_t* p_tracker,
          hashdb::import_manager_t* const p_manager_b,
          const std::string& p_repository_name) :
                  manager_a(p_manager_a),
                  tracker(p_tracker),
                  manager_b(p_manager_b),
                  processed_sources(new sources_t),
                  repository_sources(new sources_t),
                  non_repository_sources(new sources_t),
                  repository_name(p_repository_name) {
  }

  ~adder_t() {
    delete processed_sources;
    delete repository_sources;
    delete non_repository_sources;
  }

  // add hash and source information and do not re-add sources
  void add(const std::string& binary_hash) {

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

    // track these hashes
    tracker->track_hash_data(source_offset_pairs->size());

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

  // add hash and source information in count range and do not re-add sources
  void add_range(const std::string& binary_hash,
                 size_t m, size_t n) {

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

    // track these hashes
    tracker->track_hash_data(source_offset_pairs->size());

    // add if in range
    if (source_offset_pairs->size() >= m &&
        (n==0 || source_offset_pairs->size() <= n)) {

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
    }

    delete source_offset_pairs;
  }

  // add hashes and source references when the repository name matches
  void add_repository(const std::string& binary_hash) {

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

    // track these hashes
    tracker->track_hash_data(source_offset_pairs->size());

    // process each source offset pair in hash
    for (source_offset_pairs_t::const_iterator it =
         source_offset_pairs->begin(); it != source_offset_pairs->end(); ++it) {

      // make sure the source is classified as copied or skipped
      if (processed_sources->find(it->first) == processed_sources->end() &&
          non_repository_sources->find(it->first) == non_repository_sources->end()) {

        // not classified so classify it
        classify_repository_source(it->first);
      }

      // only process sources matching repository_name
      if (repository_sources->find(it->first) !=
                              repository_sources->end()) {

        // add hash for source
        manager_b->insert_hash(binary_hash, it->first, it->second,
                               entropy, block_label);

        if (processed_sources->find(it->first) == processed_sources->end()) {
          // add source information
          add_source_data(it->first);
          add_repository_source_names(it->first);
          processed_sources->insert(it->first);
        } else {
          // already processed
        }
      }
    }

    delete source_offset_pairs;
  }

  // add hashes and source references when the repository name does not match
  void add_non_repository(const std::string& binary_hash) {

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

    // track these hashes
    tracker->track_hash_data(source_offset_pairs->size());

    // process each source offset pair in hash
    for (source_offset_pairs_t::const_iterator it =
         source_offset_pairs->begin(); it != source_offset_pairs->end(); ++it) {

      // make sure the source is classified as copied or skipped
      if (processed_sources->find(it->first) == processed_sources->end() &&
          non_repository_sources->find(it->first) == non_repository_sources->end()) {

        // not classified so classify it
        classify_repository_source(it->first);
      }

      // process sources that have at least one non-matching repository_name
      if (non_repository_sources->find(it->first) !=
                              non_repository_sources->end()) {

        // add hash for source
        manager_b->insert_hash(binary_hash, it->first, it->second,
                               entropy, block_label);

        if (processed_sources->find(it->first) == processed_sources->end()) {
          // add source information
          add_source_data(it->first);
          add_non_repository_source_names(it->first);
          processed_sources->insert(it->first);
        } else {
          // already processed
        }
      }
    }

    delete source_offset_pairs;
  }
};

#endif

