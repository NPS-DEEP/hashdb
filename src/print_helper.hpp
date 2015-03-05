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
 * Defines static commands to help with printing output.
 */

#ifndef PRINT_HELPER_HPP
#define PRINT_HELPER_HPP

#include "globals.hpp"
#include "lmdb_helper.h"
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>

/**
 * Static helper class.
 */
class print_helper {

  public:
  // print header information
  static void print_header(const std::string& command_id) {
    std::cout << "# hashdb-Version: " << PACKAGE_VERSION << "\n"
              << "# " << command_id << "\n"
              << "# command_line: " << globals_t::command_line_string << "\n";
  }

  // print hash and its count
  static void print_hash(const std::string& binary_hash, size_t count) {
    std::cout << "[\"" << lmdb_helper::binary_hash_to_hex(binary_hash)
              << "\",{\"count\":" << count << "}]" << std::endl;
  }

  // print information about a source
  static void print_source_fields(const lmdb_source_it_data_t& source) {
    std::cout << "{\"source_id\":" << source.source_lookup_index
              << ",\"repository_name\":\""
              << lmdb_helper::escape_json(source.source_data.repository_name)
              << "\",\"filename\":\""
              << lmdb_helper::escape_json(source.source_data.filename)
              << "\"";
    if (source.source_data.filesize != 0) {
      std::cout << ",\"filesize\":" << source.source_data.filesize;
    }
    if (source.source_data.binary_hash != "") {
      std::cout << ",\"file_hashdigest\":\""
                << lmdb_helper::binary_hash_to_hex(source.source_data.binary_hash)
                << "\"";
    }
    std::cout << "}\n";
  }
};

#endif

