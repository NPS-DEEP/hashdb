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
 * Scan for hashes in file where lines are "<forensic path><tab><hex hash>".
 * Comment lines are forwarded to output.
 */

#ifndef SCAN_HASHES_HPP
#define SCAN_HASHES_HPP

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include "../src_libhashdb/hashdb.hpp"
#include "progress_tracker.hpp"

class scan_hashes_t {
  private:
  // state
  const std::string& hashdb_dir;
  size_t line_number;

  // resources
  hashdb::scan_manager_t manager;

  // do not allow these
  scan_hashes_t();
  scan_hashes_t(const scan_hashes_t&);
  scan_hashes_t& operator=(const scan_hashes_t&);

  void scan_line(const std::string& line) {

    // print comment lines
    if (line[0] == '#') {
      // forward to stdout
      std::cout << line << "\n";
      return;
    }

    // skip empty lines
    if (line.size() == 0) {
      return;
    }

    // find tabs
    size_t tab_index1 = line.find('\t');
    if (tab_index1 == std::string::npos) {
      std::cerr << "Tab not found on line " << line_number << ": '" << line << "'\n";
      return;
    }

    // get forensic path
    std::string forensic_path = line.substr(0, tab_index1);

    // get block hash
    std::string block_hashdigest_string = line.substr(tab_index1+1);
    std::string block_binary_hash = hashdb::hex_to_bin(block_hashdigest_string);
    if (block_binary_hash == "") {
      std::cerr << "Invalid block hash on line " << line_number
                << ": '" << line << "'\n";
      return;
    }

    // scan
    std::string expanded_text;
    bool found = manager.find_expanded_hash(block_binary_hash, expanded_text);
    if (found == true) {
      std::cout << forensic_path << "\t"
                << block_hashdigest_string << "\t"
                << expanded_text << std::endl;
    }
  }
 
  scan_hashes_t(const std::string& p_hashdb_dir,
                const std::string& cmd) :
                     hashdb_dir(p_hashdb_dir),
                     line_number(0),
                     manager(hashdb_dir) {
  }

  void read_lines(std::istream& in) {
    std::string line;
    while(getline(in, line)) {
      ++line_number;
      scan_line(line);
    }
  }

  public:
  // read tab file
  static void read(const std::string& hashdb_dir,
                   const std::string& cmd,
                   std::istream& in) {

    // create the reader
    scan_hashes_t reader(hashdb_dir, cmd);

    // write cmd
    std::cout << "# command: '" << cmd << "'\n"
              << "# hashdb-Version: " << PACKAGE_VERSION << "\n";

    // read the lines
    reader.read_lines(in);
  }
};

#endif

