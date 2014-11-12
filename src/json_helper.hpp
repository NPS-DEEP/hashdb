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
 * Defines the static formatter for json outuput
 */

#ifndef JSON_HELPER_HPP
#define JSON_HELPER_HPP
#include "hashdb_manager.hpp"
#include "source_metadata.hpp"

// Standard includes
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>

/**
 * Support ODR for json formatting.
 */

class json_helper_t {

  public:

/*
  // print the scan output vector
  static void print_scan_output(
              const std::vector<hash_t>& scan_input,
              const typename hashdb_t__<hash_t>::scan_output_t& scan_output) {

    // check that there are matches
    if (scan_output.size() == 0) {
      std::cout << "There are no matches.\n";
      return;
    }

    // print matches
    for (hashdb_t__<hash_t>::scan_output_t::const_iterator it=scan_output.begin(); it != scan_output.end(); ++it) {
      std::cout << "[\"" << scan_input[it->first] << "\",{\"count\":" << it->second << "}]\n";
    }
  }
*/

  // print source fields
  static void print_source_fields(const hashdb_manager_t& hashdb_manager,
                                  uint64_t source_lookup_index,
                                  std::ostream& os) {
    // get the repository name and filename
    std::pair<bool, std::pair<std::string, std::string> > source_pair =
                             hashdb_manager.find_source(source_lookup_index);

    // get source metadata, if available
    std::pair<bool, source_metadata_t> metadata_pair =
                     hashdb_manager.find_source_metadata(source_lookup_index);

    // print the source ID
    os << "\"source_id\":" << source_lookup_index;

    // print the source
    if (source_pair.first == true) {
      os << ",\"repository_name\":\"" << source_pair.second.first
         << "\",\"filename\":\"" << source_pair.second.second
         << "\"";
    }

    if (metadata_pair.first == true) {
      // print the metadata
      os << ",\"filesize\":" << metadata_pair.second.filesize
         << ",\"file_hashdigest\":\"" << metadata_pair.second.hashdigest.hexdigest()
         << "\"";
    }
  }
};

#endif

