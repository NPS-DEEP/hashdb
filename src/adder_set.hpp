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
 * Add database A and B into C based on a Set rule.
 */

#ifndef ADDER_SET_HPP
#define ADDER_SET_HPP

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

class adder_set_t {
  private:

  const hashdb::scan_manager_t* const manager_a;
  const hashdb::scan_manager_t* const manager_b;
  hashdb::import_manager_t* const manager_c;
  progress_tracker_t* const tracker;
  std::set<std::string> preexisting_sources;
  std::set<std::string> processed_sources;

  // do not allow copy or assignment
  adder_set_t(const adder_set_t&);
  adder_set_t& operator=(const adder_set_t&);

  // helper function
  inline bool is_preexisting_source(const std::string& data) {
    return (preexisting_sources.find(data) != preexisting_sources.end());
  }

  // add source data
  void add_source_data(const std::string& file_binary_hash) {
    // source data
    uint64_t filesize = 0;
    std::string file_type = "";
    uint64_t zero_count = 0;
    uint64_t nonprobative_count = 0;

    // read source data from A else B
    bool found_source_data_a = manager_a->find_source_data(
                                file_binary_hash, filesize, file_type,
                                zero_count, nonprobative_count);
    if (found_source_data_a == false) {
      // read source data from B instead
      bool found_source_data_b = manager_b->find_source_data(
                                file_binary_hash, filesize, file_type,
                                zero_count, nonprobative_count);
      if (found_source_data_b == false) {
        // program error
        assert(0);
      }
    }

    // write source data
    manager_c->insert_source_data(file_binary_hash, filesize, file_type,
                                  zero_count, nonprobative_count);
  }

  // add source names
  void add_source_names(const std::string& file_binary_hash) {
    hashdb::source_names_t names;

    // read A write C
    manager_a->find_source_names(file_binary_hash, names);
    for (hashdb::source_names_t::const_iterator it = names.begin();
                               it != names.end(); ++it) {
      manager_c->insert_source_name(file_binary_hash, it->first, it->second);
    }

    // read B write C
    manager_b->find_source_names(file_binary_hash, names);
    for (hashdb::source_names_t::const_iterator it = names.begin();
                               it != names.end(); ++it) {
      manager_c->insert_source_name(file_binary_hash, it->first, it->second);
    }
  }

  public:
  adder_set_t(const hashdb::scan_manager_t* const p_manager_a,
              const hashdb::scan_manager_t* const p_manager_b,
              hashdb::import_manager_t* const p_manager_c,
              progress_tracker_t* const p_tracker) :
                  manager_a(p_manager_a),
                  manager_b(p_manager_b),
                  manager_c(p_manager_c),
                  tracker(p_tracker),
                  preexisting_sources(),
                  processed_sources() {

    // identify all preexisting sources in C and skip them during processing
    std::string file_hash = manager_c->first_source();
    while (file_hash != "") {
      preexisting_sources.insert(file_hash);
      file_hash = manager_c->next_source(file_hash);
    }
  }

  // add A and B into C where A and B hash sources are common
  void intersect(const std::string& binary_hash) {

    // read hash data from A
    float entropy_a;
    std::string block_label_a;
    uint64_t count_a;
    hashdb::source_offsets_t source_offsets_a;
    bool found_hash_a = manager_a->find_hash(binary_hash, entropy_a,
                                 block_label_a, count_a, source_offsets_a);
    // hash required
    if (!found_hash_a) {
      // program error
      assert(0);
    }

    // read hash data from B
    float entropy_b;
    std::string block_label_b;
    uint64_t count_b;
    hashdb::source_offsets_t source_offsets_b;
    bool found_hash_b = manager_b->find_hash(binary_hash, entropy_b,
                                 block_label_b, count_b, source_offsets_b);
    if (found_hash_b) {

      // go through source offsets in A and look for matches in B
      for (hashdb::source_offsets_t::const_iterator it_a =
         source_offsets_a.begin(); it_a != source_offsets_a.end();
         ++it_a) {

        // skip preexisting sources
        if (is_preexisting_source(it_a->file_hash)) {
          continue;
        }

        if (source_offsets_b.find(*it_a) != source_offsets_b.end()) {
          // in A and B so put into C
          manager_c->insert_hash(binary_hash, entropy_a, block_label_a,
                        it_a->file_hash, it_a->sub_count, it_a->file_offsets);

          if (processed_sources.find(it_a->file_hash) == processed_sources.end()) {
            // add source information
            add_source_data(it_a->file_hash);
            add_source_names(it_a->file_hash);
            processed_sources.insert(it_a->file_hash);
          } else {
            // already processed
          }
        }
      }
    }

    // track these hashes
    tracker->track_hash_data(source_offsets_a.size());
  }

