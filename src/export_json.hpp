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
 *   "low_entropy_count":4, "names":[{"repository_name":"repository1",
 *   "filename":"filename1"}]}
 *
 * Block hash data:
 *   {"block_hash":"a7df...", "low_entropy_label":"W", "entropy":8,
 *   "block_label":"txt", "source_offset_pairs":["b9e7...", 4096]}
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
#include "hashdb.hpp"
#include "progress_tracker.hpp"
#include "hex_helper.hpp"
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

  // Source data:
  //   {"file_hash":"b9e7...", "filesize":8000, "file_type":"exe",
  //   "low_entropy_count":4, "names":[{"repository_name":"repository1",
  //   "filename":"filename1"}]}
  void write_source_data(std::ostream& os) {

    // source fields
    std::string file_binary_hash;
    uint64_t filesize;
    std::string file_type;
    uint64_t low_entropy_count;
    hashdb::source_names_t* source_names = new hashdb::source_names_t;

    std::pair<bool, uint64_t> pair = manager.source_begin();
    while (pair.first != false) {
      // source data
      manager.find_source_data(pair.second, file_binary_hash, filesize,
                               file_type, low_entropy_count);
      std::string file_hash = bin_to_hex(file_binary_hash);
      os << "{\"file_hash\":\"" << file_hash
         << "\",\"filesize\":" << filesize
         << ",\"file_type\":\"" << file_type
         << "\",\"low_entropy_count\":" << low_entropy_count
         ;

      // source names
      manager.find_source_names(pair.second, *source_names);
      hashdb::source_names_t::const_iterator it;
      int i=0;
      os << ",\"names\":[";
      for (it = source_names->begin(); it != source_names->end(); ++it, ++i) {
        if (i != 0) {
          // no comma before first pair
          os << ",";
        }
        os << "{\"repository_name\":\"" << it->first
           << "\",\"filename\":\"" << it->second
           << "\"}";
      }
      os << "]}\n";

      // next
      pair = manager.source_next(pair.second);
    }

    delete source_names;
  }

  // Block hash data:
  //   {"block_hash":"a7df...", "low_entropy_label":"W", "entropy":8,
  //   "block_label":"txt", "source_offset_pairs":["b9e7...", 4096]}
  void write_block_hash_data(const std::string& cmd, std::ostream& os) {

    progress_tracker_t progress_tracker(hashdb_dir, manager.size(), cmd);

    // hash fields
    std::string low_entropy_label;
    uint64_t entropy;
    std::string block_label;
    hashdb::id_offset_pairs_t* id_offset_pairs = new hashdb::id_offset_pairs_t;

    // source fields just for file_binary_hash
    std::string file_binary_hash;
    uint64_t filesize;
    std::string file_type;
    uint64_t low_entropy_count;

    std::pair<bool, std::string> pair = manager.hash_begin();

    while (pair.first != false) {

      // hash data
      manager.find_hash(pair.second, low_entropy_label, entropy, block_label,
                        *id_offset_pairs);

      os << "{\"block_hash\":\"" << bin_to_hex(pair.second)
         << "\",\"low_entropy_label\":\"" << low_entropy_label
         << "\",\"entropy\":" << entropy
         << ",\"block_label\":\"" << block_label
         << "\",\"source_offset_pairs\":[";

      int i=0;
      hashdb::id_offset_pairs_t::const_iterator it;
      for (it = id_offset_pairs->begin(); it != id_offset_pairs->end();
                                                            ++it, ++i) {
        if (i != 0) {
          // no comma before first pair
          os << ",";
        }

        // get file hash
        manager.find_source_data(it->first, file_binary_hash, filesize,
                                 file_type, low_entropy_count);

        // source, offset pair
        os << "\"" << bin_to_hex(file_binary_hash)
           << "\"," << it->second;
      }
      os << "]}\n";

      // next
      progress_tracker.track();
      pair = manager.hash_next(pair.second);
    }

    delete id_offset_pairs;
  }

  public:

  // write JSON to stream
  static void write(const std::string& p_hashdb_dir,
                    const std::string& cmd,
                    std::ostream& os) {

    // create the writer
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

    // create the printer
    export_json_t export_json(p_hashdb_dir);

    // write source data
    export_json.write_source_data(std::cout);
  }

};

#endif

