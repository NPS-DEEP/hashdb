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
#include <zlib.h>
#include "hashdb.hpp"
#include "job.hpp"
#include "tprint.hpp"
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
std::cerr << "proces_recursive.recurse size " << uncompressed_size << "\n";

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

      // this is similar to scan_image.cpp
      case hasher::job_type_t::SCAN: {

        // calculate the parent file offset
        const uint64_t parent_file_offset = (parent_job.recursion_depth == 0)
               ? parent_job.file_offset + relative_offset : relative_offset;

        // calculate the recursed filename
        const std::string recursed_filename = make_recursed_filename(
                  parent_job.filename, parent_file_offset, compression_name);

        job_t* recursed_scan_image_job = job_t::new_scan_job(
                   parent_job.scan_manager,
                   parent_job.scan_tracker,
                   parent_job.step_size,
                   parent_job.block_size,
                   recursed_filename,
                   0,                 // file_offset
                   parent_job.disable_recursive_processing,
                   uncompressed_buffer,
                   uncompressed_size, // buffer_size
                   uncompressed_size, // buffer_data_size
                   parent_job.max_recursion_depth,
                   parent_job.recursion_depth + 1);

        // run the new recursed scan image job
        process_job(*recursed_scan_image_job);
        break;
      }
    }
  }

  static const uint32_t zip_name_len_max = 1024;
  static const size_t uncompressed_size_min = 6;
  static const size_t uncompressed_size_max = 16777216; // 2^24 = 16MiB

  inline uint16_t u16(const uint8_t* const b) {
    return (uint16_t)(b[0]<<0) | (uint16_t)(b[1]<<8);
  }

  inline uint32_t u32(const uint8_t* const b) {
    return (uint32_t)(b[0]<<0) | (uint32_t)(b[1]<<8) |
           (uint32_t)(b[2]<<16) | (uint32_t)(b[3]<<24);
  }

  inline bool zip_signature(const uint8_t* const b) {
    // do not let this overflow.
    return (b[0] == 0x50 && b[1]==0x4B && b[2]==0x03 && b[3]==0x04);
  }

  static void process_zip(const hasher::job_t& job, size_t offset) {
std::cerr << "process_zip.a\n";

    const uint8_t* const b = job.buffer + offset;

    const uint16_t version_needed_to_extract= u16(b+4);
    const uint32_t compr_size=u32(b+18);
    const uint32_t uncompr_size=u32(b+22);
    const uint16_t name_len=u16(b+26);
    const uint16_t extra_field_len=u16(b+28);

//    // validate version
//    if (version_needed_to_extract != 20) {
//std::cerr << "process_zip.version needed " << version_needed_to_extract << "\n";
//      return;
//    }

std::cerr << "process_zip.b\n";
    // validate name length
    if (name_len == 0 || name_len > zip_name_len_max) {
      return;
    }

std::cerr << "process_zip.c\n";
    // calculate offset to compressed data
    uint32_t compressed_offset = offset + 30 + name_len + extra_field_len;

    // offset must be inside the buffer
    if (compressed_offset >= job.buffer_size) {
      return;
    }

std::cerr << "process_zip.d\n";
    // size of compressed data
    const uint32_t compressed_size = (compr_size == 0 ||
               compressed_offset + compr_size > job.buffer_size) 
                          ? job.buffer_size - compressed_offset : compr_size;

    // size of uncompressed data
    const uint32_t potential_uncompressed_size =
               (compr_size == 0 || compr_size > uncompressed_size_max)
                                  ? uncompressed_size_max : uncompr_size;
    
    // skip if uncompressed size is too small
    if (potential_uncompressed_size < uncompressed_size_min) {
      return;
    }

std::cerr << "process_zip.e\n";
    // create the uncompressed buffer
    uint8_t* uncompressed_buffer =
                 new (std::nothrow) uint8_t[potential_uncompressed_size]();
    if (uncompressed_buffer == NULL) {
      // comment that the buffer acquisition request failed
      std::stringstream ss;
      ss << "# bad memory allocation in uncompression in file "
         << job.filename << "\n";
      tprint(ss.str());
      return;
    }

    // set up zlib data
    z_stream zs;
    memset(&zs, 0, sizeof(zs));
    zs.next_in = const_cast<Bytef *>(reinterpret_cast<const Bytef *>(
                                         job.buffer + compressed_offset));
    zs.avail_in = compressed_size;
    zs.next_out = uncompressed_buffer;
    zs.avail_out = potential_uncompressed_size;

    // total_out
    uint32_t total_out = 0;

    // initialize zlib for this decompression
    int r = inflateInit2(&zs, -15);
    if (r == 0) {
      // inflate
      inflate(&zs, Z_SYNC_FLUSH);

      // note total_out
      total_out = zs.total_out;

      // close zlib
      inflateEnd(&zs);
    }
std::cerr << "process_zip.total_out " << total_out << ", potential_uncompressed_size: " << potential_uncompressed_size << "\n";

    // recurse if there was any decompressed data
    if (total_out > 0) {
      recurse(job, offset, "zip", uncompressed_buffer, total_out);

    } else {
      // release the empty uncompressed_buffer
      delete[] uncompressed_buffer;
    }
  }

  void process_recursive(const hasher::job_t& job) {
std::cerr << "process_recursive.a\n";

    // impose max recursion depth
    if (job.recursion_depth >= 7) {
      // too much recursive depth
      return;
    }

    // scan each byte for a compression signature, stop before end
    for (size_t i=0; i+100 < job.buffer_data_size; ++i) {

      if (zip_signature(job.buffer+i)) {
        process_zip(job, i);
      }

/*
      if (gzip_signature(job.buffer+i)) {
        process_gzip(job, i);
      }
*/
    }
  }
} // end namespace hasher

