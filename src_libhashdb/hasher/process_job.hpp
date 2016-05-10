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

#ifndef PROCESS_JOB_HPP
#define PROCESS_JOB_HPP

#include <cstring>
#include <cstdlib>
#include <stdint.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <unistd.h>
#include <sched.h>  // sched_yield
#include "job.hpp"

namespace hasher {

  std::string process_job(const hasher::job_t& job) {






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



} // end namespace hasher

#endif
