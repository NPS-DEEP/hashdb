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

class adder_t {
  private:

  const hashdb::scan_manager_t* const manager_a;
  hashdb::import_manager_t* const manager_b;
  const std::string repository_name;
  progress_tracker_t* const tracker;
  std::set<std::string> preexisting_sources;
  std::set<std::string> processed_sources;
  std::set<std::string> repository_sources;
  std::set<std::string> non_repository_sources;

  // do not allow copy or assignment
  adder_t(const adder_t&);
  adder_t& operator=(const adder_t&);

  // identify all preexisting sources in B and skip them during processing
  void load_preexisting_sources() {
    std::string file_hash = manager_b->first_source();
    while (file_hash != "") {
      preexisting_sources.insert(file_hash);
      file_hash = manager_b->next_source(file_hash);
    }
  }

  // helper function
  inline bool is_preexisting_source(const std::string& data) {
    return (preexisting_sources.find(data) != preexisting_sources.end());
  }

  // add source data
  void add_source_data(const std::string& file_block_hash) {
    // source data
    uint64_t filesize = 0;
    std::string file_type = "";
    uint64_t zero_count = 0;
    uint64_t nonprobative_count = 0;

    // read
    bool found_source_data = manager_a->find_source_data(
                                    file_block_hash, filesize, file_type,
                                    zero_count, nonprobative_count);
    if (found_source_data == false) {
      assert(0);
    }

    // write
    manager_b->insert_source_data(file_block_hash, filesize, file_type,
                                  zero_count, nonprobative_count);
  }

  // add source names
  void add_source_names(const std::string& file_block_hash) {
    hashdb::source_names_t* names = new hashdb::source_names_t;

    // read
    bool found_source_names = manager_a->find_source_names(
                                                file_block_hash, *names);
    if (found_source_names == false) {
      assert(0);
    }

    // write
    for (hashdb::source_names_t::const_iterator it = names->begin();
                               it != names->end(); ++it) {
      manager_b->insert_source_name(file_block_hash, it->first, it->second);
    }

    delete names;
  }

  // add repository source names
  void add_repository_source_names(const std::string& file_block_hash) {
    hashdb::source_names_t* names = new hashdb::source_names_t;

    // read
    bool found_source_names = manager_a->find_source_names(
                                                file_block_hash, *names);
    if (found_source_names == false) {
      assert(0);
    }

    // write
    for (hashdb::source_names_t::const_iterator it = names->begin();
                               it != names->end(); ++it) {
      if (it->first == repository_name) {
        manager_b->insert_source_name(file_block_hash, it->first, it->second);
      }
    }

    delete names;
  }

  // add non-repository source names
  void add_non_repository_source_names(const std::string& file_block_hash) {
    hashdb::source_names_t* names = new hashdb::source_names_t;

    // read
    bool found_source_names = manager_a->find_source_names(
                                                file_block_hash, *names);
    if (found_source_names == false) {
      assert(0);
    }

    // write
    for (hashdb::source_names_t::const_iterator it = names->begin();
                               it != names->end(); ++it) {
      if (it->first != repository_name) {
        manager_b->insert_source_name(file_block_hash, it->first, it->second);
      }
    }

    delete names;
  }

  void classify_repository_source(const std::string& file_block_hash) {
    hashdb::source_names_t* names = new hashdb::source_names_t;

    // repository_name must be defined
    if (repository_name == "") {
      assert(0);
    }

    // read names
    manager_a->find_source_names(file_block_hash, *names);

    // look for repository_name
    for (hashdb::source_names_t::const_iterator it = names->begin();
                               it != names->end(); ++it) {
      if (it->first == repository_name) {
        // the source has the repository name
        repository_sources.insert(file_block_hash);
      } else {
        // the source has a non-repository name
        non_repository_sources.insert(file_block_hash);
      }
    }
    delete names;
  }

  public:
  // add A into B
  adder_t(const hashdb::scan_manager_t* const p_manager_a,
          hashdb::import_manager_t* const p_manager_b,
          progress_tracker_t* const p_tracker) :
                  manager_a(p_manager_a),
                  manager_b(p_manager_b),
                  repository_name(""),
                  tracker(p_tracker),
                  preexisting_sources(),
                  processed_sources(),
                  repository_sources(),
                  non_repository_sources() {
    load_preexisting_sources();
  }

  // add A into B contingent on repository_name
  adder_t(const hashdb::scan_manager_t* const p_manager_a,
          hashdb::import_manager_t* const p_manager_b,
          const std::string& p_repository_name,
          progress_tracker_t* const p_tracker) :
                  manager_a(p_manager_a),
                  manager_b(p_manager_b),
                  repository_name(p_repository_name),
                  tracker(p_tracker),
                  preexisting_sources(),
                  processed_sources(),
                  repository_sources(),
                  non_repository_sources() {
    load_preexisting_sources();
  }

  // add hash and source information and do not re-add sources
  void add(const std::string& block_hash) {

    // hash data
    uint64_t k_entropy;
    std::string block_label;
    uint64_t count;
    hashdb::source_sub_counts_t* source_sub_counts =
                                   new hashdb::source_sub_counts_t;

    // get hash data from A
    bool found_hash = manager_a->find_hash(block_hash, k_entropy, block_label,
                                           count, *source_sub_counts);
    // hash required
    if (!found_hash) {
      // program error
      assert(0);
    }

    // process each source in source_sub_counts
    for (hashdb::source_sub_counts_t::const_iterator it =
         source_sub_counts->begin(); it != source_sub_counts->end(); ++it) {

      // skip preexisting sources
      if (is_preexisting_source(it->file_hash)) {
        continue;
      }

      // add hash for source
      manager_b->merge_hash(block_hash, k_entropy, block_label,
                            it->file_hash, it->sub_count);

      if (processed_sources.find(it->file_hash) == processed_sources.end()) {
        // add source information
        add_source_data(it->file_hash);
        add_source_names(it->file_hash);
        processed_sources.insert(it->file_hash);
      } else {
        // already processed
      }
    }

    // track these hashes
    tracker->track_hash_data(source_sub_counts->size());

    delete source_sub_counts;
  }

