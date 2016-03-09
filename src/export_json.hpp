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
 * Export data in JSON format.  Lines are one of:
 *   source data, block hash data, or comment.
 *
 * Source data:
 *   {"file_hash":"b9e7...", "filesize":8000, "file_type":"exe",
 *   "nonprobative_count":4, "name_pairs":["repository1","filename1",
 *   "repo2","f2"]
 *
 * Block hash data:
 *   {"block_hash":"a7df...", "entropy":8, "block_label":"W",
 *   "source_offset_pairs":["b9e7...", 4096]}
 *
 * Comment line:
 *   Comment lines start with #.
 */

#ifndef EXPORT_JSON_HPP
#define EXPORT_JSON_HPP

#include <zlib.h>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include "../src_libhashdb/hashdb.hpp"
#include "progress_tracker.hpp"
#include <string.h> // for strerror
#include <fstream>
#include "rapidjson.h"
#include "writer.h"
#include "document.h"

class export_json_t {
  private:

  // state
  const std::string& hashdb_dir;

  // resources
  hashdb::scan_manager_t manager;

  // do not allow these
  export_json_t();
  export_json_t(const export_json_t&);
  export_json_t& operator=(const export_json_t&);

  // private, used by write()
  export_json_t(const std::string& p_hashdb_dir) :
        hashdb_dir(p_hashdb_dir),
        manager(hashdb_dir) {
  }

  // return rapidjson::Value type from a std::string
  rapidjson::Value v(const std::string& s,
                     rapidjson::Document::AllocatorType& allocator) {
    rapidjson::Value value;
    value.SetString(s.c_str(), s.size(), allocator);
    return value;
  }

  // Source data:
  //   {"file_hash":"b9e7...", "filesize":8000, "file_type":"exe",
  //   "nonprobative_count":4, "name_pairs":["repository1","filename1",
  //   "repo2","f2"]
  void write_source_data(std::ostream& os) {

    std::string file_binary_hash;
    bool has_source = manager.source_begin(file_binary_hash);
    while (has_source == true) {

      // get source data
      std::string json_source_string = manager.find_source_json(
                                                       file_binary_hash);

      // program error
      if (json_source_string.size() == 0) {
        assert(0);
      }

      os << json_source_string << "\n";

      // next
      has_source = manager.source_next(file_binary_hash, file_binary_hash);
    }
  }

  // Block hash data:
  //   {"block_hash":"a7df...", "entropy":8, "block_label":"W",
  //   "source_offset_pairs":["b9e7...", 4096]}
  void write_block_hash_data(const std::string& cmd, std::ostream& os) {

    progress_tracker_t progress_tracker(hashdb_dir, manager.size_hashes(), cmd);

    std::string binary_hash;
    bool has_hash = manager.hash_begin(binary_hash);
    while (has_hash) {

      // get hash data
      std::string json_hash_string = manager.find_hash_json(binary_hash);

      // program error
      if (json_hash_string.size() == 0) {
        assert(0);
      }

      os << json_hash_string << "\n";

      // next
      progress_tracker.track_hash_data(manager.find_hash_count(binary_hash));
      has_hash = manager.hash_next(binary_hash, binary_hash);
    }
  }

  public:

  // write JSON to stream
  static void write(const std::string& p_hashdb_dir,
                    const std::string& cmd,
                    std::ostream& os) {

    // create the JSON formatter
    export_json_t writer(p_hashdb_dir);

    // write cmd
    os << "# command: '" << cmd << "'\n"
       << "# hashdb-Version: " << PACKAGE_VERSION << "\n";

    // write source data
    writer.write_source_data(os);

    // write block hash data
    writer.write_block_hash_data(cmd, os);
  }

  // just print sources to stdout
  static void print_sources(const std::string& p_hashdb_dir) {

    // create the JSON formatter
    export_json_t export_json(p_hashdb_dir);

    // write source data
    export_json.write_source_data(std::cout);
  }

};

#endif

