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
 * job data is used by threads for ingesting or scanning data.
 * There are two job types, see job_type_t.
 */

#ifndef JOB_HPP
#define JOB_HPP

#include <cstring>
#include <sstream>
#include <cstdlib>
#include <stdint.h>
//#include <unistd.h>
#include "hashdb.hpp"
#include "hash_calculator.hpp"
#include "ingest_tracker.hpp"
#include "scan_tracker.hpp"

namespace hasher {

enum job_type_t {INGEST, SCAN};

class job_t {

  private:
  // return recursed filename based on parent attributes
  std::string recursed_filename(const std::string& parent_filename,
                                const size_t offset,
                                const std::string& compression_name) {
    std::stringstream ss;
    ss << parent_filename << "-"
       << offset << "-"
       << compression_name;
    return ss.str();
  }

  public:
  const job_type_t job_type;
  hashdb::import_manager_t* const import_manager;
  hasher::ingest_tracker_t* const ingest_tracker;
  const hashdb::scan_manager_t* const whitelist_scan_manager;
  const std::string repository_name;
  hashdb::scan_manager_t* const scan_manager;
  hasher::scan_tracker_t* const scan_tracker;
  const size_t step_size;
  const size_t block_size;
  const std::string file_hash;
  const std::string filename;
  const uint64_t file_offset;
  const uint8_t* const buffer;
  const size_t buffer_size;
  const size_t buffer_data_size;
  const size_t max_recursion_depth;
  const size_t recursion_depth;
  std::string error_message;

  job_t(const job_type_t p_job_type,
               hashdb::import_manager_t* const p_import_manager,
               hasher::ingest_tracker_t* const p_ingest_tracker,
               const hashdb::scan_manager_t* const p_whitelist_scan_manager,
               const std::string p_repository_name,
               hashdb::scan_manager_t* const p_scan_manager,
               hasher::scan_tracker_t* const p_scan_tracker,
               const size_t p_step_size,
               const size_t p_block_size,
               const std::string p_file_hash,
               const std::string p_filename,
               const uint64_t p_file_offset,
               const uint8_t* const p_buffer,
               const size_t p_buffer_size,
               const size_t p_buffer_data_size,
               const size_t p_max_recursion_depth,
               const size_t p_recursion_depth) :
                   job_type(p_job_type),
                   import_manager(p_import_manager),
                   ingest_tracker(p_ingest_tracker),
                   whitelist_scan_manager(p_whitelist_scan_manager),
                   repository_name(p_repository_name),
                   scan_manager(p_scan_manager),
                   scan_tracker(p_scan_tracker),
                   step_size(p_step_size),
                   block_size(p_block_size),
                   file_hash(p_file_hash),
                   filename(p_filename),
                   file_offset(p_file_offset),
                   buffer(p_buffer),
                   buffer_size(p_buffer_size),
                   buffer_data_size(p_buffer_data_size),
                   max_recursion_depth(p_max_recursion_depth),
                   recursion_depth(p_recursion_depth),
                   error_message("") {
  }

  private:
  // do not allow copy or assignment
  job_t(const job_t&);
  job_t& operator=(const job_t&);

  public:

  // ingest
  static job_t* new_ingest_job(
        hashdb::import_manager_t* const p_import_manager,
        hasher::ingest_tracker_t* const p_ingest_tracker,
        const hashdb::scan_manager_t* const p_whitelist_scan_manager,
        const std::string p_repository_name,
        const size_t p_step_size,
        const size_t p_block_size,
        const std::string p_file_hash,
        const std::string p_filename,
        const uint64_t p_file_offset,
        const uint8_t* const p_buffer,
        const size_t p_buffer_size,
        const size_t p_buffer_data_size,
        const size_t p_max_recursion_depth,
        const size_t p_recursion_depth) {

    return new job_t(
                     job_type_t::INGEST,
                     p_import_manager,
                     p_ingest_tracker,
                     p_whitelist_scan_manager,
                     p_repository_name,
                     NULL, // scan_manager
                     NULL, // scan_tracker
                     p_step_size,
                     p_block_size,
                     p_file_hash,
                     p_filename,
                     p_file_offset,
                     p_buffer,
                     p_buffer_size,
                     p_buffer_data_size,
                     p_max_recursion_depth,
                     p_recursion_depth);
  }

  // scan
  static job_t* new_scan_job(
        hashdb::scan_manager_t* const p_scan_manager,
        hasher::scan_tracker_t* const p_scan_tracker,
        const size_t p_step_size,
        const size_t p_block_size,
        const std::string p_filename,
        const uint64_t p_file_offset,
        const uint8_t* const p_buffer,
        const size_t p_buffer_size,
        const size_t p_buffer_data_size,
        const size_t p_max_recursion_depth,
        const size_t p_recursion_depth) {

    return new job_t(
                     job_type_t::SCAN,
                     NULL, // import_manager
                     NULL, // ingest_tracker
                     NULL, // whitelist_scan_manager
                     "",   // repository_name
                     p_scan_manager,
                     p_scan_tracker,
                     p_step_size,
                     p_block_size,
                     "",   // file hash
                     p_filename,
                     p_file_offset,
                     p_buffer,
                     p_buffer_size,
                     p_buffer_data_size,
                     p_max_recursion_depth,
                     p_recursion_depth);
  }

  // recursed
  static job_t* new_recursed_job(
        const hasher::job_t& parent_job,
        const size_t relative_offset,
        const std::string& compression_name,
        const uint8_t* const uncompressed_buffer,
        const size_t uncompressed_size) {

    // calculate the recursed file hash
    hash_calculator_t hash_calculator;
    const std::string recursed_file_hash = hash_calculator.calculate(
            uncompressed_buffer, uncompressed_size, 0, uncompressed_size);

    // calculate the absolute offset
    const size_t absolute_offset = (parent_job.recursion_depth == 0)
               ? parent_job.file_offset + relative_offset : relative_offset;

    // calculate the recursed filename
    std::stringstream ss;
    ss << parent_job.filename<< "-"
       << absolute_offset << "-"
       << compression_name;
    const std::string recursed_filename = ss.str();

    return new job_t(
                   parent_job.job_type,
                   parent_job.import_manager,
                   parent_job.ingest_tracker,
                   parent_job.whitelist_scan_manager,
                   parent_job.repository_name,
                   parent_job.scan_manager,
                   parent_job.scan_tracker,
                   parent_job.step_size,
                   parent_job.block_size,
                   recursed_file_hash,
                   recursed_filename,
                   0, // file_offset
                   uncompressed_buffer,
                   uncompressed_size,
                   uncompressed_size,
                   parent_job.max_recursion_depth,
                   parent_job.recursion_depth + 1);
  }
};

} // end namespace hasher

#endif
