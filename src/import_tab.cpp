// Author:  Bruce Allen
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
 * Provides the service of importing hash data from a file formatted
 * using tab delimited fields, specifically:
 * <file hash>\t<block hash>\t<block offset>\n
 *
 * Note that block offset is not used.  All block hashes for new file
 * hashes are imported.  All block hashes for pre-existing file hashes
 * are ignored.
 */

#include <config.h>
// this process of getting WIN32 defined was inspired
// from i686-w64-mingw32/sys-root/mingw/include/windows.h.
// All this to include winsock2.h before windows.h to avoid a warning.
#if defined(__MINGW64__) && defined(__cplusplus)
#  ifndef WIN32
#    define WIN32
#  endif
#endif
#ifdef WIN32
  // including winsock2.h now keeps an included header somewhere from
  // including windows.h first, resulting in a warning.
  #include <winsock2.h>
#endif

#include <iostream>
#include <sstream>
#include "../src_libhashdb/hashdb.hpp"
#include "s_to_uint64.hpp"
#include "progress_tracker.hpp"

void import_tab(hashdb::import_manager_t& manager,
                const std::string& repository_name,
                const std::string& filename,
                const hashdb::scan_manager_t* const whitelist_manager,
                progress_tracker_t& progress_tracker,
                std::istream& in) {

  // only import file hashes that are new to the session
  std::set<std::string> importable_sources;

  std::string line;
  size_t line_number = 0;
  while(getline(in, line)) {
    ++line_number;

    // skip comment lines
    if (line[0] == '#') {
      continue;
    }

    // skip empty lines
    if (line.size() == 0) {
      continue;
    }

    // find tabs
    size_t tab_index1 = line.find('\t');
    if (tab_index1 == std::string::npos) {
      std::cerr << "Tab not found on line " << line_number << ": '" << line << "'\n";
      continue;
    }
    size_t tab_index2 = line.find('\t', tab_index1 + 1);
    if (tab_index2 == std::string::npos) {
      std::cerr << "Second tab not found on line " << line_number << ": '" << line << "'\n";
      continue;
    }

    // get file hash
    std::string file_hash_string = line.substr(0, tab_index1);
    std::string file_binary_hash = hashdb::hex_to_bin(file_hash_string);
    if (file_binary_hash.size() == 0) {
      std::cerr << "file hexdigest is invalid on line " << line_number
                << ": '" << line << "', '" << file_hash_string << "'\n";
      continue;
    }

    // skip the file hash if it was preexisting else identify it as importable
    if (importable_sources.find(file_binary_hash) == importable_sources.end()) {
      // the file hash has not been seen yet so see if it is preexisting
      if (manager.has_source(file_binary_hash)) {
        // the file is preexisting so skip it
        continue;
      } else {
        // the file hash is new so identify it as importable
        importable_sources.insert(file_binary_hash);
      }
    }

    // skip the file hash if it has not been identified as importable
    if (importable_sources.find(file_binary_hash) == importable_sources.end()) {
      continue;
    }

    // get block hash
    std::string block_hashdigest_string = line.substr(
                                  tab_index1+1, tab_index2 - tab_index1 - 1);
    std::string block_binary_hash = hashdb::hex_to_bin(block_hashdigest_string);
    if (block_binary_hash == "") {
      std::cerr << "Invalid block hash on line " << line_number
                << ": '" << line << "', '" << block_hashdigest_string << "'\n";
      continue;
    }

    // skip the file offset

    // mark with "w" if in whitelist
    std::string whitelist_flag = "";
    if (whitelist_manager != NULL) {
      if (whitelist_manager->find_hash_count(block_binary_hash) > 0) {
        whitelist_flag = "w";
      }
    }

    // add source data
    manager.insert_source_data(file_binary_hash, 0, "", 0, 0);

    // add name pair
    manager.insert_source_name(file_binary_hash, repository_name, filename);

    // add block hash
    manager.insert_hash(block_binary_hash, 0.0, whitelist_flag,
                        file_binary_hash);

    // update progress tracker
    progress_tracker.track();
  }
}

