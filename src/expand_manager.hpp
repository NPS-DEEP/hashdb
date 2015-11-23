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
 * Expand sources for a hash and print them.
 * Do not print more than max_sources sources for a given hash.
 * Generate and print source IDs.
 * Do not re-print any sources for a previously seen source ID.
 */

#ifndef EXPAND_MANAGER_HPP
#define EXPAND_MANAGER_HPP
#include "print_helper.hpp"
#include "lmdb_ro_manager.hpp"
#include "lmdb_hash_it_data.hpp"
#include "lmdb_source_data.hpp"
#include "feature_line.hpp"

#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <crc32.h>

class expand_manager_t {

  private:
  const lmdb_ro_manager_t* ro_manager;
  uint32_t max_sources;
  std::set<uint64_t>* source_ids;
  std::set<std::string>* hashes;

  // do not allow copy or assignment
  expand_manager_t(const expand_manager_t&);
  expand_manager_t& operator=(const expand_manager_t&);

  // calculate source list size and ID pair
  std::pair<size_t, uint32_t> calculate_source_list_size_id_pair(
                                         const std::string binary_hash) {
    std::set<uint64_t>* temp_source_ids;
    temp_source_ids = new std::set<uint64_t>;

    // add each source ID to the temporary set
    lmdb_hash_it_data_t hash_it_data = ro_manager->find_first(binary_hash);
    while (hash_it_data.binary_hash == binary_hash) {

      // add source ID
      temp_source_ids->insert(hash_it_data.source_lookup_index);
      hash_it_data = ro_manager->find_next(hash_it_data);
    }

    // start a source list ID CRC hash
    uint32_t crc = 0;

    // add each source ID to the CRC hash
    for (std::set<uint64_t>::const_iterator it = temp_source_ids->begin();
         it != temp_source_ids->end(); ++it) {
//      crc = crc32(crc, reinterpret_cast<const uint8_t*>(binary_hash.c_str()), binary_hash.size());
      crc = crc32(crc, reinterpret_cast<const uint8_t*>(&(*it)), sizeof(uint64_t));
    }

    std::pair<size_t, uint64_t> pair = std::pair<size_t,uint64_t>(
                                            temp_source_ids->size(), crc);
    delete temp_source_ids;
    return pair;
  }

  // print the source list
  void print_source_list(const std::string& binary_hash) {

    // print opening
    std::cout << ", \"sources\":[";

    // print each source
    bool at_start = true;
    lmdb_hash_it_data_t hash_it_data = ro_manager->find_first(binary_hash);
    while (hash_it_data.binary_hash == binary_hash) {

      // manage the comma
      if (at_start) {
        at_start = false;
      } else {
        // print continuation
        std::cout << ",";
      }

      // print the source ID and file offset
      std::cout << "{\"source_id\":" << hash_it_data.source_lookup_index
                << ",\"file_offset\":" << hash_it_data.file_offset;

      // print associated hash label, if present
      if (hash_it_data.hash_label != "") {
        std::cout << ",\"label\":\"" << hash_it_data.hash_label << "\"";
      }

      // print full source information the first time
      if (source_ids->find(hash_it_data.source_lookup_index) == source_ids->end()) {

        // record that this source ID has been printed
        source_ids->insert(hash_it_data.source_lookup_index);

        // get source data for this source ID
        lmdb_source_data_t source_data = ro_manager->find_source(
                                        hash_it_data.source_lookup_index);

        // print source data
        std::cout << ",\"repository_name\":\""
                  << lmdb_helper::escape_json(source_data.repository_name)
                  << "\",\"filename\":\""
                  << lmdb_helper::escape_json(source_data.filename)
                  << "\"";
        if (source_data.filesize != 0) {
          std::cout << ",\"filesize\":" << source_data.filesize;
        }
        if (source_data.binary_hash != "" && source_data.binary_hash !=
                                     lmdb_helper::hex_to_binary_hash("00")) {
          std::cout << ",\"file_hashdigest\":\""
                    << lmdb_helper::binary_hash_to_hex(source_data.binary_hash)
                    << "\"";
        }
      }
      hash_it_data = ro_manager->find_next(hash_it_data);

      // print source closure
      std::cout << "}";
    }

    // print sources closure
    std::cout << "]";
  }

  public:
  expand_manager_t(const lmdb_ro_manager_t* p_ro_manager,
                   uint32_t p_max_sources) :
          ro_manager(p_ro_manager),
          max_sources(p_max_sources),
          source_ids(),
          hashes() {
    source_ids = new std::set<uint64_t>;
    hashes = new std::set<std::string>;
  }

  ~expand_manager_t() {
    delete source_ids;
    delete hashes;
  }

  // print expanded hash
  void expand_hash(const std::string& binary_hash) {

    // skip invalid hash
    if (binary_hash == "") {
      return;
    }

    // print the block hashdigest
    std::cout << "{\"block_hashdigest\":\""
              << lmdb_helper::binary_hash_to_hex(binary_hash) << "\"";

    // only print count and source information once for this hash
    if (hashes->find(binary_hash) == hashes->end()) {

      // print the count
      size_t count = ro_manager->find_count(binary_hash);
      std::cout << ", \"count\":" << count;

      // evaluate the source list
      std::pair<size_t, uint64_t> pair =
                           calculate_source_list_size_id_pair(binary_hash);

      // print the source list ID
      std::cout << ", \"source_list_id\":" << pair.second;

      // print the source list unless the list is too long
      if (max_sources == 0 || pair.first <= max_sources) {
        print_source_list(binary_hash);
      }

      // remember this hash
      hashes->insert(binary_hash);
    }

    // close line
    std::cout << "}" << std::endl;
  }

  // print expanded feature line
  void expand_feature_line(const feature_line_t& feature_line) {

    // get the binary hash
    std::string binary_hash = lmdb_helper::hex_to_binary_hash(feature_line.feature);

    // reject invalid input
    if (binary_hash == "") {
      std::cout << "Error: Invalid hash: '" << feature_line.feature << "'\n";
      return;
    }

    // make sure the hash is in the DB
    if (ro_manager->find_count(binary_hash) == 0) {
      std::cout << "Error: Invalid hash, incorrect feature file or hash database, '" << feature_line.feature << "'\n";
      return;
    }

    // write the forensic path
    std::cout << feature_line.forensic_path << "\t";

    // write the hashdigest
    std::cout << feature_line.feature << "\t";

    // write the opening of the context
    std::cout << "[";

    // write the old context
    std::cout << feature_line.context;

    // only print count and source information once for this hash
    if (hashes->find(binary_hash) == hashes->end()) {

      // write a separator
      std::cout << ",";

      // calculate the source list ID
      std::pair<size_t, uint64_t> pair =
                           calculate_source_list_size_id_pair(binary_hash);

      // print the source list ID
      std::cout << "{\"source_list_id\":" << pair.second;

      // print the source list unless the list is too long
      if (max_sources == 0 || pair.first <= max_sources) {
        print_source_list(binary_hash);
      }

      // close the source list ID
      std::cout << "}";

      // remember this hash
      hashes->insert(binary_hash);
    }

    // write the closure of the context
    std::cout << "]" << std::endl;
  }
};

#endif

