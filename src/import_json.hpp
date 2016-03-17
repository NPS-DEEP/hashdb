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
 * Import from data in JSON format.  Lines are one of:
 *   source data, block hash data, or comment.
 *
 * Source data:
 *   {"file_hash":"b9e7...", "filesize":8000, "file_type":"exe",
 *   "nonprobative_count":4, "name_pairs":["repository1","filename1",
 *   "repo2","f2"]
 * Block hash data:
 *   {"block_hash":"a7df...", "entropy":8, "block_label":"W",
 *   "source_offset_pairs":["b9e7...", 4096]}
 *
 * Comment line:
 *   Comment lines start with #.
 */

#ifndef IMPORT_JSON_HPP
#define IMPORT_JSON_HPP

#include <zlib.h>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include "../src_libhashdb/hashdb.hpp"
#include "progress_tracker.hpp"
#include <string.h> // for strerror
#include <fstream>

class import_json_t {
  private:

  // state
  const std::string& hashdb_dir;
  size_t line_number;

  // resources
  hashdb::import_manager_t manager;
  progress_tracker_t progress_tracker;

  // do not allow these
  import_json_t();
  import_json_t(const import_json_t&);
  import_json_t& operator=(const import_json_t&);

  // private, used by read()
  import_json_t(const std::string& p_hashdb_dir,
                const std::string& cmd) :
        hashdb_dir(p_hashdb_dir),
        line_number(0),
        manager(hashdb_dir, cmd),
        progress_tracker(hashdb_dir, 0, cmd) {
  }

  void report_invalid_line(const std::string& field,
                           const std::string& line) const {
    std::cerr << "Invalid line " << line_number
              << " field: " << field
              << ": '" << line << "'\n";
  }

  void read_lines(std::istream& in) {
    std::string line;
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
      std::string error_message = manager.insert_json(line);
      if (error_message.size() != 0) {
        report_invalid_line(error_message, line);
      }
    }
  }

  public:

  // read JSON file
  static void read(const std::string& hashdb_dir,
                   const std::string& cmd,
                   std::istream& in) {

    // create the reader
    import_json_t reader(hashdb_dir, cmd);

    // read the lines
    reader.read_lines(in);
  }
};

#endif

