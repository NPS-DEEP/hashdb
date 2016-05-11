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
#include "hashdb.hpp"
#include "job.hpp"
#include "hash_calculator.hpp"
#include "calculate_entropy.hpp"
#include "calculate_block_label.hpp"

namespace hasher {

  // process INGEST job
  static void process_ingest_job(const hasher::job_t& job) {

    // get hash calculator object
    hasher::hash_calculator_t hash_calculator;

    // iterate over buffer to add block hashes and metadata
    size_t nonprobative_count = 0;
    for (size_t i=0; i < job.buffer_data_size; i+= job.step_size) {

      // block hash
      std::string block_hash = hash_calculator.calculate(job.buffer,
                  job.buffer_size, i, job.block_size);

      // entropy
      size_t entropy = hasher::calculate_entropy(job.buffer,
                  job.buffer_size, i, job.block_size);

      // block label
      std::string block_label = hasher::calculate_block_label(job.buffer,
                  job.buffer_size, i, job.block_size);
      if (block_label.size() != 0) {
        ++nonprobative_count;
      }

      // add block hash to DB
      job.import_manager->insert_hash(block_hash, job.file_hash,
                  job.file_offset+i, entropy, block_label);
    }

    // submit nonprobative count to source data manager
    job.source_data_manager->update_nonprobative_count(
                                       job.file_hash, nonprobative_count);

    // we are now done with this job.  Delete it.
    delete[] job.buffer;
    delete &job;
  }

  // process SCAN job
  static void process_scan_job(const hasher::job_t& job) {

    // get hash calculator object
    hasher::hash_calculator_t hash_calculator;

    // iterate over buffer to calculate and scan for block hashes
    for (size_t i=0; i < job.buffer_data_size; i+= job.step_size) {

      // block hash
      const std::string block_hash = hash_calculator.calculate(job.buffer,
                  job.buffer_size, i, job.block_size);

      const std::string json_string = 
                     job.scan_manager->find_expanded_hash_json(block_hash);

      if (json_string.size() > 0) {
        // offset <tab> file <tab> json
        std::cout << job.file_offset + i << job.filename << json_string << "\n";
      }
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

#endif