  // add hash and source information in count range and do not re-add sources
  void add_range(const std::string& block_hash,
                 size_t m, size_t n) {

    // hash data
    uint64_t k_entropy;
    std::string block_label;
    uint64_t count;
    hashdb::source_sub_counts_t* source_sub_counts =
                                     new hashdb::source_sub_counts_t;

    // get hash data from A
    bool found_hash = manager_a->find_hash(block_hash, k_entropy, block_label,
                                           count, *source_sub_counts);
    // hash required
    if (!found_hash) {
      // program error
      assert(0);
    }

    // add if in range
    if (count >= m && (n==0 || count <= n)) {

      // process each source in source_sub_counts
      for (hashdb::source_sub_counts_t::const_iterator it =
           source_sub_counts->begin(); it != source_sub_counts->end(); ++it) {

        // skip preexisting sources
        if (is_preexisting_source(it->file_hash)) {
          continue;
        }

        // add hash for source
        manager_b->merge_hash(block_hash, k_entropy, block_label,
                              it->file_hash, it->sub_count);

        if (processed_sources.find(it->file_hash) == processed_sources.end()) {
          // add source information
          add_source_data(it->file_hash);
          add_source_names(it->file_hash);
          processed_sources.insert(it->file_hash);
        } else {
          // already processed
        }
      }
    }

    // track these hashes
    tracker->track_hash_data(source_sub_counts->size());

    delete source_sub_counts;
  }

  // add hashes and source references when the repository name matches
  void add_repository(const std::string& block_hash) {

    // hash data
    uint64_t k_entropy;
    std::string block_label;
    uint64_t count;
    hashdb::source_sub_counts_t* source_sub_counts =
                                 new hashdb::source_sub_counts_t;

    // get hash data from A
    bool found_hash = manager_a->find_hash(block_hash, k_entropy, block_label,
                                           count, *source_sub_counts);
    // hash required
    if (!found_hash) {
      // program error
      assert(0);
    }

    // process each source in source_sub_counts
    for (hashdb::source_sub_counts_t::const_iterator it =
         source_sub_counts->begin(); it != source_sub_counts->end(); ++it) {

      // skip preexisting sources
      if (is_preexisting_source(it->file_hash)) {
        continue;
      }

      // make sure the source is classified as copied or skipped
      if (processed_sources.find(it->file_hash) == processed_sources.end() &&
          non_repository_sources.find(it->file_hash) == non_repository_sources.end()) {

        // not classified so classify it
        classify_repository_source(it->file_hash);
      }

      // only process sources matching repository_name
      if (repository_sources.find(it->file_hash) !=
                              repository_sources.end()) {

        // add hash for source
        manager_b->merge_hash(block_hash, k_entropy, block_label,
                              it->file_hash, it->sub_count);

        if (processed_sources.find(it->file_hash) == processed_sources.end()) {
          // add source information
          add_source_data(it->file_hash);
          add_repository_source_names(it->file_hash);
          processed_sources.insert(it->file_hash);
        } else {
          // already processed
        }
      }
    }

    // track these hashes
    tracker->track_hash_data(source_sub_counts->size());

    delete source_sub_counts;
  }

  // add hashes and source references when the repository name does not match
  void add_non_repository(const std::string& block_hash) {

    // hash data
    uint64_t k_entropy;
    std::string block_label;
    uint64_t count;
    hashdb::source_sub_counts_t* source_sub_counts =
                                      new hashdb::source_sub_counts_t;

    // get hash data from A
    bool found_hash = manager_a->find_hash(block_hash, k_entropy, block_label,
                                           count, *source_sub_counts);
    // hash required
    if (!found_hash) {
      // program error
      assert(0);
    }

    // process each source in source_sub_counts
    for (hashdb::source_sub_counts_t::const_iterator it =
         source_sub_counts->begin(); it != source_sub_counts->end(); ++it) {

      // skip preexisting sources
      if (is_preexisting_source(it->file_hash)) {
        continue;
      }

      // make sure the source is classified as copied or skipped
      if (processed_sources.find(it->file_hash) == processed_sources.end() &&
          non_repository_sources.find(it->file_hash) == non_repository_sources.end()) {

        // not classified so classify it
        classify_repository_source(it->file_hash);
      }

      // process sources that have at least one non-matching repository_name
      if (non_repository_sources.find(it->file_hash) !=
                              non_repository_sources.end()) {

        // add hash for source
        manager_b->merge_hash(block_hash, k_entropy, block_label,
                              it->file_hash, it->sub_count);

        if (processed_sources.find(it->file_hash) == processed_sources.end()) {
          // add source information
          add_source_data(it->file_hash);
          add_non_repository_source_names(it->file_hash);
          processed_sources.insert(it->file_hash);
        } else {
          // already processed
        }
      }
    }

    // track these hashes
    tracker->track_hash_data(source_sub_counts->size());

    delete source_sub_counts;
  }
};

#endif

