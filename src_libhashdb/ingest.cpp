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
#include "hasher_buffer.hpp"
#include "calculate_hash.hpp"
#include "calculate_entropy.hpp"
#include "calculate_block_label.hpp"

namespace hashdb {

  // ************************************************************
  // support interfaces
  // ************************************************************

  // calculate source file hash from file in file reader
  static std::string calculate_file_hash(
                                  const hasher::file_reader_t& file_reader,
                                  uint8_t* const buffer,
                                  const size_t buffer_size) {
    size_t bytes_read;
    hasher::calculate_hash_t calculate_hash;
    calculate_hash.init();
    for (uint64_t offset = 0; offset < file_reader.filesize;
         offset += buffer_size) {

      // offer feedback for large files
      if (file_reader.filesize > buffer_size) {
        std::cout << "Calculating file hash for " << file_reader.filename
                  << " offset " << offset
                  << " size " << file_reader.filesize
                  << "\n";
      }

      file_reader.read(offset, buffer, buffer_size, &bytes_read);
      calculate_hash.update(buffer, buffer_size, 0, bytes_read);
    }
    return calculate_hash.final();
  }

  static void ingest_buffer(import_manager_t& import_manager,
                            scan_manager_t* whitelist_scan_manager,
                            const std::string& repository_name,
                            const size_t step_size,
                            const size_t block_size,
                            const hasher::hasher_buffer_t& hasher_buffer,
                            size_t* nonprobative_count) {

    // get hash calculator object
    hasher::calculate_hash_t calculate_hash;

    // iterate over buffer to add block hashes and metadata
    for (size_t i=0; i < hasher_buffer.end_byte; i+= step_size) {

      // block hash
      std::string block_hash = calculate_hash.calculate(hasher_buffer.buffer,
                  hasher_buffer.end_byte, i, block_size);

      // entropy
      size_t entropy = hasher::calculate_entropy(hasher_buffer.buffer,
                  hasher_buffer.end_byte, i, block_size);

      // block label
      std::string block_label = hasher::calculate_block_label(
                  hasher_buffer.buffer, hasher_buffer.end_byte, i, block_size);
      if (block_label.size() != 0) {
        ++nonprobative_count;
      }

      // add block hash to DB
      import_manager.insert_hash(block_hash, hasher_buffer.source_hash,
                  hasher_buffer.offset+i, entropy, block_label);
    }
  }


  // ingest file in file reader
  static void ingest_file(const hasher::file_reader_t& file_reader,
                          import_manager_t& import_manager,
                          scan_manager_t* whitelist_scan_manager,
                          const std::string& repository_name,
                          const size_t step_size,
                          const size_t block_size,
                          // zz later, move buffer to threadpool.
                          uint8_t* const buffer,
                          const size_t buffer_size) {

    // calculate source file hash
    std::string source_file_hash = calculate_file_hash(
                                         file_reader, buffer, buffer_size);

    // add source file to DB
    import_manager.insert_source_name(
                      source_file_hash, repository_name, file_reader.filename);

    // allocate a hash calculator for block hashes
    hasher::calculate_hash_t calculate_hash;

    // iterate over file
    size_t nonprobative_count = 0;
    static const size_t page_size = 16777216; // 2^24=16MiB
    for (uint64_t offset = 0; offset < file_reader.filesize;
         offset += page_size) {

      // print status
      std::cout << "Ingesting file " << file_reader.filename
                << " offset " << offset
                << " size " << file_reader.filesize
                << "\n";

      // read into buffer
      size_t bytes_read;
      file_reader.read(offset, buffer, buffer_size, &bytes_read);
      const size_t end_byte = (bytes_read < page_size) ? bytes_read : page_size;

      // create hasher buffer
      hasher::hasher_buffer_t hasher_buffer(source_file_hash,
                                            file_reader.filename,
                                            offset,
                                            buffer,
                                            buffer_size,
                                            end_byte,
                                            true,
                                            0);

      ingest_buffer(import_manager,
                    whitelist_scan_manager,
                    repository_name,
                    step_size,
                    block_size,
                    hasher_buffer,
                    &nonprobative_count);
    }

    // add source file metadata
    import_manager.insert_source_data(source_file_hash,
                              file_reader.filesize, "", nonprobative_count);
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

    // make sure step size is compatible with byte alignment
    if (step_size % settings.byte_alignment != 0) {
      std::stringstream ss;
      ss << "Invalid byte alignment: step size " << step_size
         << " does not align with byte alignment " << settings.byte_alignment;
      return ss.str();
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

    // maybe open whitelist DB
    if (has_whitelist) {
      whitelist_scan_manager = new scan_manager_t(whitelist_dir);
    }

    // recursive directory walker
    hasher::dig dig_tool(ingest_path);
    hasher::dig::const_iterator it = dig_tool.begin();

    // zz until multithreading, everything uses this buffer
    const size_t buffer_size = 16777216+1048576; // 2^24+2^20=16MiB+1MiB
    uint8_t* buffer = new uint8_t[buffer_size];

    // iterate over files
    while (it != dig_tool.end()) {
      hasher::file_reader_t file_reader(*it);

      if (file_reader.is_open) {

        // only process when file size > 0
        if (file_reader.filesize > 0) {
          ingest_file(file_reader, import_manager, whitelist_scan_manager,
                      p_repository_name, step_size, settings.block_size,
                      buffer, buffer_size);
        } else {
          std::cout << "skipping file " << file_reader.filename
                    << " size " << file_reader.filesize << "\n";
        }
      } else {
        // this file could not be opened
        std::cout << "unable to import file " << file_reader.filename
                  << ", " << file_reader.error_message << "\n";
      }
      ++it;
    }

    // done
    if (has_whitelist) {
      delete[] buffer;
      delete whitelist_scan_manager;
    }

    // success
    return "";
  }

} // end namespace hashdb

