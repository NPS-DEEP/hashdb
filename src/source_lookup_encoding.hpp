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
 * Provides algorithms for working with a uint64_t as a source lookup
 * encoding containing source lookup index, hash block offset, and count values.
 * This encoding is a performance optimization.
 *
 * bits are spread across uint64_t in one of the following ways:
 *   source lookup index (32-40) ... hash block offset (32-24).
 *   or
 *   count ... 0xffff
 */

#ifndef SOURCE_LOOKUP_ENCODING_HPP
#define SOURCE_LOOKUP_ENCODING_HPP
#include <stdint.h>
#include <cassert>
#include <string>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <limits>

class source_lookup_encoding {

  private:

  // the hardcoded bit distribution
  static uint8_t source_lookup_index_bits = 32;

  // runtime error if source lookup index is too large
  static inline void check_source_lookup_index(uint64_t index) {
    uint64_t max = ((uint64_t)1<<source_lookup_index_bits) - 2;
    if (index > max) {
      std::ostringstream ss;
      ss << "Error: The source lookup index is too large for the "
         << (uint32_t)source_lookup_index_bits
         << " index bits\ncurrently specified.\n";
      ss << "Specifically, requested index " << index << " > max " << max << ".\n";
      throw std::runtime_error(ss.str());
    }
  }

  // runtime error if hash block offset is too large
  static inline void check_hash_block_offset(uint64_t offset) {
    uint64_t max = ((uint64_t)1<<(64 - source_lookup_index_bits)) - 2;
    if (offset > max) {
      std::ostringstream ss;
      ss << "Error: The hash block offset value is too large for the "
         << (uint32_t)source_lookup_index_bits
         << " index bits\ncurrently specified.\n";
      ss << "Specifically, requested offset " << offset << " > max " << max << ".\n";
      throw std::runtime_error(ss.str());
    }
  }

  // do not instantiate this
  source_lookup_encoding();

  public:
  /**
   * Get the source lookup encoding for a source lookup index
   * and hash block offset value given a bit size specification.
   */
  static uint64_t get_source_lookup_encoding(
                   uint64_t source_lookup_index,
                   uint64_t hash_block_offset) {

    // validate request
    check_source_lookup_index(source_lookup_index);
    check_hash_block_offset(hash_block_offset);

    return ((source_lookup_index << (64 - source_lookup_index_bits)) | hash_block_offset);
  }

  /**
   * Get the source lookup encoding for a count value.
   */
  static uint64_t get_source_lookup_encoding(uint32_t count) {
    return ((uint64_t)count<<32 | 0xffffffff);
  }

  /**
   * Get the count value represented by the source lookup encoding.
   */
  static uint32_t get_count(uint64_t source_lookup_encoding) {
    if ((source_lookup_encoding & 0xffffffff) == 0xffffffff) {
      // the source lookup encoding indicates count mode, so return count value
      return source_lookup_encoding>>32;
    } else {
      // the source lookup encoding indicates no count mode,
      // so the count value is 1
      return 1;
    }
  }

  /**
   * Get the source lookup index value given a bit size specification.
   */
  static uint64_t get_source_lookup_index(
                   uint64_t source_lookup_encoding) {

    return source_lookup_encoding >> (64 - source_lookup_index_bits);
  }

  /**
   * Get the hash block offset value given a bit size specification.
   */
  static uint64_t get_hash_block_offset(
                   uint64_t source_lookup_encoding) {

    // calculate bit mask for the hash block offset bit fields
    uint64_t bit_mask = ((uint64_t)1<<(64 - source_lookup_index_bits)) - 1;

    return (source_lookup_encoding & bit_mask);
  }
};

#endif

