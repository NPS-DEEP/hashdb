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
#include "globals.hpp"
#include "lmdb_ro_manager.hpp"
#include "lmdb_hash_it_data.hpp"
#include "lmdb_source_data.hpp"

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
  std::set<uint64_t>* source_list_ids;

  // do not allow copy or assignment
  expand_manager_t(const expand_manager_t&);
  expand_manager_t& operator=(const expand_manager_t&);

  // calculate source list CRC
  uint32_t calculate_source_list_id(const std::string binary_hash) {

    // start a source list ID CRC hash
    uint32_t crc = 0;

    // add each source ID to the CRC hash
    lmdb_hash_it_data_t hash_it_data = ro_manager->find_first(binary_hash);
    while (hash_it_data.binary_hash == binary_hash) {

      // add source ID to source list ID
      crc = crc32(crc, reinterpret_cast<const uint8_t*>(binary_hash.c_str()), binary_hash.size());
      hash_it_data = ro_manager->find_next(hash_it_data);
    }

    return crc;
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
        if (source_data.binary_hash != "") {
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
          source_list_ids() {
    source_ids = new std::set<uint64_t>;
    source_list_ids = new std::set<uint64_t>;

    // print header
    std::cout << "# hashdb-Version: " << PACKAGE_VERSION << "\n"
              << "# expand-hash-Version: 2\n"
              << "# command_line: " << globals_t::command_line_string << "\n";

  }

  ~expand_manager_t() {
    delete source_ids;
    delete source_list_ids;
  }

  // print header
  static void print_header() {
  }

  // print expanded hash
  void expand(const std::string& binary_hash) {

    // print the block hashdigest
    std::cout << "{\"block_hashdigest\":\""
              << lmdb_helper::binary_hash_to_hex(binary_hash) << "\"";

    // print the count
    size_t count = ro_manager->find_count(binary_hash);
    std::cout << ", \"count\":" << count;

    // calculate the source list ID
    uint64_t source_list_id = calculate_source_list_id(binary_hash);

    // print the source list ID
    std::cout << ", \"source_list_id\":" << source_list_id;

    // print the list of sources the first time unless it is too long
    if (count <= max_sources) {
      if (source_list_ids->find(source_list_id) == source_list_ids->end()) {
        source_list_ids->insert(source_list_id);
        print_source_list(binary_hash);
      }
    }

    // close line
    std::cout << "}" << std::endl;
  }
};

#endif

