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
#include <cstdlib>
#include <stdint.h>
//#include <unistd.h>
#include "hashdb.hpp"
#include "source_data_manager.hpp"

namespace hasher {

enum job_type_t {INGEST, SCAN};

class job_t {

  public:
  const job_type_t job_type;
  hashdb::import_manager_t* const import_manager;
  hasher::source_data_manager_t* const source_data_manager;
  const hashdb::scan_manager_t* const whitelist_scan_manager;
  hashdb::scan_manager_t* const scan_manager;
  const std::string repository_name;
  const size_t step_size;
  const size_t block_size;
  const std::string file_hash;
  const std::string filename;
  const uint64_t file_offset;
  const uint8_t* const buffer;
  const size_t buffer_size;
  const size_t buffer_data_size;
  const size_t max_recursion_depth;
  const size_t recursion_count;
  std::string error_message;

  job_t(const job_type_t p_job_type,
               hashdb::import_manager_t* const p_import_manager,
               hasher::source_data_manager_t* const p_source_data_manager,
               const hashdb::scan_manager_t* const p_whitelist_scan_manager,
               hashdb::scan_manager_t* const p_scan_manager,
               const std::string p_repository_name,
               const size_t p_step_size,
               const size_t p_block_size,
               const std::string p_file_hash,
               const std::string p_file_name,
               const uint64_t p_file_offset,
               const uint8_t* const p_buffer,
               const size_t p_buffer_size,
               const size_t p_buffer_data_size,
               const size_t p_max_recursion_depth,
               const size_t p_recursion_count) :
                   job_type(p_job_type),
                   import_manager(p_import_manager),
                   source_data_manager(p_source_data_manager),
                   whitelist_scan_manager(p_whitelist_scan_manager),
                   scan_manager(p_scan_manager),
                   repository_name(p_repository_name),
                   step_size(p_step_size),
                   block_size(p_block_size),
                   file_hash(p_file_hash),
                   filename(p_file_name),
                   file_offset(p_file_offset),
                   buffer(p_buffer),
                   buffer_size(p_buffer_size),
                   buffer_data_size(p_buffer_data_size),
                   max_recursion_depth(p_max_recursion_depth),
                   recursion_count(p_recursion_count),
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
        hasher::source_data_manager_t* const p_source_data_manager,
        const hashdb::scan_manager_t* const p_whitelist_scan_manager,
        const std::string p_repository_name,
        const size_t p_step_size,
        const size_t p_block_size,
        const std::string p_file_hash,
        const std::string p_file_name,
        const uint64_t p_file_offset,
        const uint8_t* const p_buffer,
        const size_t p_buffer_size,
        const size_t p_buffer_data_size,
        const size_t p_max_recursion_depth,
        const size_t p_recursion_count) {

    return new job_t(
                     job_type_t::INGEST,
                     p_import_manager,
                     p_source_data_manager,
                     p_whitelist_scan_manager,
                     NULL, // scan_manager
                     p_repository_name,
                     p_step_size,
                     p_block_size,
                     p_file_hash,
                     p_file_name,
                     p_file_offset,
                     p_buffer,
                     p_buffer_size,
                     p_buffer_data_size,
                     p_max_recursion_depth,
                     p_recursion_count);
  }

  // scan
  static job_t* new_scan_job(
        hashdb::scan_manager_t* const p_scan_manager,
        const size_t p_step_size,
        const size_t p_block_size,
        const std::string p_file_name,
        const uint64_t p_file_offset,
        const uint8_t* const p_buffer,
        const size_t p_buffer_size,
        const size_t p_buffer_data_size,
        const size_t p_max_recursion_depth,
        const size_t p_recursion_count) {


    return new job_t(
                     job_type_t::SCAN,
                     NULL, // import_manager
                     NULL, // source_data_manager
                     NULL, // whitelist_scan_manager
                     p_scan_manager,
                     "",   // repository_name
                     p_step_size,
                     p_block_size,
                     "",   // file hash
                     p_file_name,
                     p_file_offset,
                     p_buffer,
                     p_buffer_size,
                     p_buffer_data_size,
                     p_max_recursion_depth,
                     p_recursion_count);
  }
};

} // end namespace hasher

#endif
