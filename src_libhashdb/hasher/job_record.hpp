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
 * This data structure is used by threads in the threadpool for ingesting
 * or scanning data.
 *
 * There are four job types, see job_type_t.
 * See job_t generators for parameters required by each job type.
 */

#ifndef JOB_RECORD_HPP
#define JOB_RECORD_HPP

#include <cstring>
#include <cstdlib>
#include <stdint.h>
//#include <unistd.h>

namespace hasher {

enum job_type_t {INGEST_FILE, SCAN_FILE, INGEST_BUFFER, SCAN_BUFFER};

class job_record_t {

  private:
  const job_type_t job_type;
  hasher::threadpool_t* const threadpool;
  hashdb::import_manager_t* const import_manager;
  const hashdb::scan_manager_t* const whitelist_scan_manager;
  const hashdb::scan_manager_t* const scan_manager;
  const std::string repository_name;
  const size_t step_size;
  const size_t block_size;
  const hasher::filel_reader_t* const file_reader;
  const std::string source_hash;
  const std::string source_name;
  const uint64_t source_offset;
  const uint8_t* const buffer;
  const size_t buffer_size;
  const size_t buffer_data_size;
  const size_t recursion_count;
  size_t nonprobative_count;
  std::string error_message;

  job_record_t(const job_type_t p_job_type,
               const hasher::threadpool_t* const p_threadpool,
               const hashdb::import_manager_t* const p_import_manager,
               const hashdb::scan_manager_t* const p_whitelist_scan_manager,
               const hashdb::scan_manager_t* const p_scan_manager,
               const std::string p_repository_name,
               const size_t p_step_size,
               const size_t p_block_size,
               const hasher::file_reader_t* const p_file_reader,
               const std::string p_source_hash,
               const std::string p_source_name,
               const uint64_t p_source_offset,
               const uint8_t* const p_buffer,
               const size_t p_buffer_size,
               const size_t p_buffer_data_size,
               const size_t p_recursion_count) :
                   job_type(p_job_type),
                   threadpool(p_threadpool),
                   import_manager(p_import_manager),
                   whitelist_scan_manager(p_whitelist_scan_manager),
                   scan_manager(p_scan_manager),
                   repository_name(p_repository_name),
                   step_size(p_step_size),
                   block_size(p_block_size),
                   file_reader(p_file_reader),
                   source_hash(p_source_hash),
                   source_name(p_source_name),
                   source_offset(p_source_offset),
                   buffer(p_buffer),
                   buffer_size(p_buffer_size),
                   buffer_data_size(p_buffer_data_size),
                   recursion_count(p_recursion_count),
                   nonprobative_count(0),
                   error_message("") {
  }

  // do not allow copy or assignment
  job_record_t(const job_record_t&);
  job_record_t& operator=(const job_record_t&);

  public:
  static job_record_t* new_ingest_file_job_record(
        const hasher::threadpool_t* const threadpool,
        const hashdb::import_manager_t* const import_manager,
        const hashdb::scan_manager_t* const whitelist_scan_manager,
        const std::string repository_name,
        const size_t step_size,
        const size_t block_size,
        const hasher::file_reader_t* const file_reader) {

    return new job_record_t(
                     job_type_t::INGEST_FILE,
                     threadpool,
                     import_manager,
                     whitelist_scan_manager,
                     repository_name,
                     step_size,
                     block_size,
                     file_reader,
                     "",   // source hash
                     "",   // source name
                     0,    // source offset
                     NULL, // buffer
                     0,    // buffer size
                     0,    // buffer data size
                     0,    // recursion count
                     "");  // error message
  }

  static job_record_t* new_scan_file_job_record(
        const hasher::threadpool_t* const threadpool,
        const hashdb::scan_manager_t* const scan_manager,
        const size_t step_size,
        const size_t block_size,
        const hasher::file_reader_t* const file_reader) {

    return new job_record_t(
                     job_type_t::SCAN_FILE,
                     threadpool,
                     import_manager,
                     NULL, // whitelist_scan_manager
                     NULL, // scan_manager
                     "",   // repository_name
                     step_size,
                     block_size,
                     file_reader,
                     "",   // source hash
                     "",   // source name
                     0,    // source offset
                     NULL, // buffer
                     0,    // buffer size
                     0,    // buffer data size
                     0,    // recursion count
                     "");  // error message
  }

  static job_record_t* new_ingest_buffer_job_record(
        const hasher::threadpool_t* const threadpool,
        const hashdb::import_manager_t* const import_manager,
        const hashdb::scan_manager_t* const whitelist_scan_manager,
        const std::string repository_name,
        const size_t step_size,
        const size_t block_size,
        const std::string source_hash,
        const std::string source_name,
        const uint64_t source_offset,
        const uint8_t* const buffer,
        const size_t buffer_size,
        const size_t buffer_data_size,
        const size_t recursion_count) {

    return new job_record_t(
                     job_type_t::INGEST_FILE,
                     threadpool,
                     import_manager,
                     whitelist_scan_manager,
                     repository_name,
                     step_size,
                     block_size,
                     NULL, // file_reader
                     source_hash,
                     source_name,
                     source_offset,
                     buffer,
                     buffer_size,
                     buffer_data_size,
                     recursion_count,
                     "");  // error message
  }

  static job_record_t* new_scan_buffer_job_record(
        const hasher::threadpool_t* const threadpool,
        const hashdb::scan_manager_t* const scan_manager,
        const size_t step_size,
        const size_t block_size,
        const std::string source_hash,
        const std::string source_name,
        const uint64_t source_offset,
        const uint8_t* const buffer,
        const size_t buffer_size,
        const size_t buffer_data_size,
        const size_t recursion_count) {


    return new job_record_t(
                     job_type_t::SCAN_FILE,
                     threadpool,
                     import_manager,
                     NULL, // whitelist_scan_manager
                     NULL, // scan_manager
                     "",   // repository_name
                     step_size,
                     block_size,
                     file_reader,
                     "",   // source hash
                     "",   // source name
                     0,    // source offset
                     NULL, // buffer
                     0,    // buffer size
                     0,    // buffer data size
                     0,    // recursion count
                     "");  // error message
  }
};

} // end namespace hasher

#endif