  // add A and B into C when A and B hash is common
  void intersect_hash(const std::string& binary_hash) {

    // read hash data from A
    float entropy_a;
    std::string block_label_a;
    uint64_t count_a;
    hashdb::source_offsets_t source_offsets_a;
    bool found_hash_a = manager_a->find_hash(binary_hash, entropy_a,
                                 block_label_a, count_a, source_offsets_a);
    // hash required
    if (!found_hash_a) {
      // program error
      assert(0);
    }

    // read hash data from B
    float entropy_b;
    std::string block_label_b;
    uint64_t count_b;
    hashdb::source_offsets_t source_offsets_b;
    bool found_hash_b = manager_b->find_hash(binary_hash, entropy_b,
                                 block_label_b, count_b, source_offsets_b);
    if (found_hash_b) {

      // union sources A and B into C
      hashdb::source_offsets_t source_offsets_c;
      for (hashdb::source_offsets_t::const_iterator it_a =
           source_offsets_a.begin(); it_a != source_offsets_a.end();
           ++it_a) {
        source_offsets_c.insert(*it_a);
      }
      for (hashdb::source_offsets_t::const_iterator it_b =
           source_offsets_b.begin(); it_b != source_offsets_b.end();
           ++it_b) {
        source_offsets_c.insert(*it_b);
      }

      // copy union of sources
      for (hashdb::source_offsets_t::const_iterator it =
           source_offsets_c.begin(); it != source_offsets_c.end();
           ++it) {

        // skip preexisting sources
        if (is_preexisting_source(it->file_hash)) {
          continue;
        }

        // add hash for source
        manager_c->insert_hash(binary_hash, entropy_a, block_label_a,
                        it->file_hash, it->sub_count, it->file_offsets);

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
    tracker->track_hash_data(source_offsets_a.size());
  }

  // add A into C when A hash and source is not in B
  void subtract(const std::string& binary_hash) {

    // read hash data from A
    float entropy_a;
    std::string block_label_a;
    uint64_t count_a;
    hashdb::source_offsets_t source_offsets_a;
    bool found_hash_a = manager_a->find_hash(binary_hash, entropy_a,
                                 block_label_a, count_a, source_offsets_a);
    // hash required
    if (!found_hash_a) {
      // program error
      assert(0);
    }

    // read hash data from B
    float entropy_b;
    std::string block_label_b;
    uint64_t count_b;
    hashdb::source_offsets_t source_offsets_b;
    manager_b->find_hash(binary_hash, entropy_b,
                                 block_label_b, count_b, source_offsets_b);
 
    // put sources in A and not in B into C
    hashdb::source_offsets_t source_offsets_c;
    for (hashdb::source_offsets_t::const_iterator it_a =
         source_offsets_a.begin(); it_a != source_offsets_a.end();
         ++it_a) {
      if (source_offsets_b.find(*it_a) == source_offsets_b.end()) {
        source_offsets_c.insert(*it_a);
      }
    }

    // copy sources in A that were not subtracted
    for (hashdb::source_offsets_t::const_iterator it =
         source_offsets_c.begin(); it != source_offsets_c.end();
         ++it) {

      // skip preexisting sources
      if (is_preexisting_source(it->file_hash)) {
        continue;
      }

      // add hash for source
      manager_c->insert_hash(binary_hash, entropy_a, block_label_a,
                        it->file_hash, it->sub_count, it->file_offsets);

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
    tracker->track_hash_data(source_offsets_a.size());
  }

  // add A into C when A hash is not in B
  void subtract_hash(const std::string& binary_hash) {

    // read hash data from A
    float entropy_a;
    std::string block_label_a;
    uint64_t count_a;
    hashdb::source_offsets_t source_offsets_a;
    bool found_hash_a = manager_a->find_hash(binary_hash, entropy_a,
                                 block_label_a, count_a, source_offsets_a);
    // hash required
    if (!found_hash_a) {
      // program error
      assert(0);
    }

    // read hash count from B
    size_t count = manager_b->find_hash_count(binary_hash);
    if (count == 0) {
      // hash not in B so copy to C

      // copy A sources
      for (hashdb::source_offsets_t::const_iterator it =
           source_offsets_a.begin(); it != source_offsets_a.end();
           ++it) {

        // skip preexisting sources
        if (is_preexisting_source(it->file_hash)) {
          continue;
        }

        // add hash for source
        manager_c->insert_hash(binary_hash, entropy_a, block_label_a,
                        it->file_hash, it->sub_count, it->file_offsets);

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
    tracker->track_hash_data(source_offsets_a.size());
  }

};

#endif

