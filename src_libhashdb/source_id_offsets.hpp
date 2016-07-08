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
 * Provide structure for storing source count and file offsets.
 */

#ifndef SOURCE_ID_OFFSETS_HPP
#define SOURCE_ID_OFFSETS_HPP

#include <string>
#include <cstdint>

namespace hashdb {

typedef std::set<uint64_t> file_offsets_t;

struct source_id_offset_t {
  uint64_t source_id;
  uint64_t sub_count;
  file_offsets_t file_offsets;
  source_id_offset_t() : source_id(0), sub_count(0), file_offsets() {
  }
  source_id_offset_t(uint64_t p_source_id,
                     uint64_t p_sub_count,
                     file_offsets_t p_file_offsets) :
              source_id(p_source_id),
              sub_count(p_sub_count),
              file_offsets(p_file_offsets) {
  }
};

typedef std::set<source_id_offset_t> source_id_offsets_t;

} // end namespace hashdb

#endif

