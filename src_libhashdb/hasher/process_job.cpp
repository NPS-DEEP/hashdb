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
#include "tprint.hpp"
#include "process_job.hpp"
#include "process_recursive.hpp"
#include "hash_calculator.hpp"
#include "entropy_calculator.hpp"
#include "calculate_block_label.hpp"

namespace hasher {

  // detect if block is all zero
  inline bool all_zero(const uint8_t* const buffer, const size_t buffer_size,
                       const size_t offset, const size_t p_count) {

    // number of bytes
    const size_t count =
          (offset + p_count <= buffer_size) ? p_count : buffer_size - offset;
    for (size_t i=offset+1; i < offset + count; ++i) {
      if (buffer[i] != 0) {
        return false;
      }
    }
    return true;
  }

  // process INGEST job
  static void process_ingest_job(const hasher::job_t& job) {

    if (!job.disable_ingest_hashes) {
      // get hash calculator object
      hasher::hash_calculator_t hash_calculator;

      // get entropy calculator object
      hasher::entropy_calculator_t entropy_calculator(job.block_size);

      // iterate over buffer to add block hashes and metadata
      size_t zero_count = 0;
      size_t nonprobative_count = 0;
      for (size_t i=0; i < job.buffer_data_size; i+= job.step_size) {

        // skip if all the bytes are the same
        if (all_zero(job.buffer, job.buffer_size, i, job.block_size)) {
          ++zero_count;
          continue;
        }

        // calculate block hash
        const std::string block_hash = hash_calculator.calculate(job.buffer,
                                    job.buffer_size, i, job.block_size);

        // calculate entropy
        float entropy = 0;
        if (!job.disable_calculate_entropy) {
          entropy = entropy_calculator.calculate(job.buffer,
                                    job.buffer_size, i);
        }

        // calculate block label
        std::string block_label = "";
        if (!job.disable_calculate_labels) {
          block_label = hasher::calculate_block_label(job.buffer,
                                   job.buffer_size, i, job.block_size);
          if (block_label.size() != 0) {
            ++nonprobative_count;
          }
        }

        // add block hash to DB
        job.import_manager->insert_hash(block_hash, job.file_hash,
                    job.file_offset+i, entropy, block_label);
      }

      // submit tracked source counts to the ingest tracker for final reporting
      job.ingest_tracker->track_source(
                               job.file_hash, zero_count, nonprobative_count);
    }

    // submit bytes processed to the ingest tracker for final reporting
    if (job.recursion_depth == 0 && !job.disable_ingest_hashes) {
      job.ingest_tracker->track_bytes(job.buffer_data_size);
    }

    // recursively find and process any uncompressible data
    if (!job.disable_recursive_processing) {
      process_recursive(job);
    }

    // we are now done with this job.  Delete it.
    delete[] job.buffer;
    delete &job;
  }

  // process SCAN job
  static void process_scan_job(const hasher::job_t& job) {

    size_t zero_count = 0;

    // get hash calculator object
    hasher::hash_calculator_t hash_calculator;

    // iterate over buffer to calculate and scan for block hashes
    for (size_t i=0; i < job.buffer_data_size; i+= job.step_size) {

      // skip if all the bytes are the same
      if (all_zero(job.buffer, job.buffer_size, i, job.block_size)) {
        ++zero_count;
        continue;
      }

      // calculate block hash
      const std::string block_hash = hash_calculator.calculate(job.buffer,
                  job.buffer_size, i, job.block_size);

      // scan
      const std::string json_string = 
              job.scan_manager->find_hash_json(job.scan_mode, block_hash);

      if (json_string.size() > 0) {
        // offset <tab> file <tab> json
        std::stringstream ss;
        ss << job.file_offset + i << "\t"
           << hashdb::bin_to_hex(block_hash) << "\t"
           << json_string << "\n";
        hashdb::tprint(ss.str());
      }
    }

    // submit tracked zero_count to the scan tracker for final reporting
    job.scan_tracker->track_zero_count(zero_count);

    // submit tracked bytes processed to the scan tracker for final reporting
    if (job.recursion_depth == 0) {
      job.scan_tracker->track_bytes(job.buffer_data_size);
    }

    // recursively find and process any uncompressible data
    if (!job.disable_recursive_processing) {
      process_recursive(job);
    }

    // we are now done with this job.  Delete it.
    delete[] job.buffer;
    delete &job;
  }

  void process_job(const hasher::job_t& job) {

    switch(job.job_type) {
      case hasher::job_type_t::INGEST: {
        process_ingest_job(job);
        break;
      }

      case hasher::job_type_t::SCAN: {
        process_scan_job(job);
        break;
      }

      default:
        assert(0);
    }
  }
} // end namespace hasher

