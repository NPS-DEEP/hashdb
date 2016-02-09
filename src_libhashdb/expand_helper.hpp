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
 * Support find_expanded_hash.
 */

#ifndef EXPAND_HELPER_HPP
#define EXPAND_HELPER_HPP
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <string>
#include <set>
#include "to_hex.hpp"

namespace hashdb {

  // helper for producing expanded source for a source ID
  static void provide_source_information(const hashdb::scan_manager_t& manager,
                                         const uint64_t source_id,
                                         std::stringstream& ss) {

    // fields to hold source information
    std::string file_binary_hash;
    uint64_t filesize;
    std::string file_type;
    uint64_t nonprobative_count;
    hashdb::source_names_t* source_names(new hashdb::source_names_t);

    // read source data
    manager.find_source_data(source_id, file_binary_hash, filesize,
                             file_type, nonprobative_count);

    // read source names
    manager.find_source_names(source_id, *source_names);

    // provide source data
    ss << "{\"source_id\":" << source_id
       << "\"file_hash\":\"" << hashdb::to_hex(file_binary_hash) << "\""
       << ",\"filesize\":" << filesize
       << ",\"file_type\":\"" << file_type << "\""
       << ",\"nonprobative_count\":" << nonprobative_count
       ;

    // provide source names
    ss << ",\"names\"[";
    int i = 0;
    hashdb::source_names_t::const_iterator it;
    for (it = source_names->begin(); it != source_names->end();
         ++it, ++i) {

      // put comma before all but first name pair
      if (i > 0) {
        ss << ",";
      }

      // provide name pair
      ss << "{\"repository_name\":\"" << escape_json(it->first)
         << "\",\"filename\":\"" << escape_json(it->second)
         << "\"}";
    }

    // close source names list and source data
    ss << "]}";

    delete source_names;
  }
}

#endif

