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
 * Export data in JSON format.  Lines are one of:
 *   source data, block hash data, or comment.
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
#include <cassert>
#include "../src_libhashdb/hashdb.hpp"
#include "progress_tracker.hpp"

void export_json_sources(const hashdb::scan_manager_t& manager,
                         std::ostream& os) {

  std::string file_binary_hash = manager.first_source();
  while (file_binary_hash.size() != 0) {

    // get source data
    std::string json_source_string = manager.export_source_json(
                                                       file_binary_hash);

    // program error
    if (json_source_string.size() == 0) {
      assert(0);
    }

    os << json_source_string << "\n";

    // next
    file_binary_hash = manager.next_source(file_binary_hash);
  }
}

void export_json_hashes(const hashdb::scan_manager_t& manager,
                        progress_tracker_t& progress_tracker,
                        std::ostream& os) {

  std::string binary_hash = manager.first_hash();
  while (binary_hash.size() != 0) {

    // get hash data
    std::string json_hash_string = manager.export_hash_json(binary_hash);

    // program error
    if (json_hash_string.size() == 0) {
      assert(0);
    }

    os << json_hash_string << "\n";

    // next
    progress_tracker.track_hash_data(manager.find_hash_count(binary_hash));
    binary_hash = manager.next_hash(binary_hash);
  }
}

