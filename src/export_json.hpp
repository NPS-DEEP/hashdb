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

  // return rapidjson::Value type from a std::string
  rapidjson::Value v(const std::string& s,
                     rapidjson::Document::AllocatorType& allocator) {
    rapidjson::Value v;
    v.SetString(s.c_str(), s.size(), allocator);
    return v;
  }

  // Source data:
  //   {"file_hash":"b9e7...", "filesize":8000, "file_type":"exe",
  //   "nonprobative_count":4, "name_pairs":["repository1","filename1",
  //   "repo2","f2"]
  void write_source_data(std::ostream& os) {

    // source fields
    uint64_t filesize;
    std::string file_type;
    uint64_t nonprobative_count;
    hashdb::source_names_t* source_names = new hashdb::source_names_t;

    std::pair<bool, std::string> pair = manager.source_begin();
    while (pair.first != false) {

      // get source data
      manager.find_source_data(pair.second, filesize, file_type,
                               nonprobative_count);

      // prepare JSON
      rapidjson::Document json_doc;
      rapidjson::Document::AllocatorType& allocator = json_doc.GetAllocator();
      json_doc.SetObject();

      // set source data
      std::string file_hash = bin_to_hex(pair.second);
      json_doc.AddMember("file_hash", v(file_hash, allocator), allocator);
      json_doc.AddMember("filesize", filesize, allocator);
      json_doc.AddMember("file_type", v(file_type, allocator), allocator);
      json_doc.AddMember("nonprobative_count", nonprobative_count, allocator);

      // get source names
      manager.find_source_names(pair.second, *source_names);

      // name_pairs object
      rapidjson::Value json_name_pairs(rapidjson::kArrayType);

      // provide names
      hashdb::source_names_t::const_iterator it;
      for (it = source_names->begin(); it != source_names->end(); ++it) {
        // repository name
        json_name_pairs.PushBack(v(it->first, allocator), allocator);
        // filename
        json_name_pairs.PushBack(v(it->second, allocator), allocator);
      }
      json_doc.AddMember("name_pairs", json_name_pairs, allocator);

      // write JSON text
      rapidjson::StringBuffer strbuf;
      rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
      json_doc.Accept(writer);
      os << strbuf.GetString() << "\n";

      // next
      pair = manager.source_next(pair.second);
    }

    delete source_names;
  }

  // Block hash data:
  //   {"block_hash":"a7df...", "entropy":8, "block_label":"W",
  //   "source_offset_pairs":["b9e7...", 4096]}
  void write_block_hash_data(const std::string& cmd, std::ostream& os) {

    progress_tracker_t progress_tracker(hashdb_dir, manager.size(), cmd);

    // hash fields
    uint64_t entropy;
    std::string block_label;
    hashdb::source_offset_pairs_t* source_offset_pairs =
                                          new hashdb::source_offset_pairs_t;

    std::pair<bool, std::string> pair = manager.hash_begin();
    while (pair.first != false) {

      // get hash data
      manager.find_hash(pair.second, entropy, block_label,
                        *source_offset_pairs);

      // prepare JSON
      rapidjson::Document json_doc;
      rapidjson::Document::AllocatorType& allocator = json_doc.GetAllocator();
      json_doc.SetObject();

      // set hash data
      std::string block_hash = bin_to_hex(pair.second);
      json_doc.AddMember("block_hash", v(block_hash, allocator), allocator);
      json_doc.AddMember("entropy", entropy, allocator);
      json_doc.AddMember("block_label", v(block_label, allocator), allocator);

      // source_offset_pairs
      rapidjson::Value json_pairs(rapidjson::kArrayType);

      for (hashdb::source_offset_pairs_t::const_iterator it =
           source_offset_pairs->begin();
           it != source_offset_pairs->end(); ++it) {

        // file hash
        json_pairs.PushBack(v(bin_to_hex(it->first), allocator), allocator);
        // file offset
        json_pairs.PushBack(it->second, allocator);
      }
      json_doc.AddMember("source_offset_pairs", json_pairs, allocator);

      // write JSON text
      rapidjson::StringBuffer strbuf;
      rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
      json_doc.Accept(writer);
      os << strbuf.GetString() << "\n";

      // next
      progress_tracker.track_hash_data(*source_offset_pairs);
      pair = manager.hash_next(pair.second);
    }

    delete source_offset_pairs;
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

