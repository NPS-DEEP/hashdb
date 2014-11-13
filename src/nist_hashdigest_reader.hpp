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
 * using NIST syntax, specifically:
 * <file hash>\t<block hash>\t<block offset>\n
 */

#ifndef NIST_HASHDIGEST_READER_HPP
#define NIST_HASHDIGEST_READER_HPP

#include <zlib.h>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include "hash_t_selector.h"

class nist_hashdigest_reader_t {
  private:
  hashdb_manager_t* hashdb_manager;
  progress_tracker_t* progress_tracker;
  const std::string& repository_name;

  // do not allow these
  nist_hashdigest_reader_t();
  nist_hashdigest_reader_t(const nist_hashdigest_reader_t&);
  nist_hashdigest_reader_t& operator=(const nist_hashdigest_reader_t&);

  std::pair<bool, std::string> read_text(std::string nist_file) {

    // open text file
    std::ifstream in(nist_file);
    if (!in.is_open()) {
      std::stringstream ss;
      ss << "Cannot open " << nist_file << ": " << strerror(errno);
      return std::pair<bool, std::string>(false, ss.str());
    }

    // process lines
    std::string line;
    while(getline(in, line)) {
      import_line(line);
    }
    return std::pair<bool, std::string>(true, "");
  }

  std::pair<bool, std::string> read_zip(std::string nist_file) {
//zz TBD
      return std::pair<bool, std::string>(false, "");
  }


  void import_line(const std::string& line) {
std::cout << "nist.import_line.a\n";
    // skip comment lines
    if (line[0] == '#') {
      // not valid
      return;
    }

std::cout << "nist.import_line.b\n";
    // find tabs
    size_t tab_index1 = line.find('\t');
    if (tab_index1 == std::string::npos) {
      return;
    }
    size_t tab_index2 = line.find('\t', tab_index1 + 1);
    if (tab_index2 == std::string::npos) {
      return;
    }

std::cout << "nist.import_line.c\n";
    // file hashdigest
    std::string file_hashdigest_string = line.substr(0, tab_index1);
    std::pair<bool, hash_t> file_hashdigest_pair = safe_hash_from_hex(
                                                file_hashdigest_string);
    if (file_hashdigest_pair.first == false) {
      std::cerr << "invalid file hashdigest in line: '" << line << "'\n";
      return;
    }

std::cout << "nist.import_line.d\n";
    // block hashdigest
    std::string block_hashdigest_string = line.substr(tab_index1+1, tab_index2);
    std::pair<bool, hash_t> block_hashdigest_pair = safe_hash_from_hex(
                                                block_hashdigest_string);
    if (block_hashdigest_pair.first == false) {
      std::cerr << "invalid block hashdigest in line: '" << line << "'\n";
      return;
    }

std::cout << "nist.import_line.e\n";
    // file offset
    uint64_t file_offset;
    try {
      file_offset = boost::lexical_cast<uint64_t>(line.substr(tab_index2+1));
    } catch(...) {
      std::cerr << "Invalid file offset in line: '" << line << "'\n";
      return;
    }

std::cout << "nist.import_line.f\n";
    // create the hashdb element
    hashdb_element_t hashdb_element(
               block_hashdigest_pair.second,            // block hash
               hashdb_manager->settings.hash_block_size,
               repository_name,
               file_hashdigest_pair.second.hexdigest(), // use file hash for filename
               file_offset);

std::cout << "nist.import_line.g\n";
    // import
    hashdb_manager->insert(hashdb_element);
std::cout << "nist.import_line.h\n";
  }
 
  public:
  nist_hashdigest_reader_t(hashdb_manager_t* p_hashdb_manager,
                           progress_tracker_t* p_progress_tracker,
                           const std::string& p_repository_name) :
              hashdb_manager(p_hashdb_manager),
              progress_tracker(p_progress_tracker),
              repository_name(p_repository_name) {
  }

  std::pair<bool, std::string> read(std::string nist_file) {
    if (nist_file.size() > 4 &&
        nist_file.substr(nist_file.size() - 4, 4) == ".zip") {
      return read_zip(nist_file);
    } else {
      return read_text(nist_file);
    }
  }
};

#endif

