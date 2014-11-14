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
#include <boost/lexical_cast.hpp>
#include "hash_t_selector.h"

class tab_hashdigest_reader_t {
  private:
  hashdb_manager_t* hashdb_manager;
  progress_tracker_t* progress_tracker;
  const std::string& repository_name;
  const uint32_t sector_size;

  // do not allow these
  tab_hashdigest_reader_t();
  tab_hashdigest_reader_t(const tab_hashdigest_reader_t&);
  tab_hashdigest_reader_t& operator=(const tab_hashdigest_reader_t&);

  std::pair<bool, std::string> read_text(std::string tab_file) {

    // open text file
    std::ifstream in(tab_file);
    if (!in.is_open()) {
      std::stringstream ss;
      ss << "Cannot open " << tab_file << ": " << strerror(errno);
      return std::pair<bool, std::string>(false, ss.str());
    }

    // process lines
    std::string line;
    while(getline(in, line)) {
      import_line(line);
    }
    return std::pair<bool, std::string>(true, "");
  }

  void import_line(const std::string& line) {
    // skip comment lines
    if (line[0] == '#') {
      // skip comment line
      return;
    }

    // find tabs
    size_t tab_index1 = line.find('\t');
    if (tab_index1 == std::string::npos) {
      std::cerr << "tab not found in line: '" << line << "'\n";
      return;
    }
    size_t tab_index2 = line.find('\t', tab_index1 + 1);
    if (tab_index2 == std::string::npos) {
      std::cerr << "second tab not found in line: '" << line << "'\n";
      return;
    }

    // parse filename
    std::string filename = line.substr(0, tab_index1);

    // parse block hashdigest
    std::string block_hashdigest_string = line.substr(
                                  tab_index1+1, tab_index2 - tab_index1 - 1);
    std::pair<bool, hash_t> block_hashdigest_pair = safe_hash_from_hex(
                                                block_hashdigest_string);
    if (block_hashdigest_pair.first == false) {
      std::cerr << "invalid block hashdigest in line: '" << line << "'\n";
      return;
    }

    // parse file offset
    uint64_t file_offset;
    try {
      file_offset = boost::lexical_cast<uint64_t>(line.substr(tab_index2+1))
                                                              * sector_size;
    } catch(...) {
      std::cerr << "Invalid file offset in line: '" << line << "'\n";
      return;
    }

    // create the hashdb element
    hashdb_element_t hashdb_element(
               block_hashdigest_pair.second,             // block hash
               hashdb_manager->settings.hash_block_size, // block size
               repository_name,                          // repository name
               filename,                                 // filename
               file_offset);                             // file offset

    // update progress tracker
    progress_tracker->track();

    // import
    hashdb_manager->insert(hashdb_element);
  }
 
  public:
  tab_hashdigest_reader_t(hashdb_manager_t* p_hashdb_manager,
                           progress_tracker_t* p_progress_tracker,
                           const std::string& p_repository_name,
                           uint32_t p_sector_size) :
              hashdb_manager(p_hashdb_manager),
              progress_tracker(p_progress_tracker),
              repository_name(p_repository_name),
              sector_size(p_sector_size) {
  }

  std::pair<bool, std::string> read(std::string tab_file) {
    return read_text(tab_file);
  }
};

#endif

