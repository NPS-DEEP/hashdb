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
 * Import from data in JSON format.  Input types are:
 *
 * Block hash data:
 *   {"block_hash":"a7df...", "file_hash":"b9e7...", "file_offset":4096,
 *   "low_entropy_label":"W", "entropy":8, "block_label":"txt"}
 *
 * Source data:
 *   {"file_hash":"b9e7...", "filesize":8000, "file_type":"exe",
 *   "low_entropy_count":4}
 *
 * Source name:
 *   {"file_hash":"b9e7...", "repository_name":"repository1",
 *   "filename":"filename1.dat"}
 *
 * Identify type by field, one of: "block_hash", "filesize", or "filename".
 */

#ifndef TAB_HASHDIGEST_READER_HPP
#define TAB_HASHDIGEST_READER_HPP

#include <zlib.h>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include "hashdb.hpp"
#include "progress_tracker.hpp"
#include "hex_helper.hpp"

class import_json_t {
  private:

  // state
  const std::string& hashdb_dir;
  const std::string& json_file;
  const std::string& cmd;
  size_t line_number;

  // resources
  hashdb::import_manager_t manager;
  std::ifstream in;
  progress_tracker_t progress_tracker;

  // do not allow these
  import_json_t();
  import_json_t(const import_json_t&);
  import_json_t& operator=(const import_json_t&);

  void add_line(const std::string& line, hashdb::import_manager_t& manager,
                progress_tracker_t& progress_tracker) {
    // skip comment lines
    if (line[0] == '#') {
      return;
    }

    // skip empty lines
    if (line.size() == 0) {
      return;
    }

    // find tabs
    size_t tab_index1 = line.find('\t');
    if (tab_index1 == std::string::npos) {
      std::cerr << "Tab not found on line " << line_number << ": '" << line << "'\n";
      return;
    }
    size_t tab_index2 = line.find('\t', tab_index1 + 1);
    if (tab_index2 == std::string::npos) {
      std::cerr << "Second tab not found on line " << line_number << ": '" << line << "'\n";
      return;
    }

    // get file hash
    std::string file_hash_string = line.substr(0, tab_index1);
    std::string file_binary_hash = hex_to_bin(file_hash_string);
    if (file_binary_hash.size() == 0) {
      std::cerr << "file hexdigest is invalid on line " << line_number
                << ": '" << line << "', '" << file_hash_string << "'\n";
      return;
    }

    // get block hash 
    std::string block_hashdigest_string = line.substr(
                                  tab_index1+1, tab_index2 - tab_index1 - 1);
    std::string block_binary_hash = hex_to_bin(block_hashdigest_string);
    if (block_binary_hash == "") {
      std::cerr << "Invalid block hash on line " << line_number
                << ": '" << line << "', '" << block_hashdigest_string << "'\n";
      return;
    }

    // get file offset
    size_t sector_index;
    sector_index = std::atol(line.substr(tab_index2 + 1).c_str());
    if (sector_index == 0) {
      // index starts at 1 so 0 is invalid
      std::cerr << "Invalid sector index on line " << line_number
                << ": '" << line << "', '"
                << line.substr(tab_index2 + 1) << "'\n";
      return;
    }
    uint64_t file_offset = (sector_index - 1) * sector_size;

    // get source ID
    std::pair<bool, uint64_t> pair = manager.insert_source_id(file_binary_hash);
    uint64_t source_id = pair.second;

    if (pair.first == true) {
      // source is new so add name and data for it
      manager.insert_source_name(source_id, repository_name, json_file);
      manager.insert_source_data(source_id, file_binary_hash, 0, "", 0);
    }

    // add block hash
    manager.insert_hash(block_binary_hash, source_id, file_offset, "", 0, "");

    // update progress tracker
    progress_tracker.track();
  }
 
