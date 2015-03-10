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
 * Defines the static commands that hashdb_manager can execute.
 */

#ifndef EXPLAIN_MANAGER_HPP
#define EXPLAIN_MANAGER_HPP

// Standard includes
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>

/**
 * Gather hash and source data and print explanation.
 */

class explain_manager_t {
  private:
  const lmdb_ro_manager_t* ro_manager;
  const uint32_t requested_max;
  std::set<std::string>* hashes;                // hash
  std::set<uint64_t>* source_lookup_indexes;    // source ID

  typedef std::set<std::string>::const_iterator hashes_it_t;

  // do not allow copy or assignment
  explain_manager_t(const explain_manager_t&);
  explain_manager_t& operator=(const explain_manager_t&);

  public:
  explain_manager_t(lmdb_ro_manager_t* p_ro_manager, uint32_t p_requested_max) :
                         ro_manager(p_ro_manager),
                         requested_max(p_requested_max),
                         hashes(),
                         source_lookup_indexes() {
    hashes = new std::set<std::string>;
    source_lookup_indexes = new std::set<uint64_t>;
  }

  ~explain_manager_t() {
    delete hashes;
    delete source_lookup_indexes;
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

    // make sure the hash is in the DB
    size_t count = ro_manager->find_count(binary_hash);
    if (count == 0) {
      std::cout << "Error: Invalid hash, incorrect feature file or hash database, '" << feature_line.feature << "'\n";
      return;
    }

    // add hash to hash set if not already there
    std::pair<hashes_it_t, bool> insert_pair = hashes->insert(binary_hash);

    // do not re-process this hash if it is already in the hash set
    if (insert_pair.second == false) {
      return;
    }
  
    // do not add sources for this hash if count > requested max
    if (ro_manager->find_count(binary_hash) > requested_max) {
      return;
    }

    // add each source relating to this hash
    lmdb_hash_it_data_t hash_it_data = ro_manager->find_first(binary_hash);
    while (hash_it_data.binary_hash == binary_hash) {

      // add the source lookup index to the source lookup index set
      source_lookup_indexes->insert(hash_it_data.source_lookup_index);

      hash_it_data = ro_manager->find_next(hash_it_data);
    }
  }

  // print table of relevant hashes
  void print_identified_hashes() const {

    // iterate through block hashes, printing hashes with identified sources
    bool has_reportable_hash = false;
    for (hashes_it_t it = hashes->begin(); it != hashes->end(); ++it) {

      // get this iteration's hash and count
      const std::string binary_hash = *it;
      const size_t count = ro_manager->find_count(binary_hash);

      // start preparing this block hash line
      std::stringstream ss;
      ss << "[\"" << lmdb_helper::binary_hash_to_hex(binary_hash)
         << "\",{\"count\":" << count
         << "},["; // count closure followed by source array open bracket

      // track when to put in the comma
      bool has_identified_source = false;

      // print the identified sources associated with this identified hash
      lmdb_hash_it_data_t hash_it_data = ro_manager->find_first(binary_hash);
      while (hash_it_data.binary_hash == binary_hash) {

        // skip sources that are not identified
        if (source_lookup_indexes->find(hash_it_data.source_lookup_index) ==
                                                source_lookup_indexes->end()) {

          // next
          hash_it_data = ro_manager->find_next(hash_it_data);
          continue;
        }

        // maybe prepend comma
        if (has_identified_source) {
          ss << ",";
        } else {
          has_identified_source = true;
        }

        // add the source_id and file_offset entry
        ss << "{\"source_id\":" << hash_it_data.source_lookup_index
           << ",\"file_offset\":" << hash_it_data.file_offset;

        // add associated hash label, if present
        if (hash_it_data.hash_label != "") {
          ss << ",\"label\":\"" << hash_it_data.hash_label << "\"";
        }

        // close source entry
        ss << "}";

        // next
        hash_it_data = ro_manager->find_next(hash_it_data);
      }

      // close source array and this block hash
      ss << "]]";

      // print the identified hash if it has at least one identified source
      if (has_identified_source) {
        has_reportable_hash = true;
        std::cout << ss.str() << std::endl;
      }
    }
    if (!has_reportable_hash) {
      std::cout << "# There are no hashes to report.\n";
      return;
    }

  }

  // print table of relevant sources
  void print_identified_sources() const {

    if (source_lookup_indexes->size() == 0) {
      std::cout << "# There are no sources to report.\n";
      return;
    }

    // iterate through sources
    for (std::set<uint64_t>::iterator it = source_lookup_indexes->begin();
                                   it!=source_lookup_indexes->end(); ++it) {

      // get source data
      lmdb_source_data_t source_data = ro_manager->find_source(*it);
      lmdb_source_it_data_t source_it_data(*it, source_data, true);

      // print the source
      print_helper::print_source_fields(source_it_data);
    }
  }
};

#endif

