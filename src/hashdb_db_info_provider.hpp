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
 * Provides hashdb metadata information as a string.
 */

#ifndef HASHDB_DB_INFO_PROVIDER_HPP
#define HASHDB_DB_INFO_PROVIDER_HPP

#include "hashdb_db_manager.hpp"
#include <sstream>
#include <fstream>
#include <iostream>

/**
 * Provides hashdb metadata information as a string of XML information.
 */
class hashdb_db_info_provider_t {
  public:
  static void get_hashdb_info(const hashdb_db_manager_t& hashdb, std::string& info) {

    // for now, just return settings information

    // get the settings filename
    std::string settings_filename = hashdb_filenames_t::settings_filename(hashdb.hashdb_dir);

    // open the settings filename
    if(access(settings_filename.c_str(),R_OK)){
      std::cerr << "Error: File " << settings_filename << " is missing or unreadable.\n";
      std::cerr << "Cannot continue.\n";
      exit(1);
    }

    // get file stream
    std::fstream in(settings_filename.c_str());
    if (!in.is_open()) {
      std::cout << "Cannot open " << settings_filename << ": " << strerror(errno) << "\n";
      exit(1);
    }

    // make output stream
    std::stringstream ss;

    // parse each line
    std::string line;
    bool error = false;
    while(getline(in, line) && !error) {
      ss << line << std::endl;
    }

    // close resource
    in.close();

    info = ss.str();
  }

  private:
  hashdb_db_info_provider_t();
};

#endif

