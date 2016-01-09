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
 * Provides the service of importing hash data from a file formatted
 * using tab delimited fields, specifically:
 * <file hash>\t<block hash>\t<block offset>\n
 *
 * zlib reader adapted from online file zpipe.c
 */

#ifndef TAB_HASHDIGEST_READER_HPP
#define TAB_HASHDIGEST_READER_HPP

#include <zlib.h>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include "hashdb.hpp"
#include "progress_tracker.hpp"
#include "hex_helper.hpp"

class tab_hashdigest_reader_t {
  private:
  const std::string& hashdb_dir;
  const std::string& tab_file;
  const std::string& repository_name;
  const std::string& cmd;
  size_t line_number;

  static const uint32_t sector_size = 512;

  // do not allow these
  tab_hashdigest_reader_t();
  tab_hashdigest_reader_t(const tab_hashdigest_reader_t&);
  tab_hashdigest_reader_t& operator=(const tab_hashdigest_reader_t&);

  void add_line(const std::string& line, hashdb::import_manager_t& manager,
                progress_tracker_t& progress_tracker) {
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
    std::string file_binary_hash = hex_to_binary_hash(file_hash_string);
    if (file_binary_hash.size() == 0) {
      std::cerr << "file hexdigest is invalid on line " << line_number
                << ": '" << line << "', '" << file_hash_string << "'\n";
      return;
    }

    // get block hash 
    std::string block_hashdigest_string = line.substr(
                                  tab_index1+1, tab_index2 - tab_index1 - 1);
    std::string block_binary_hash = hex_to_binary_hash(block_hashdigest_string);
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

    // add source file hash
    std::pair<bool, uint64_t> pair = manager.insert_source_file_hash(
                                                         file_binary_hash);
    uint64_t source_id = pair.second;

    if (pair.first == true) {
      // source is new so add name and data for it
      manager.insert_source_name(source_id, repository_name, tab_file);
      manager.insert_source_data(source_id, file_binary_hash, 0, "", 0);
    }

    // add block hash
    bool is_added = manager.insert_hash(block_binary_hash);
    if (is_added) {
      // hash is new so add hash data and source
      manager.insert_hash_data(block_binary_hash, "", 0, "");
      manager.insert_hash_source(block_binary_hash, source_id, file_offset);
    }

    // update progress tracker
    progress_tracker.track();
  }
 
  public:
  tab_hashdigest_reader_t(
                     const std::string& p_hashdb_dir,
                     const std::string& p_tab_file,
                     const std::string& p_repository_name,
                     const std::string& p_cmd) :
                  hashdb_dir(p_hashdb_dir),
                  tab_file(p_tab_file),
                  repository_name(p_repository_name),
                  cmd(p_cmd),
                  line_number(0) {
  }

  // read tab file
  std::pair<bool, std::string> read() {

    // validate hashdb_dir path
    std::pair<bool, std::string> pair;
    pair = hashdb::is_valid_hashdb(hashdb_dir);
    if (pair.first == false) {
      return pair;
    }

    // validate and open text file
    std::ifstream in(tab_file.c_str());
    if (!in.is_open()) {
      std::stringstream ss;
      ss << "Cannot open " << tab_file << ": " << strerror(errno);
      return std::pair<bool, std::string>(false, ss.str());
    }

    // open hashdb manager
    hashdb::import_manager_t manager(hashdb_dir, cmd);

    // open progress tracker
    progress_tracker_t progress_tracker(hashdb_dir, 0, false, cmd);

    // process lines
    std::string line;
    while(getline(in, line)) {
      ++line_number;
      add_line(line, manager, progress_tracker);
    }

    // done
    return std::pair<bool, std::string>(true, "");
  }
};

#endif

