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
 * Import from data in JSON format.  Lines are one of source data,
 * block hash data, or comment.
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

#include <sstream>
#include "../src_libhashdb/hashdb.hpp"
#include "progress_tracker.hpp"
#include <fstream>

void import_json(hashdb::import_manager_t& manager,
                 progress_tracker_t& progress_tracker,
                 std::istream& in) {
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

    // import JSON
    std::string error_message = manager.import_json(line);
    if (error_message.size() != 0) {
      std::cerr << "Invalid line " << line_number
                << " error: " << error_message
                << ": '" << line << "'\n";
    } else {
      progress_tracker.track();
    }
  }
}

