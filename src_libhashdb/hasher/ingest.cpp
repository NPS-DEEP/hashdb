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
 * Support hashdb ingest recursive from path.
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

#include <string>
#include <cassert>
#include <iostream>
#include <unistd.h> // for F_OK
#include <sstream>
#include "num_cpus.hpp"
#include "hashdb.hpp"
#include "dig.h"
#include "file_reader.hpp"
#include "hash_calculator.hpp"
#include "threadpool.hpp"
#include "job.hpp"
#include "job_queue.hpp"
#include "ingest_tracker.hpp"
#include "tprint.hpp"

static const size_t BUFFER_DATA_SIZE = 16777216;   // 2^24=16MiB
static const size_t BUFFER_SIZE = 17825792;        // 2^24+2^20=17MiB
static const size_t MAX_RECURSION_DEPTH = 7;

namespace hashdb {
  // ************************************************************
  // helpers
  // ************************************************************
  static uint64_t calculate_total_bytes(const std::string& ingest_path) {

    // open the recursive directory walker
    hasher::dig dig_tool(ingest_path);
    hasher::dig::const_iterator it = dig_tool.begin();

    // iterate over files
    uint64_t total_bytes = 0;
    while (it != dig_tool.end()) {
      const hasher::file_reader_t file_reader(*it);
      if (file_reader.is_open) {
        total_bytes += file_reader.filesize;
        ++it;
      }
    }
    return total_bytes;
  }

  std::string ingest_file(
        const hasher::file_reader_t& file_reader,
        hashdb::import_manager_t& import_manager,
        hasher::ingest_tracker_t& ingest_tracker,
        const hashdb::scan_manager_t* const whitelist_scan_manager,
        const std::string& repository_name,
        const size_t step_size,
        const size_t block_size,
        const bool process_embedded_data,
        hasher::job_queue_t* const job_queue) {

    // identify the maximum recursion depth
    size_t max_recursion_depth = 
                        (process_embedded_data) ? MAX_RECURSION_DEPTH : 0;

    // create buffer b to read into
    size_t b_size = (file_reader.filesize <= BUFFER_SIZE) ?
                          file_reader.filesize : BUFFER_SIZE;
    uint8_t* b = new (std::nothrow) uint8_t[b_size]();
    if (b == NULL) {
      return "bad memory allocation";
    }

    // read into buffer b
    size_t bytes_read;
    std::string error_message;
    error_message = file_reader.read(0, b, b_size, &bytes_read);
    if (error_message.size() > 0) {
      // abort
      delete[] b;
      return error_message;
    }

    // get a source file hash calculator
    hasher::hash_calculator_t hash_calculator;
    hash_calculator.init();

    // hash first buffer b
    hash_calculator.update(b, b_size, 0, b_size);

    // read and hash subsequent buffers in b2
    if (file_reader.filesize > BUFFER_SIZE) {

      // create b2 to read into
      uint8_t* b2 = new (std::nothrow) uint8_t[BUFFER_SIZE]();
      if (b2 == NULL) {
        // abort
        delete[] b;
        return "bad memory allocation";
      }

      // read and hash b2 into final source file hash value
      for (uint64_t offset = BUFFER_SIZE;
           offset < file_reader.filesize;
           offset += BUFFER_SIZE) {

        // print status
        std::stringstream ss;
        ss << "# Calculating file hash for file " << file_reader.filename
           << " offset " << offset
           << " size " << file_reader.filesize
           << "\n";
        hasher::tprint(ss.str());

        // read into b2
        size_t b2_bytes_read = 0;
        error_message = file_reader.read(
                              offset, b2, BUFFER_SIZE, &b2_bytes_read);
        if (error_message.size() > 0) {
          // abort
          delete[] b2;
          delete[] b;
          return error_message;
        }

        // hash b2 into final source file hash value
        hash_calculator.update(b2, BUFFER_SIZE, 0, b2_bytes_read);
      }

      delete[] b2;
    }

    // get the source file hash
    const std::string file_hash = hash_calculator.final();

    // store the source repository name and filename
    import_manager.insert_source_name(file_hash, repository_name,
                                      file_reader.filename);

    // calculate the number of buffer parts required to process this file

    // do not reprocess file if file_hash is already in ingest_tracker
    if (ingest_tracker.seen_source(file_hash)) {
      ingest_tracker.track_bytes(file_reader.filesize);
      delete[] b;
      return "skipping duplicate file";
    }

    // add source file to ingest_tracker
    const std::string file_type = "";
    const size_t parts_total = (file_reader.filesize + (BUFFER_DATA_SIZE - 1)) /
                               BUFFER_DATA_SIZE;
    ingest_tracker.add_source(file_hash, file_reader.filesize,
                              file_type, parts_total);

    // build buffers from file sections and push them onto the job queue

    // push buffer b onto the job queue
    size_t b_data_size = (b_size > BUFFER_DATA_SIZE)
                         ? BUFFER_DATA_SIZE : b_size;
    job_queue->push(hasher::job_t::new_ingest_job(
                 &import_manager,
                 &ingest_tracker,
                 whitelist_scan_manager,
                 repository_name,
                 step_size,
                 block_size,
                 file_hash,
                 file_reader.filename,
                 0,      // file_offset
                 b,      // buffer
                 b_size, // buffer_size
                 b_data_size, // buffer_data_size,
                 max_recursion_depth,
                 0));    // recursion_count

    // read and push remaining buffers onto the job queue
    for (uint64_t offset = BUFFER_DATA_SIZE;
         offset < file_reader.filesize;
         offset += BUFFER_DATA_SIZE) {

      // print status
      std::stringstream ss;
      ss << "# Processing file " << file_reader.filename
         << " offset " << offset
         << " size " << file_reader.filesize
         << "\n";
      hasher::tprint(ss.str());

      // create b2 to read into
      uint8_t* b2 = new (std::nothrow) uint8_t[BUFFER_SIZE]();
      if (b2 == NULL) {
        // abort
        return "bad memory allocation";
      }

      // read into b2
      size_t b2_bytes_read = 0;
      error_message = file_reader.read(
                                  offset, b2, BUFFER_SIZE, &b2_bytes_read);
      if (error_message.size() > 0) {
        // abort submitting jobs for this file
        delete[] b2;
        return error_message;
      }

      // push this buffer b2 onto the job queue
      size_t b2_data_size = (b2_bytes_read > BUFFER_DATA_SIZE)
                                        ? BUFFER_DATA_SIZE : b2_bytes_read;
      job_queue->push(hasher::job_t::new_ingest_job(
                 &import_manager,
                 &ingest_tracker,
                 whitelist_scan_manager,
                 repository_name,
                 step_size,
                 block_size,
                 file_hash,
                 file_reader.filename,
                 offset,  // file_offset
                 b2,      // buffer
                 b2_bytes_read, // buffer_size
                 b2_data_size,  // buffer_data_size
                 max_recursion_depth,
                 0));  // recursion_count
    }
    return "";
  }

