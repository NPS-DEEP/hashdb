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
#include "file_reader.hpp"

namespace hashdb {

  // ************************************************************
  // support interfaces
  // ************************************************************
  void print_processing(const hasher::filename_t& filename, const uint64_t filesize) {
    std::cout << "processing ";
#ifdef WIN32
    std::wcout << filename;
#else
    std::cout << filename;
#endif
    std::cout << ", size " << filesize << "...\n";
  }


  void print_skipping(const hasher::filename_t& filename, const uint64_t filesize,
                 const std::string message) {
    std::cout << "skipping ";
#ifdef WIN32
    std::wcout << filename;
#else
    std::cout << filename;
#endif
    std::cout << ", size " << filesize << ": " << message << "\n";
  }

//zz move this to hasher/ingest_file.hpp
  void ingest_file(const hasher::file_reader_t& reader,
                   import_manager_t& import_manager,
                   scan_manager_t* whitelist_scan_manager,
                   const std::string& repository_name) {

  // calculate file MD5
  // iterate file to calculate MD5

  //
//zz
  }


  // ************************************************************
  // ingest
  // ************************************************************
  std::string ingest(const std::string& hashdb_dir,
                     const std::string& ingest_path,
                     const size_t step_size,
                     const std::string& p_repository_name,
                     const std::string& whitelist_dir,
                     const std::string& cmd) {

    bool has_whitelist = false;
    hashdb::scan_manager_t* whitelist_scan_manager;

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

    // open import manager
    hashdb::import_manager_t import_manager(hashdb_dir, cmd);

    // open whitelist DB
    
    if (has_whitelist) {
      whitelist_scan_manager = new scan_manager_t(whitelist_dir);
    }

    // recursive directory walker
    hasher::dig dig_tool(ingest_path);
    hasher::dig::const_iterator it = dig_tool.begin();

    // iterate
    const size_t buffer_size = 16777216; // 2^24=16MiB
    uint8_t* b = new uint8_t[buffer_size];
    while (it != dig_tool.end()) {
      hasher::file_reader_t file_reader(*it, b, buffer_size);
      
      if (file_reader.is_open) {

        // only process when file size > 0
        if (file_reader.filesize > 0) {
          print_processing(*it, file_reader.filesize);
          ingest_file(file_reader, import_manager, whitelist_scan_manager,
                      p_repository_name);
        } else {
          print_skipping(*it, file_reader.filesize, "empty file");
        }
      } else {
        // this file could not be opened
        print_skipping(*it, file_reader.filesize, file_reader.error_message);
      }
      ++it;
    }

    // done
    if (has_whitelist) {
      delete[] b;
      delete whitelist_scan_manager;
    }

    // success
    return "";
  }

} // end namespace hashdb

