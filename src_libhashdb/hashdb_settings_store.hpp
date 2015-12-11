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
 * This file manages the hashdb settings.
 */

#ifndef    HASHDB_SETTINGS_STORE_HPP
#define    HASHDB_SETTINGS_STORE_HPP

#include "hashdb_settings.hpp"
#include "hashdb_settings_reader.hpp"
#include "hashdb.hpp" // for globals
#include <dfxml_writer.h>
#include <unistd.h>
#include <string>
#include <sstream>
#include <stdint.h>
#include <iostream>

// hashdb tuning options
class hashdb_settings_store_t {
  public:

  static hashdb_settings_t read_settings(const std::string& hashdb_dir) {

    // hashdb_dir containing settings.xml must exist
    std::string filename = hashdb_dir + "/settings.xml";
    if (access(filename.c_str(), F_OK) != 0) {
      std::cerr << "Unable to read database '" << hashdb_dir
                << "'.\nAborting.\n";
      exit(1);
    }

    // read settings
    hashdb_settings_t settings;
    hashdb_settings_reader_t::read_settings(filename, settings);

    // validate that the settings version is compatible with hashdb
    if (settings.data_store_version != hashdb::data_store_version) {
      std::cerr << "Database version error in settings version.\n"
                << "Database '" << hashdb_dir << "' uses data store version " << settings.data_store_version
                << "\nbut hashdb requires data store version " << hashdb::data_store_version
                << ".\nAborting.\n";
      exit(1);
    }

    return settings;
  }

  static void write_settings(const std::string& hashdb_dir,
                             const hashdb_settings_t& settings) {

    // calculate the settings filename
    std::string filename = hashdb_dir + "/settings.xml";
    std::string filename_old = hashdb_dir + "/_old_settings.xml";

    // if present, move existing settings to old
    if (access(filename.c_str(), F_OK) == 0) {
      std::remove(filename_old.c_str());
      int status = std::rename(filename.c_str(), filename_old.c_str());
      if (status != 0) {
        std::cerr << "Warning: unable to back up '" << filename
                  << "' to '" << filename_old << "': "
                  << strerror(status) << "\n";
      }
    }

    // write out the settings
    dfxml_writer x(filename, false);
    x.push("settings");
    settings.report_settings(x);
    x.pop();
  }
};

#endif