  // ************************************************************
  // ingest
  // ************************************************************
  std::string ingest(const std::string& hashdb_dir,
                     const std::string& ingest_path,
                     const size_t step_size,
                     const std::string& p_repository_name,
                     const std::string& whitelist_dir,
                     const bool process_embedded_data,
                     const std::string& cmd) {

    bool has_whitelist = false;
    hashdb::scan_manager_t* whitelist_scan_manager = NULL;

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

    // calculate the total number of buffers that will be processed
    uint64_t total_bytes = calculate_total_bytes(ingest_path);

    // create the ingest_tracker
    hasher::ingest_tracker_t ingest_tracker(&import_manager, total_bytes);

    // maybe open whitelist DB
    if (has_whitelist) {
      whitelist_scan_manager = new scan_manager_t(whitelist_dir);
    }

    // get the number of CPUs
    const size_t num_cpus = hasher::numCPU();

    // create the job queue to hold more jobs than threads
    hasher::job_queue_t* job_queue = new hasher::job_queue_t(num_cpus * 2);

    // create the threadpool that will process jobs until job_queue.is_done
    hasher::threadpool_t* const threadpool =
                               new hasher::threadpool_t(num_cpus, job_queue);

    // open the recursive directory walker
    hasher::dig dig_tool(ingest_path);
    hasher::dig::const_iterator it = dig_tool.begin();

    // iterate over files
    while (it != dig_tool.end()) {
      const hasher::file_reader_t file_reader(*it);

      if (file_reader.is_open) {

        // only process when file size > 0
        if (file_reader.filesize > 0) {
          std::string success = ingest_file(
                 file_reader, import_manager, ingest_tracker,
                 whitelist_scan_manager,
                 repository_name, step_size, settings.block_size,
                 process_embedded_data, job_queue);
          if (success.size() > 0) {
            std::stringstream ss;
            ss << "# error while importing file " << file_reader.filename
               << ", " << file_reader.error_message << "\n";
            hasher::tprint(ss.str());
          }

        } else {
          std::stringstream ss;
          ss << "# skipping file " << file_reader.filename
             << " size " << file_reader.filesize << "\n";
          hasher::tprint(ss.str());
        }
      } else {
        // this file could not be opened
        std::stringstream ss;
        ss << "# unable to import file " << file_reader.filename
           << ", " << file_reader.error_message << "\n";
        hasher::tprint(ss.str());
      }
      ++it;
    }

    // done
    job_queue->done_adding();
    delete threadpool;
    delete job_queue;
    if (has_whitelist) {
      delete whitelist_scan_manager;
    }

    // success
    return "";
  }

} // end namespace hashdb

