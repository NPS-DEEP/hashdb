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
 * hex conversion code for the hashdb library.
 */

#include <config.h>
#include <string>
#include <cassert>
#include <iostream>
#include <unistd.h> // for F_OK
#include <sstream>
#include "hashdb.hpp"
#include "dig.h"

namespace hashdb {

  // ************************************************************
  // interfaces
  // ************************************************************

  // ingest
  std::string ingest(const std::string& hashdb_dir,
                     const std::string& ingest_path,
                     const std::string& p_repository_name,
                     const std::string& whitelist_dir,
                     const std::string& cmd) {

    bool has_whitelist = false;

    // make sure hashdb_dir is there
    std::string error_message;
    hashdb::settings_t settings;
    error_message = hashdb::read_settings(hashdb_dir, settings);
    if (error_message.size() != 0) {
      return error_message;
    }

    // make sure file or directory at ingest_path is readable
    if (access(hashdb_dir.c_str(), F_OK) != 0) {
      return "Invalid ingest path '" + ingest_path + "'.";
    }

    // establish repository_name
    const std::string repository_name =
            (p_repository_name.size() > 0) ? p_repository_name : ingest_path;

    // see if whitelist_dir is present
    error_message = hashdb::read_settings(whitelist_dir, settings);
    if (error_message.size() == 0) {
      has_whitelist = true;
    }

    // recursive directory walker
    hashdb::dig dig_tool(ingest_path);
    hashdb::dig::const_iterator it = dig_tool.begin();

    // iterate
    while (it != dig_tool.end()) {
#ifdef WIN32
      std::wcout << *it << "\n";
#else
      std::cout << *it << "\n";
#endif
      ++it;
    }

    // done
    std::cout << "TBD\n";

    // success
    return "";
  }

} // end namespace hashdb

