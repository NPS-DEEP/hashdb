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

// Code adapted from bulk_extractor recursion scanners.

// NOTE: private methods do not check for buffer overflow.  Do not call
//       them when near the end of data.

/**
 * \file
 * Process ingest or hash job from a buffer.  Can recurse up to a depth.
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

#include <cstring>
#include <sstream>
#include <cstdlib>
#include <stdint.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <unistd.h>
#include "hashdb.hpp"
#include "job.hpp"
#include "uncompress.hpp"
#include "process_job.hpp"
#include "hash_calculator.hpp"
#include "entropy_calculator.hpp"
#include "calculate_block_label.hpp"

namespace hasher {

  static std::string make_recursed_filename(
                const std::string& parent_filename,
                const uint64_t parent_file_offset,
                const std::string& compression_name) {
    std::stringstream ss;
    ss << parent_filename<< "-"
       << parent_file_offset << "-"
       << compression_name;
    return ss.str();
  }

  // prepare and run a recursed job
  static void recurse(const hasher::job_t& parent_job,
               const size_t relative_offset,
               const std::string& compression_name,
               const uint8_t* const uncompressed_buffer,
               const size_t uncompressed_size) {

    // make sure there is something to do
    if (uncompressed_size == 0) {
      // no data so free the buffer and be done
      delete[] uncompressed_buffer;
      return;
    }

    // impose max recursion depth
    if (parent_job.recursion_depth >= 7) {
      // too much recursive depth
      delete[] uncompressed_buffer;
      return;
    }

    // process recursed job based on job type
    switch(parent_job.job_type) {
      // this is similar to ingest.cpp
      case hasher::job_type_t::INGEST: {

        // calculate the recursed file hash
        hash_calculator_t hash_calculator;
        const std::string recursed_file_hash = hash_calculator.calculate(
                uncompressed_buffer, uncompressed_size, 0, uncompressed_size);

        // calculate the parent file offset
        const uint64_t parent_file_offset = (parent_job.recursion_depth == 0)
               ? parent_job.file_offset + relative_offset : relative_offset;

        // calculate the recursed filename
        const std::string recursed_filename = make_recursed_filename(
                   parent_job.filename, parent_file_offset, compression_name);

        // store the source repository name and filename
        parent_job.import_manager->insert_source_name(recursed_file_hash,
                   parent_job.repository_name, recursed_filename);

        // define the file type, currently not defined
        const std::string file_type = "";

        // add uncompressed recursed source file to ingest_tracker
        const bool source_added = parent_job.ingest_tracker->add_source(
                                              recursed_file_hash,
                                              uncompressed_size,
                                              file_type,
                                              1);  // parts_total

        // do not re-ingest hashes from duplicate sources
        const bool ingest_hashes = (source_added == false);

        // create a new recursed ingest job
        job_t* recursed_ingest_job = job_t::new_ingest_job(
                   parent_job.import_manager,
                   parent_job.ingest_tracker,
                   parent_job.whitelist_scan_manager,
                   parent_job.repository_name,
                   parent_job.step_size,
                   parent_job.block_size,
                   recursed_file_hash,
                   recursed_filename,
                   uncompressed_size, // file size is buffer_size
                   0,                 // file_offset
                   parent_job.disable_recursive_processing,
                   parent_job.disable_calculate_entropy,
                   parent_job.disable_calculate_labels,
                   ingest_hashes,
                   uncompressed_buffer,
                   uncompressed_size, // buffer_size
                   uncompressed_size, // buffer_data_size
                   parent_job.max_recursion_depth,
                   parent_job.recursion_depth + 1);

        // run the new recursed ingest job
        process_job(*recursed_ingest_job);
        break;
      }

      // this is similar to scan_media.cpp
      case hasher::job_type_t::SCAN: {

        // calculate the parent file offset
        const uint64_t parent_file_offset = (parent_job.recursion_depth == 0)
               ? parent_job.file_offset + relative_offset : relative_offset;

        // calculate the recursed filename
        const std::string recursed_filename = make_recursed_filename(
                  parent_job.filename, parent_file_offset, compression_name);

        job_t* recursed_scan_media_job = job_t::new_scan_job(
                   parent_job.scan_manager,
                   parent_job.scan_tracker,
                   parent_job.step_size,
                   parent_job.block_size,
                   recursed_filename,
                   uncompressed_size, // file size is buffer_size
                   0,                 // file_offset
                   parent_job.disable_recursive_processing,
                   parent_job.scan_mode,
                   uncompressed_buffer,
                   uncompressed_size, // buffer_size
                   uncompressed_size, // buffer_data_size
                   parent_job.max_recursion_depth,
                   parent_job.recursion_depth + 1);

        // run the new recursed scan media job
        process_job(*recursed_scan_media_job);
        break;
      }
    }
  }

  void process_recursive(const hasher::job_t& job) {

    // impose max recursion depth
    if (job.recursion_depth >= 7) {
      // too much recursive depth
      return;
    }

    // scan each byte for a compression signature, stop before end
    for (size_t i=0; i < job.buffer_data_size; ++i) {

      if (zip_signature(job.buffer, job.buffer_size, i)) {

        // inflate and recurse
        uint8_t* out_buf;
        size_t out_size;
        std::string error_message = new_from_zip(
                                           job.buffer, job.buffer_size, i,
                                           &out_buf, &out_size);
        if (error_message == "") {
          recurse(job, i, "zip", out_buf, out_size);
        }

      } else if (gzip_signature(job.buffer, job.buffer_size, i)) {

        // inflate and recurse
        uint8_t* out_buf;
        size_t out_size;
        std::string error_message = new_from_gzip(
                                           job.buffer, job.buffer_size, i,
                                           &out_buf, &out_size);
        if (error_message == "") {
          recurse(job, i, "gzip", out_buf, out_size);
        }
      }
    }
  }
} // end namespace hasher

