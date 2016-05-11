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
 * Provides the service of importing hash data from a file formatted
 * using tab delimited fields, specifically:
 * <file hash>\t<block hash>\t<block offset>\n
 */

#ifndef IMPORT_TAB_HPP
#define IMPORT_TAB_HPP

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include "../src_libhashdb/hashdb.hpp"
#include "progress_tracker.hpp"

class import_tab_t {
  private:
  // state
  const std::string& hashdb_dir;
  const std::string& tab_file;
  const std::string& repository_name;
  const std::string& whitelist_dir;
  size_t line_number;

  // resources
  hashdb::import_manager_t manager;
  hashdb::scan_manager_t* whitelist_manager;
  progress_tracker_t progress_tracker;

  static const uint32_t sector_size = 512;

  // do not allow these
  import_tab_t();
  import_tab_t(const import_tab_t&);
  import_tab_t& operator=(const import_tab_t&);

  void add_line(const std::string& line) {

    // skip comment lines
    if (line[0] == '#') {
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
    size_t tab_index2 = line.find('\t', tab_index1 + 1);
    if (tab_index2 == std::string::npos) {
      std::cerr << "Second tab not found on line " << line_number << ": '" << line << "'\n";
      return;
    }

    // get file hash
    std::string file_hash_string = line.substr(0, tab_index1);
    std::string file_binary_hash = hashdb::hex_to_bin(file_hash_string);
    if (file_binary_hash.size() == 0) {
      std::cerr << "file hexdigest is invalid on line " << line_number
                << ": '" << line << "', '" << file_hash_string << "'\n";
      return;
    }

    // get block hash 
    std::string block_hashdigest_string = line.substr(
                                  tab_index1+1, tab_index2 - tab_index1 - 1);
    std::string block_binary_hash = hashdb::hex_to_bin(block_hashdigest_string);
    if (block_binary_hash == "") {
      std::cerr << "Invalid block hash on line " << line_number
                << ": '" << line << "', '" << block_hashdigest_string << "'\n";
      return;
    }

    // get file offset
    size_t sector_index;
    sector_index = std::atol(line.substr(tab_index2 + 1).c_str());
    if (sector_index == 0) {
      // index starts at 1 so 0 is invalid
      std::cerr << "Invalid sector index on line " << line_number
                << ": '" << line << "', '"
                << line.substr(tab_index2 + 1) << "'\n";
      return;
    }
    uint64_t file_offset = (sector_index - 1) * sector_size;

    // mark with "w" if in whitelist
    std::string whitelist_flag = "";
    if (whitelist_manager != NULL) {
      if (whitelist_manager->find_hash_count(block_binary_hash) > 0) {
        whitelist_flag = "w";
      }
    }

    // add source data
    manager.insert_source_data(file_binary_hash, 0, "", 0);

    // add name pair
    manager.insert_source_name(file_binary_hash, repository_name, tab_file);

    // add block hash
    manager.insert_hash(block_binary_hash, file_binary_hash, file_offset,
                        0, whitelist_flag);

    // update progress tracker
    progress_tracker.track();
  }
 
  import_tab_t(const std::string& p_hashdb_dir,
               const std::string& p_tab_file,
               const std::string& p_repository_name,
               const std::string& p_whitelist_dir,
               const std::string& cmd) :
                  hashdb_dir(p_hashdb_dir),
                  tab_file(p_tab_file),
                  repository_name(p_repository_name),
                  whitelist_dir(p_whitelist_dir),
                  line_number(0),
                  manager(hashdb_dir, cmd),
                  whitelist_manager(NULL),
                  progress_tracker(hashdb_dir, 0, cmd) {
    if (whitelist_dir != "") {
      whitelist_manager = new hashdb::scan_manager_t(whitelist_dir);
    }
  }

  ~import_tab_t() {
    if (whitelist_manager != NULL) {
      delete whitelist_manager;
    }
  }

  void read_lines(std::istream& in) {
    std::string line;
    while(getline(in, line)) {
      ++line_number;
      add_line(line);
    }
  }

  public:
  // read tab file
  static void read(const std::string& hashdb_dir,
                   const std::string& tab_file,
                   const std::string& repository_name,
                   const std::string& whitelist_dir,
                   const std::string& cmd,
                   std::istream& in) {

    // create the reader
    import_tab_t reader(hashdb_dir, tab_file, repository_name, whitelist_dir,
                        cmd);

    // read the lines
    reader.read_lines(in);
  }
};

#endif

