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

#ifndef IDENTIFIED_SOURCES_WRITER_HPP
#define IDENTIFIED_SOURCES_WRITER_HPP

// Standard includes
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>

class identified_sources_writer_t {

  private:
  std::fstream outf;
  std::ostream *out;

  // do not allow copy or assignment
  identified_sources_writer_t(const identified_sources_writer_t&);
  identified_sources_writer_t& operator=(const identified_sources_writer_t&);

  public:
  identified_sources_writer_t(std::string p_filename) :
                    outf(p_filename.c_str()),
                    out(&outf) {

    // make sure the output file was opened
    if (!outf.is_open()) {
      perror(p_filename.c_str());
      exit(1);
    }
  }

//  void write_hashdb_element(std::string offset, hashdb_element_t hashdb_element)
  void write(std::string line) {
    *out << line;
  }
};

#endif

