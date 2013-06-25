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
 * Reads from identified_blocks.txt into sources_request_md5_t structure.
 */

#ifndef IDENTIFIED_BLOCKS_READER_HPP
#define IDENTIFIED_BLOCKS_READER_HPP

// Standard includes
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>
//#include <vector>
#include <map>
#include "hashdb.hpp"
#include "dfxml/src/hash_t.h"
#include <errno.h>

class identified_blocks_reader_t {

  public:
  identified_blocks_reader_t(const std::string& filename,
                             hashdb::sources_request_md5_t& request,
                             std::map<uint32_t, std::string>& offset_map) {
    uint32_t offset_index = 0;
    request.clear();
    offset_map.clear();

    if(access(filename.c_str(),R_OK)){
      std::cerr << "Error: File " << filename << " is missing or unreadable.\n";
      std::cerr << "Cannot continue.\n";
      exit(1);
    }

    // get file stream
    std::fstream in(filename.c_str());
    if (!in.is_open()) {
      std::cout << "Cannot open " << filename << ": " << strerror(errno) << "\n";
      exit(1);
    }
 
    // parse each line
    std::string line;
    bool error = false;
    while(getline(in, line) && !error) {

      // skip comment lines
      if (line[0] == '#') {
        continue;
      }

      // find hash
      size_t tab_index = line.find('\t');
      if (tab_index == std::string::npos) {
        continue;
      }
      md5_t md5 = md5_t(md5_t::fromhex(line.substr(tab_index+1, 32)));
      uint8_t digest[16];
      memcpy(digest, md5.digest, 16);
      request.push_back(hashdb::source_request_md5_t(offset_index, digest, 0,0,0));
      offset_map[offset_index] = line.substr(0, tab_index);

      ++offset_index;
    }

    // close
    in.close();
  }
};

#endif

