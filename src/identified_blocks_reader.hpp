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
 * Provides array<pair<string offset, string hashdigest>>.
 */

#ifndef IDENTIFIED_BLOCKS_READER_HPP
#define IDENTIFIED_BLOCKS_READER_HPP

// Standard includes
#include <cstdlib>
#include <cstdio>
#include <string>
#include <cstring>
#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>
//#include <vector>
#include <map>
#include <errno.h>
#include "identified_blocks_reader_iterator.hpp"

class identified_blocks_reader_t {

  std::string filename;
  std::ifstream in;

  public:
  identified_blocks_reader_t(std::string p_filename) :
                    filename(p_filename),
                    in(filename.c_str()) {

/*
    // identified_blocks.txt must exist
    if(access(filename.c_str(),R_OK)){
      std::cerr << "Error: unable to read identified_blocks.txt feature file '" << filename << "'.\n";
      std::cerr << "Cannot continue.\n";
      exit(1);
    }
*/

    // see that in initialized
    if (!in.is_open()) {
      std::cout << "Cannot open " << filename << ": " << strerror(errno) << "\n";
      exit(1);
    }
  }

  identified_blocks_reader_iterator_t begin() {
    return identified_blocks_reader_iterator_t(&in, false);
  }

  identified_blocks_reader_iterator_t end() {
    return identified_blocks_reader_iterator_t(&in, true);
  }
};

#endif

