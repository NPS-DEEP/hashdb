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

#ifndef COMMANDS_EXPLAIN_HPP
#define COMMANDS_EXPLAIN_HPP

#include "globals.hpp"
#include "json_formatter.hpp"

// Standard includes
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>

/**
 * Provides the commands that hashdb_manager can execute.
 * This totally static class is a class rather than an namespace
 * so it can have private members.
 */

class commands_explain {
  private:
  // support expalin_identified_blocks
  typedef std::map<hash_t, std::string> hashes_t;


  // ingest table of relevant hashes and table of their sources
  static void identify_hashes_and_sources(
                            const hashdb_manager_t& hashdb_manager,
                            const std::string& identified_blocks_file,
                            uint32_t requested_max,
                            hashes_t& hashes,
                            std::set<uint64_t>& source_lookup_indexes) {

    // get the identified_blocks.txt file reader
    feature_file_reader_t reader(identified_blocks_file);
    while (!reader.at_eof()) {

      feature_line_t feature_line = reader.read();

      // validate the hash string
      std::pair<bool, hash_t> hash_pair = safe_hash_from_hex(feature_line.feature);
      if (hash_pair.first == false) {
        // bad hash value
        continue;
      }
      hash_t hash = hash_pair.second;

      // add hash to hash set
      std::pair<std::map<hash_t, std::string>::iterator, bool> insert_pair =
                         hashes.insert(std::pair<hash_t, std::string>(
                         hash, feature_line.context));

      // do not re-process hashes that are already in the hash set
      if (insert_pair.second == false) {
        continue;
      }
  
      // do not add sources for this hash if count > requested max
      if (hashdb_manager.find_count(hash) > requested_max) {
        continue;
      }

      // get the iterator for this hash value
      hash_store_key_iterator_range_t it_pair = hashdb_manager.find(hash);

      // add the source lookup index for all sources associated with this hash
      for (; it_pair.first != it_pair.second; ++it_pair.first) {

        // get the source lookup index for this entry
        uint64_t source_lookup_index = hashdb_manager.source_id(it_pair.first);

        // add the source lookup index to the source lookup index set
        source_lookup_indexes.insert(source_lookup_index);
      }
    }
  }

  // print table of relevant hashes
  static void print_identified_hashes(
                            const hashdb_manager_t& hashdb_manager,
                            const hashes_t& hashes,
                            const std::set<uint64_t>& source_lookup_indexes) {

    if (hashes.size() == 0) {
      std::cout << "# There are no hashes to report.\n";
      return;
    }

    // iterate through block hashes
    for (hashes_t::const_iterator it = hashes.begin(); it != hashes.end(); ++it) {

      // prepare block hash line to print
      std::stringstream ss;
      ss << "[\"" << it->first << "\"";

      // get the context field
      std::string context = it->second;

      // add the reduced context field
      ss << "," << context;

      // add the source array open bracket
      ss << ",[";

      // get the multimap iterator for this hash value
      hash_store_key_iterator_range_t it_pair = hashdb_manager.find(it->first);

      // the user did something wrong if there is no range
      if (it_pair.first == it_pair.second) {
        std::cout << "# Invalid hash, incorrect file or database, " << it->first.hexdigest() << "\n";
        continue;
      }

      // track when to put in the comma
      bool found_identified_source = false;

      // print sources associated with this hash value
      for (; it_pair.first != it_pair.second; ++it_pair.first) {

        // get the source lookup index and file offset for this entry
        uint64_t source_lookup_index = hashdb_manager.source_id(it_pair.first);
        if (source_lookup_indexes.find(source_lookup_index) == source_lookup_indexes.end()) {
          // do not report sources that are not identified
          continue;
        }

        // consume identified source
        if (found_identified_source) {
          // prepend comma
          ss << ",";
        } else {
          // a printable source has been found
          found_identified_source = true;
        }

        // get the file offset
        uint64_t file_offset = hashdb_manager.file_offset(it_pair.first);

        // add source_id and file_offset entry
        ss << "{\"source_id\":" << source_lookup_index
           << ",\"file_offset\":" << file_offset
           << "}";
      }

      // done printing the source array
      ss << "]";

      // done printing this block hash
      ss << "]";

      // the block hash is interesting if it has at least one referenced source
      if (found_identified_source) {
        std::cout << ss.str() << "\n";
      }
    }
  }

  // print table of relevant sources
  static void print_identified_sources(
                            const hashdb_manager_t& hashdb_manager,
                            std::set<uint64_t>& source_lookup_indexes) {

    if (source_lookup_indexes.size() == 0) {
      std::cout << "# There are no sources to report.\n";
      return;
    }

    // iterate through sources
    for (std::set<uint64_t>::iterator it = source_lookup_indexes.begin();
                                   it!=source_lookup_indexes.end(); ++it) {

      // print the source
      std::cout << "{";
      print_source_fields(hashdb_manager, *it, std::cout);
      std::cout << "}\n";
    }
  }

  public:
  // explain identified_blocks.txt
  static void explain_identified_blocks(
                            const std::string& hashdb_dir,
                            const std::string& identified_blocks_file,
                            uint32_t requested_max) {

    // open hashdb
    hashdb_manager_t hashdb_manager(hashdb_dir, READ_ONLY);

    // create a hash set for tracking hashes that will be used
    hashes_t* hashes = new hashes_t;

    // create a source lookup index set for tracking source lookup indexes
    std::set<uint64_t>* source_lookup_indexes = new std::set<uint64_t>;

    // ingest table of relevant hashes and table of relevant sources
    identify_hashes_and_sources(hashdb_manager, identified_blocks_file,
                                requested_max, 
                                *hashes, *source_lookup_indexes);

    // print header information
    print_header("explain_identified_blocks-command-Version: 2");

    // print identified hashes
    std::cout << "# hashes\n";
    print_identified_hashes(hashdb_manager, *hashes, *source_lookup_indexes);

    // print identified sources
    std::cout << "# sources\n";
    print_identified_sources(hashdb_manager, *source_lookup_indexes);

    // clean up
    delete hashes;
    delete source_lookup_indexes;
  }
};

#endif

