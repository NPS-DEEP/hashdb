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
 * Read and write hashdb settings
 */

#ifndef SETTINGS_MANAGER_HPP
#define SETTINGS_MANAGER_HPP

#include <string>
#include <sstream>
#include <stdint.h>
#include <iostream>
#include <unistd.h>
#include <string.h> // for strerror
#include <cerrno>
#include <fstream>
#include "hashdb.hpp" // for settings
#include "rapidjson.h"
#include "writer.h"
#include "document.h"

namespace hashdb {

  bool read_settings(const std::string& hashdb_dir,
                     hashdb::settings_t& settings,
                     std::string& error_message) {

    // path must exist
    if (access(hashdb_dir.c_str(), F_OK) != 0) {
      error_message = "No hashdb at path '" + hashdb_dir + "'.";
      return false;
    }

    // settings file must exist
    std::string filename = hashdb_dir + "/settings.json";
    if (access(filename.c_str(), F_OK) != 0) {
      error_message = "Path '" + hashdb_dir + "' is not a hashdb database.";
      return false;
    }

    // open settings file
    std::ifstream in(filename.c_str());
    if (!in.is_open()) {
      error_message = "Unable to open settings file at Path '" +
                      hashdb_dir + "'.";
      return false;
    }

    // find and read the first line of content
    std::string line;
    while(getline(in, line)) {
      if (line.size() == 0 || line[0] == '#') {
        continue;
      }
      break;
    }
    in.close();
    if (line.size() == 0) {
      // no first line
      error_message = "Empty settings file at path '" + hashdb_dir + "'.";
      return false;
    }

    // parse settings into a JSON DOM document
    rapidjson::Document document;
    if (document.Parse(line.c_str()).HasParseError()) {

      error_message = "Invalid settings file at path '" + hashdb_dir + "'.";
      return false;
    }
    if (!document.IsObject()) {
      error_message = "Invalid JSON in settings file at path '" +
                      hashdb_dir + "'.";
      return false;
    }

    // parse the values
    settings.settings_version = document["settings_version"].GetUint64();
    settings.sector_size = document["sector_size"].GetUint64();
    settings.block_size = document["block_size"].GetUint64();
    settings.max_source_offset_pairs =
                          document["max_source_offset_pairs"].GetUint64();
    settings.hash_prefix_bits = document["hash_prefix_bits"].GetUint64();
    settings.hash_suffix_bytes = document["hash_suffix_bytes"].GetUint64();

    // settings version must be compatible
    if (settings.settings_version <
                             hashdb::settings_t::CURRENT_SETTINGS_VERSION) {
      error_message = "The hashdb at path '" + hashdb_dir + 
                      "' is not compatible.";
    }

    // accept the read
    error_message = "";
    return true;
  }


  static bool write_settings(const std::string& hashdb_dir,
                             const hashdb::settings_t& settings,
                             std::string& error_message) {

    // calculate the settings filename
    std::string filename = hashdb_dir + "/settings.json";
    std::string filename_old = hashdb_dir + "/_old_settings.json";

    // if present, try to move existing settings to old
    if (access(filename.c_str(), F_OK) == 0) {
      std::remove(filename_old.c_str());
      int status = std::rename(filename.c_str(), filename_old.c_str());
      if (status != 0) {
        std::cerr << "Warning: unable to back up '" << filename
                  << "' to '" << filename_old << "': "
                  << strerror(status) << "\n";
      }
    }

    // write the settings
    std::ofstream out;
    out.open(filename.c_str());
    if (!out.is_open()) {
      error_message = std::string(strerror(errno));
      return false;
    }

    out << settings.settings_string() << "\n";
    out.close();

    error_message = "";
    return true;
  }
}

#endif

