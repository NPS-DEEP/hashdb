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
#include "crc32.h"

static const std::string file_binary_hash = "\0";

class tab_hashdigest_reader_t {
  private:
  const std::string& hashdb_dir;
  const std::string& tab_file;
  const std::string& repository_name;
  const std::string& whitelist_dir;
  const bool skip_low_entropy;
  const std::string& cmd;
  size_t line_number;
  hashdb::hash_data_list_t data; // only gets size 1

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

    // parse filename
    std::string filename = line.substr(0, tab_index1);

    // binary filename is not available, so use a CRC of the filename
    // get CRC32 of filename
    uint32_t filename_crc = crc32(filename);
    std::stringstream ss;
    ss << filename_crc;
    std::string filename_crc_string(ss.str());

    // parse block hashdigest
    std::string block_hashdigest_string = line.substr(
                                  tab_index1+1, tab_index2 - tab_index1 - 1);

    // get binary hash from hex hash
    std::string binary_hash = hex_to_binary_hash(block_hashdigest_string);
    if (binary_hash == "") {
      // reject invalid input
      std::cerr << "Invalid hash on line " << line_number << ": '" << line << "', '" << block_hashdigest_string << "'\n";
      return;
    }

    // parse file offset
    size_t sector_index;
    sector_index = std::atol(line.substr(tab_index2+1).c_str());
    if (sector_index == 0) {
      // index starts at 1 so 0 is invalid
      std::cerr << "Invalid sector index on line " << line_number << ": '" << line << "', '" << line.substr(tab_index2+1) << "'\n";
      return;
    }
    uint64_t file_offset = (sector_index -1) * sector_size;

    // add the source entry
    manager.import_source_name(filename_crc_string, repository_name, filename);

    // add the hash
    data.clear();
    data.push_back(hashdb::hash_data_t(binary_hash, file_offset, ""));
    manager.import_source_data(file_binary_hash, 0, data);

    // update progress tracker
    progress_tracker.track();
  }
 
  public:
  tab_hashdigest_reader_t(
                     const std::string& p_hashdb_dir,
                     const std::string& p_tab_file,
                     const std::string& p_repository_name,
                     const std::string& p_whitelist_dir,
                     const bool p_skip_low_entropy,
                     const std::string& p_cmd) :
                  hashdb_dir(p_hashdb_dir),
                  tab_file(p_tab_file),
                  repository_name(p_repository_name),
                  whitelist_dir(p_whitelist_dir),
                  skip_low_entropy(p_skip_low_entropy),
                  cmd(p_cmd),
                  line_number(0),
                  data() { // data(1) would be nice since 1 element only
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

    // validate whitelist_dir path
    pair = hashdb::is_valid_hashdb(whitelist_dir);
    if (pair.first == false) {
      return pair;
    }

    // open hashdb manager
    hashdb::import_manager_t manager(hashdb_dir, whitelist_dir,
                                     skip_low_entropy, cmd);

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

