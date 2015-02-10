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

#ifndef COMMANDS_HELPER_HPP
#define COMMANDS_HELPER_HPP
#include "globals.hpp"
#include <string>

/**
 * Provides misc. command support.
 */

class commands_helper {

  static void require_compatibility(const std::string hashdb_dir1,
                                    const std::string hashdb_dir2) {

    // databases should not be the same one
    if (hashdb_dir1 == hashdb_dir2) {
      std::cerr << "Error: the databases must not be the same one:\n'"
                << hashdb_dir1 << "', '"
                << hashdb_dir2 << "'\n"
                << "Aborting.\n";
      exit(1);
    }
  }

  static void require_compatibility(const std::string hashdb_dir1,
                                    const std::string hashdb_dir2,
                                    const std::string hashdb_dir3) {

    // databases should not be the same one
    if (hashdb_dir1 == hashdb_dir2
     || hashdb_dir2 == hashdb_dir3
     || hashdb_dir3 == hashdb_dir1) {
      std::cerr << "Error: the databases must not be the same one:\n'"
                << hashdb_dir1 << "', '"
                << hashdb_dir2 << "', '"
                << hashdb_dir3 << "'\n"
                << "Aborting.\n";
      exit(1);
    }
  }

  // print header information
  static void print_header(const std::string& command_id) {
    std::cout << "# hashdb-Version: " << PACKAGE_VERSION << "\n"
              << "# " << command_id << "\n"
              << "# command_line: " << globals_t::command_line_string << "\n";
  }


/*
//zzzzzzzzzzzz
  // print the scan output vector
  static void print_scan_output(
              const std::vector<hash_t>& scan_input,
              const hashdb_t__<hash_t>::scan_output_t& scan_output) {

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
*/







};

#endif

