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
 * Scan for hashes in file where lines are "<forensic path><tab><hex hash>".
 * Comment lines are forwarded to output.
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
#include "../src_libhashdb/hashdb.hpp"

void scan_list(hashdb::scan_manager_t& manager, std::istream& in,
               const hashdb::scan_mode_t scan_mode) {

  size_t line_number = 0;
  std::string line;
  while(getline(in, line)) {
    ++line_number;

    // print comment lines
    if (line[0] == '#') {
      // forward to stdout
      std::cout << line << "\n";
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

    // get forensic path
    std::string forensic_path = line.substr(0, tab_index1);

    // get block hash
    std::string block_hashdigest_string = line.substr(tab_index1+1);
    std::string block_binary_hash = hashdb::hex_to_bin(block_hashdigest_string);
    if (block_binary_hash == "") {
      std::cerr << "Invalid block hash on line " << line_number
                << ": '" << line << "'\n";
      continue;
    }

    // scan
    const std::string expanded_text = manager.find_hash_json(
                                              scan_mode, block_binary_hash);
    if (expanded_text.size() != 0) {
      std::cout << forensic_path << "\t"
                << block_hashdigest_string << "\t"
                << expanded_text << std::endl;
    }
  }
}

