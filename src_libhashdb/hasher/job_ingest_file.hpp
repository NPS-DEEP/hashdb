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
 * or scanning its referenced buffer of data.
 *
 * There are four job types, see job_type_t.
 * See job_t generators for parameters required by each job type.
 */

#ifndef JOB_INGEST_FILE_HPP
#define JOB_INGEST_FILE_HPP

#include <cstring>
#include <cstdlib>
#include <stdint.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <unistd.h>

namespace hasher {

  static const size_t BUFFER_DATA_SIZE = 16777216;   // 2^24=16MiB
  static const size_t BUFFER_SIZE = 17825792;        // 2^24+2^20=17MiB

  static void* job_ingest_file(void* arg) {

    // get job_record
    hashdb::job_record_t* job_record = static_cast<hashdb::job_record_t*>(arg);
    const hashdb::file_reader_t& file_reader = *job_record->file_reader;

    // create a buffer to read into
    size_t b_size = (job_record->file_reader->filesize <= BUFFER_SIZE) ?
                          job_record->file_reader->filesize : BUFFER_SIZE;
    uint8_t* b = new (std::nothrow) uint8_t[b_size];
    if (b == NULL) {
      // abort
      job_record->error_message = "bad memory allocation";
      pthread_exit(static_cast<void*>(job_record));
    }

    // read into the buffer
    size_t bytes_read;
    std::string success = job_record->file_reader->read(b, b_size, &bytes_read);
    if (success.size() > 0) {
      // abort
      delete[] b;
      job_record->error_message = "bad memory allocation";
      pthread_exit(static_cast<void*>(job_record));
    }

    // calculate the file hash
    hasher::hash_calculator_t hash_calculator;
    hash_calculator.init();

    // hash first buffer
    hash_calculator.update(b, b_size, 0, b_size);

    // read and hash subsequent buffers
    if (job_record->file_reader->filesize > BFFER_SIZE) {

      // create b2 to read into
      uint8_t* b2 = new (std::nothrow) uint8_t[BUFFER_SIZE];
      if (b2 == NULL) {
        // abort
        delete[] b;
        job_record->error_message = "bad memory allocation";
        pthread_exit(static_cast<void*>(job_record));
      }

      // read and hash
      for (uint64_t offset = BUFFER_SIZE;
           offset < job_record->file_reader->filesize;
           offset += BUFFER_SIZE) {

        // read into b2
        size_t b2_bytes_read
        std::string success = job_record->file_reader->read(
                                          b2, BUFFER_SIZE, &b2_bytes_read);
        if (success.size() > 0) {
          // abort
          delete[] b2;
          delete[] b;
          job_record->error_message = "bad memory allocation";
          pthread_exit(static_cast<void*>(job_record));
        }

        // hash b2
        hash_calculator.update(b2, bytes_read, 0, bytes_read);
      }

      delete[] b2;
    }

    // retrieve source file hash
    std::string file_hash = hash_calculator.final();

    // ingest source file name
    job_record->import_manager->insert_source_name(file_hash,
            job_record->repository_name, job_record->file_reader->filename);

    // now ingest buffers
    // first buffer
    hasher::job_record_t* job_record = 
                        hasher::job_record_t::new_ingest_buffer_job_record(
                 job_record->threadpool,
                 job_record->import_manager,
                 job_record->whitelist_scan_manager,
                 job_record->repository_name,
                 job_record->step_size,
                 job_record->block_size,
                 file_hash,
                 job_record->file_reader->filename,
                 0,      // source_offset
                 b,      // buffer
                 b_size, // buffer_size
                 b_size, // buffer_data_size
                 0);     // recursion count

    // zz span this thread

    // prepare subsequent threads
    for (uint64_t offset = BUFFER_SIZE;
         offset < job_record->file_reader->filesize;
         offset += BUFFER_SIZE) {

      // create b2 to read into
      uint8_t* b2 = new (std::nothrow) uint8_t[BUFFER_SIZE];
      if (b2 == NULL) {
        // abort
        delete[] b;
        job_record->error_message = "bad memory allocation";
        pthread_exit(static_cast<void*>(job_record));
      }

      // read into b2
      size_t b2_bytes_read
      std::string success = job_record->file_reader->read(
                                        b2, BUFFER_SIZE, &b2_bytes_read);
      if (success.size() > 0) {
        // abort
        delete[] b2;
        delete[] b;
        job_record->error_message = "bad memory allocation";
        pthread_exit(static_cast<void*>(job_record));
      }

    // zz span subsequent threads



} // end namespace hasher

#endif