  import_json_t(const std::string& p_hashdb_dir,
                const std::string& p_json_file,
                const std::string& p_repository_name,
                const std::string& p_cmd) :
        hashdb_dir(p_hashdb_dir),
        json_file(p_json_file),
        repository_name(p_repository_name),
        cmd(p_cmd),
        line_number(0),
        manager(hashdb_dir, cmd),
        in(json_file),
        progress_tracker(hashdb_dir, 0) {
  }

  ~import_json_t() {
    in.close();
  }

  void show_bad_line(const std::string& line) {
    std::cerr << "Invalid line " << line_number << ": '" << line << "'\n";
  }

  void read_lines() {
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

      // open the line as a JSON DOM document
      rapidjson::Document document;
      if (document.Parse(line.c_str()).HasParseError() ||
          !document.IsObject()) {
        show_bad_line(line);
        continue;
      }

      // process block hash data
      if (document.HasMember("block_hash") {
        if (!document["block_hash"].IsString() {
          show_bad_line(line);
          continue;
        }
        std::string block_hash = document["block_hash"].GetString();

        if (!document.HasMember("file_hash") ||
                      !document["file_hash"].IsString() {
          show_bad_line(line);
          continue;
        }
        std::string file_hash = document["file_hash"].GetString();

        if (!document.HasMember("file_offset") ||
                      !document["file_offset"].IsNumber() {
          show_bad_line(line);
          continue;
        }
        uint64_t file_offset = document["file_offset"].GetUint64();

        if (!document.HasMember("low_entropy_label") ||
                      !document["low_entropy_label"].IsString() {
          show_bad_line(line);
          continue;
        }
        std::string low_entropy_label =
                             document["low_entropy_label"].GetString();

        if (!document.HasMember("entropy") ||
                      !document["entropy"].IsNumber() {
          show_bad_line(line);
          continue;
        }
        uint64_t entropy = document["entropy"].GetUint64();

        if (!document.HasMember("block_label") ||
                      !document["block_label"].IsString() {
          show_bad_line(line);
          continue;
        }
        std::string block_label = document["block_label"].GetString();

        std::pair<bool, uint64_t> id_pair = manager.insert_source_id(
                                                    hex_to_bin(file_hash))

        manager.insert_hash(hex_to_bin(binary_hash), source_id, file_offset,
                            low_entropy_label, entropy, block_label);

      // process source data
      } else if (document.HasMember("filesize") {
        if (!document["fileszie"].IsNumber() {
          show_bad_line(line);
          continue;
        }
//zzzzzzzzzzzzzzzz

      add_line(line);
    }
  }

/*
 * Block hash data:
 *   {"block_hash":"a7df...", "file_hash":"b9e7...", "file_offset":4096,
 *   "low_entropy_label":"W", "entropy":8, "block_label":"txt"}
 *
 * Source data:
 *   {"file_hash":"b9e7...", "filesize":8000, "file_type":"exe",
 *   "low_entropy_count":4}
 *
 * Source name:
 *   {"file_hash":"b9e7...", "repository_name":"repository1",
 *   "filename":"filename1.dat"}
 *
 * Identify type by field, one of: "block_hash", "filesize", or "filename".
*/


  public:

  // read JSON file
  std::pair<bool, std::string> read() {

    // validate hashdb_dir path
    std::pair<bool, std::string> pair;
    pair = hashdb::is_valid_hashdb(hashdb_dir);
    if (pair.first == false) {
      return pair;
    }

    // validate ability to open the JSON file
    std::ifstream p_in(p_json_file.c_str());
    if (!p_in.is_open()) {
      std::stringstream ss;
      ss << "Cannot open " << p_json_file << ": " << strerror(errno);
      return std::pair<bool, std::string>(false, ss.str());
    }
    p_in.close();

    // create the reader
    import_tab_t reader(p_hashdb_dir, p_json_file, p_repository_name, p_cmd);

    // read the lines
    reader.read_lines();

    // done
    return std::pair<bool, std::string>(true, "");
  }
};

#endif

