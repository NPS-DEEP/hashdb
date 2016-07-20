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
        const uint64_t p_filesize,
        const uint64_t p_file_offset,
        const bool p_disable_recursive_processing,
        const bool p_disable_calculate_entropy,
        const bool p_disable_calculate_labels,
        const bool p_disable_ingest_hashes,
        const hashdb::scan_mode_t p_scan_mode,
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
                   filesize(p_filesize),
                   file_offset(p_file_offset),
                   disable_recursive_processing(p_disable_recursive_processing),
                   disable_calculate_entropy(p_disable_calculate_entropy),
                   disable_calculate_labels(p_disable_calculate_labels),
                   disable_ingest_hashes(p_disable_ingest_hashes),
                   scan_mode(p_scan_mode),
                   buffer(p_buffer),
                   buffer_size(p_buffer_size),
                   buffer_data_size(p_buffer_data_size),
                   max_recursion_depth(p_max_recursion_depth),
                   recursion_depth(p_recursion_depth),
                   error_message("") {
  }

  // do not allow copy or assignment
  job_t(const job_t&);
  job_t& operator=(const job_t&);

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
  const uint64_t filesize;
  const uint64_t file_offset;
  const bool disable_recursive_processing;
  const bool disable_calculate_entropy;
  const bool disable_calculate_labels;
  const bool disable_ingest_hashes;
  const hashdb::scan_mode_t scan_mode;
  const uint8_t* const buffer;
  const size_t buffer_size;
  const size_t buffer_data_size;
  const size_t max_recursion_depth;
  const size_t recursion_depth;
  std::string error_message;

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
        const uint64_t p_filesize,
        const uint64_t p_file_offset,
        const bool p_disable_recursive_processing,
        const bool p_disable_calculate_entropy,
        const bool p_disable_calculate_labels,
        const bool p_disable_ingest_hashes,
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
                     p_filesize,
                     p_file_offset,
                     p_disable_recursive_processing,
                     p_disable_calculate_entropy,
                     p_disable_calculate_labels,
                     p_disable_ingest_hashes,
                     hashdb::scan_mode_t::EXPANDED, // scan_mode not used
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
        const uint64_t p_filesize,
        const uint64_t p_file_offset,
        const bool p_disable_recursive_processing,
        const hashdb::scan_mode_t p_scan_mode,
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
                     p_filesize,
                     p_file_offset,
                     p_disable_recursive_processing,
                     false, // disable_calculate_entropy
                     false, // disable_calculate_labels
                     false, // disable_ingest_hashes
                     p_scan_mode,
                     p_buffer,
                     p_buffer_size,
                     p_buffer_data_size,
                     p_max_recursion_depth,
                     p_recursion_depth);
  }
};

} // end namespace hasher

#endif
