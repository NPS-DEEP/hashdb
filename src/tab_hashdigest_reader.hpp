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
#include "lmdb_rw_manager.hpp"
#include "lmdb_source_data.hpp"

class tab_hashdigest_reader_t {
  private:
  lmdb_rw_manager_t* rw_manager;
  progress_tracker_t* progress_tracker;
  const std::string& repository_name;
  const uint32_t sector_size;
  size_t line_number;

  // do not allow these
  tab_hashdigest_reader_t();
  tab_hashdigest_reader_t(const tab_hashdigest_reader_t&);
  tab_hashdigest_reader_t& operator=(const tab_hashdigest_reader_t&);

  void import_line(const std::string& line) {
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

    // parse block hashdigest
    std::string block_hashdigest_string = line.substr(
                                  tab_index1+1, tab_index2 - tab_index1 - 1);

    // parse file offset
    size_t sector_index;
    sector_index = std::atol(line.substr(tab_index2+1).c_str());
    if (sector_index == 0) {
      // index starts at 1 so 0 is invalid
      std::cerr << "Invalid sector index on line " << line_number << ": '" << line << "', '" << line.substr(tab_index2+1) << "'\n";
      return;
    }

    // get binary hash
    std::string binary_hash = lmdb_helper::hex_to_binary_hash(
                                                block_hashdigest_string);

    // reject invalid input
    if (binary_hash == "") {
      std::cerr << "Invalid hash on line " << line_number << ": '" << line << "', '" << block_hashdigest_string << "'\n";
      return;
    }

    // get source data
    lmdb_source_data_t source_data(repository_name, filename, 0, "");

    // get file offset
    uint64_t file_offset = (sector_index -1) * sector_size;

    // update progress tracker
    progress_tracker->track();

    // insert the entry
    rw_manager->insert(binary_hash, file_offset, sector_size, source_data);
  }
 
  public:
  tab_hashdigest_reader_t(lmdb_rw_manager_t* p_rw_manager,
                           progress_tracker_t* p_progress_tracker,
                           const std::string& p_repository_name,
                           uint32_t p_sector_size) :
              rw_manager(p_rw_manager),
              progress_tracker(p_progress_tracker),
              repository_name(p_repository_name),
              sector_size(p_sector_size),
              line_number(0) {
  }

  // read tab file
  std::pair<bool, std::string> read(std::string tab_file) {

    // open text file
    std::ifstream in(tab_file.c_str());
    if (!in.is_open()) {
      std::stringstream ss;
      ss << "Cannot open " << tab_file << ": " << strerror(errno);
      return std::pair<bool, std::string>(false, ss.str());
    }

    // process lines
    std::string line;
    while(getline(in, line)) {
      ++line_number;
      import_line(line);
    }
    return std::pair<bool, std::string>(true, "");
  }
};

#endif

