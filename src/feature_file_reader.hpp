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
 * Provides a feature file reader service.
 */

#ifndef FEATURE_FILE_READER_HPP
#define FEATURE_FILE_READER_HPP

// Standard includes
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <errno.h>
#include <cassert>
#include "feature_line.hpp"

class feature_file_reader_t {

  std::string filename;
  std::ifstream in;

  feature_line_t feature_line;
  bool at_end;

  // successfully read feature or set at_end=true
  void read_feature() {

    // parse the next available line
    std::string line;
    while(getline(in, line)) {

      // skip comment lines
      if (line[0] == '#') {
        // not valid
        continue;
      }

      // parse line of "offset tab hexdigest tab count"

      // find tabs
      size_t tab_index1 = line.find('\t');
      if (tab_index1 == std::string::npos) {
        continue;
      }
      size_t tab_index2 = line.find('\t', tab_index1 + 1);
      if (tab_index2 == std::string::npos) {
        continue;
      }

      // forensic path
      std::string forensic_path = line.substr(0, tab_index1);

      // feature
      std::string feature = line.substr(tab_index1+1, tab_index2 - tab_index1 - 1);

      // context
      std::string context = line.substr(tab_index2+1);

      feature_line = feature_line_t(forensic_path, feature, context);

      return;
    }

    // at eof
    at_end = true;
  }

  public:
  feature_file_reader_t(std::string p_filename) :
              filename(p_filename),
              in(filename.c_str()),
              feature_line("","",""),
              at_end(false) {

    // see that the file opened
    if (!in.is_open()) {
      std::cout << "Cannot open " << filename << ": " << strerror(errno) << "\n";
      exit(1);
    }

    // read the first feature
    read_feature();
  }

  feature_line_t read() {
    if (at_end) {
      // program error
      assert(0);
    }
    feature_line_t temp = feature_line;
    read_feature();
    return temp;
  }

  // at EOF
  bool at_eof() {
    return at_end;
  }
};

#endif

