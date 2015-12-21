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
#include "rapidjson.h"
#include "writer.h"
#include "document.h"
#include <unistd.h>
#include <string>
#include <sstream>
#include <stdint.h>
#include <iostream>

// hashdb tuning options
namespace hashdb_settings_store {

  std::pair<bool, std::string> read_settings(const std::string& hashdb_dir,
                                             hashdb_settings_t& settings) {

    // path must exist
    if (access(hashdb_dir.c_str(), F_OK) != 0) {
      return std::pair<bool, std::string>(false, "No hashdb at path '"
                       + hashdb_dir + "'.");
    }

    // settings file must exist
    std::string filename = hashdb_dir + "/settings.json";
    if (access(filename.c_str(), F_OK) != 0) {
        return std::pair<bool, std::string>(false, "Path '"
                     + hashdb_dir + "' is not a hashdb database.");
    }

    // open settings file
    std::ifstream in(filename);
    if (!in.is_open()) {
        return std::pair<bool, std::string>(false,
               "Unable to open settings file at Path '" + hashdb_dir + ".");
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
      return std::pair<bool, std::string>(false,
                      "Empty settings file at path '" + hashdb_dir + "'.");
    }

    // parse settings into a JSON DOM document
    rapidjson::Document document;
    if (document.Parse(line.c_str()).HasParseError()) {

      return std::pair<bool, std::string>(false,
                      "Invalid settings file at path '" + hashdb_dir + "'.");
    }
    if (!document.IsObject()) {
      return std::pair<bool, std::string>(false,
                      "Invalid JSON in settings file at path '" +
                      hashdb_dir + "'.");
    }

    settings.data_store_version = document["data_store_version"].GetUint64();
    settings.sector_size = document["sector_size"].GetUint64();
    settings.block_size = document["block_size"].GetUint64();
    settings.bloom_is_used = document["bloom_is_used"].GetBool();
    settings.bloom_M_hash_size = document["bloom_M_hash_size"].GetUint64();
    settings.bloom_k_hash_functions =
                              document["bloom_k_hash_functions"].GetUint64();

    // settings version must be compatible
    if (settings.data_store_version < settings.expected_data_store_version) {
      return std::pair<bool, std::string>(false, "The hashdb at path '"
                                     + hashdb_dir + "' is not compatible.");
    }

    // accept the read
    return std::pair<bool, std::string>(true, "");
  }


  static void write_settings(const std::string& hashdb_dir,
                             const hashdb_settings_t& settings) {

    // calculate the settings filename
    std::string filename = hashdb_dir + "/settings.json";
    std::string filename_old = hashdb_dir + "/_old_settings.json";

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

    // create the json settings document
    rapidjson::Document settings_document;
    rapidjson::Document::AllocatorType& allocator =
                                          settings_document.GetAllocator();
    settings_document.SetObject();
    settings_document.AddMember("data_store_version",
                                settings.data_store_version, allocator);
    settings_document.AddMember("sector_size",
                                settings.sector_size, allocator);
    settings_document.AddMember("block_size",
                                settings.block_size, allocator);
    settings_document.AddMember("bloom_is_used",
                                settings.bloom_is_used, allocator);
    settings_document.AddMember("bloom_M_hash_size",
                                settings.bloom_M_hash_size, allocator);
    settings_document.AddMember("bloom_k_hash_functions",
                                settings.bloom_k_hash_functions, allocator);

    // create the json settings string
    rapidjson::StringBuffer strbuf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
    settings_document.Accept(writer);
    std::string json_settings_string = strbuf.GetString();

    // write out the settings
    std::ofstream out;
    out.open(filename);
    out << json_settings_string << "\n";
    out.close();
  }
};

#endif

