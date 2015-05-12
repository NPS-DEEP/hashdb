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
 * Rank sources identified by hashes in identified_blocks.txt input.
 */

#ifndef rank_MANAGER_HPP
#define rank_MANAGER_HPP

// Standard includes
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>

/**
 * Gather source data and print ratio ranking.
 */

class rank_manager_t {
  private:
  struct source_data_t {
    size_t count;
    double weight;
    size_t probative_count;
    double probative_weight;
    source_data_t() : count(0), weight(0),
                      probative_count(0), probative_weight(0) {
    }
  };

  const lmdb_ro_manager_t* ro_manager;
  typedef std::map<uint64_t, source_data_t> sources_t;
  sources_t* sources;

  // do not allow copy or assignment
  rank_manager_t(const rank_manager_t&);
  rank_manager_t& operator=(const rank_manager_t&);

  public:
  rank_manager_t(const lmdb_ro_manager_t* p_ro_manager) :
                         ro_manager(p_ro_manager),
                         sources() {
    sources = new sources_t;
  }

  ~rank_manager_t() {
    delete sources;
  }

  // ingest hash from feature line
  void ingest_hash(feature_line_t feature_line) {

    // get the binary hash
    std::string binary_hash = lmdb_helper::hex_to_binary_hash(feature_line.feature);

    // reject invalid input
    if (binary_hash == "") {
      std::cerr << "Error: Invalid hash: '" << feature_line.feature << "'\n";
      return;
    }

    // get count of sources for this hash
    size_t count = ro_manager->find_count(binary_hash);

    // get range of entries for the hash
    lmdb_hash_it_data_t hash_it_data = ro_manager->find_first(binary_hash);

    // make sure the hash is in the DB
    if (!hash_it_data.is_valid) {
      std::cout << "Error: Invalid hash, incorrect feature file or hash database, '" << feature_line.feature << "'\n";
      return;
    }

    // process each source for the hash
    source_data_t* source;
    while (hash_it_data.is_valid && hash_it_data.binary_hash == binary_hash) {
      // get current working source_id
      uint64_t source_id = hash_it_data.source_lookup_index;

      // find or create source_data_t record
      sources_t::iterator sources_it = sources->find(source_id);
      if (sources_it == sources->end()) {
        // create and point to new source
        std::pair<sources_t::iterator, bool> insert_pair = sources->insert(
             (std::pair<uint64_t, source_data_t>(source_id, source_data_t())));

        if (insert_pair.second == false) {
          // program error if not present
          assert(0);
        }
        source = &(insert_pair.first->second);
      } else {
        // point to existing source
        source = &(sources_it->second);
      }

      // adjust source fields for this source
      ++source->count;
      source->weight += 1/count;
      if (hash_it_data.hash_label == "") {
        ++source->probative_count;
        source->probative_weight += 1/count;
      }

      // store back the change
      sources->insert(std::pair<uint64_t, source_data_t>(source_id, *source));

      // next
      hash_it_data = ro_manager->find_next(hash_it_data);
    }
  }

  // print ranked sources
  void print_ranked_sources() const {
    // get value to use for block size
    size_t block_size = (ro_manager->settings.hash_block_size == 0) ?
                                     1 : ro_manager->settings.hash_block_size;

    // iterate through sources, printing calculated fields
    for (sources_t::iterator it = sources->begin(); it != sources->end(); ++it) {
      uint64_t source_id = it->first;
      lmdb_source_data_t lmdb_source_data = ro_manager->find_source(source_id);
      std::cout << "{"
                << "\"source_id\":" << source_id
                << ",\"repository_name\":\""
                << lmdb_helper::escape_json(lmdb_source_data.repository_name) << "\""
                << ",\"filename\":\""
                << lmdb_helper::escape_json(lmdb_source_data.filename) << "\""
                << ",\"file_blocks\":" << lmdb_source_data.filesize / block_size
//                << ",\"file_probative_blocks\":" << lmdb_source_data.filesize / block_size
                << ",\"count\":" << it->second.count
                << ",\"weight\":" << it->second.weight
                << ",\"probative_count\":" << it->second.probative_count
                << ",\"probative_weight\":" << it->second.probative_weight
                << "}" << std::endl;
    }
  }
};

#endif

