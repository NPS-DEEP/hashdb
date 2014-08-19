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
 * Provides conversions between the source lookup encoding and the
 * source lookup index and the file offset.
 *
 * Allocation is: 34 bits toward calculating file offset,
 *                30 bits toward the source lookup index.
 *
 * Actual file offset = byte alignment offset * HASHDB_BYTE_ALIGNMENT
 */

#ifndef SOURCE_LOOKUP_ENCODING_HPP
#define SOURCE_LOOKUP_ENCODING_HPP
#include <stdint.h>
#include <cassert>
#include <string>
#include <stdexcept>
//#include <iostream>
//#include <limits>

namespace source_lookup_encoding {

  /**
   * Get the source lookup encoding given a source lookup index and file offset.
   */
  inline static uint64_t get_source_lookup_encoding(
                   uint64_t source_lookup_index,
                   uint64_t file_offset) {

    // runtime error if source lookup index is too large
    uint64_t max1 = ((uint64_t)1<<30) - 1;
    if (source_lookup_index > max1) {
      throw std::runtime_error("Error: The source lookup index is too large.");
    }

    // runtime error if file offset is too large
    uint64_t max2 = (((uint64_t)1<<34) - 1) * HASHDB_BYTE_ALIGNMENT;
    if (file_offset > max2) {
      throw std::runtime_error("Error: The file offset is too large.");
    }

    // runtime error if file offset is not byte-aligned
    if ((file_offset % HASHDB_BYTE_ALIGNMENT) != 0) {
      throw std::runtime_error("Error: The file offset is not byte aligned.");
    }

    // return the value
    return (source_lookup_index << 34) | (file_offset / HASHDB_BYTE_ALIGNMENT);
  }

  /**
   * Get the source lookup index given a source lookup encoding
   */
  inline static uint64_t get_source_lookup_index(
                   uint64_t source_lookup_encoding) {

    return source_lookup_encoding >> 34;
  }

  /**
   * Get the file offset given a source lookup encoding
   */
  inline static uint64_t get_file_offset(
                   uint64_t source_lookup_encoding) {

    // calculate bit mask for the hash block offset bit fields
    uint64_t bit_mask = ((uint64_t)1<<34) - 1;

    return (source_lookup_encoding & bit_mask) * HASHDB_BYTE_ALIGNMENT;
  }
};

#endif

