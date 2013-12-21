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
#include <iostream>
#include <limits>

class source_lookup_encoding {

  private:
  // program error if source lookup index bits are out of range
  static inline void check_source_lookup_index_bits(uint8_t i) {
    if (i < 32 || i > 40) {
      std::cerr << "invalid i " << (uint32_t)i << "\n";
      assert(0);
    }
  }

  // runtime error if source lookup index is too large
  static inline void check_source_lookup_index(uint8_t i, uint64_t index) {
    check_source_lookup_index_bits(i);
    if (index >= (uint64_t)1<<i) {
      std::ostringstream ss;
      ss << "Error: The source lookup index has become too big for the current number of source lookup index bits specified, " << i << ".\n";
      ss << "No more source lookup records can be allocated at this setting.\n";
      ss << "Use a larger number of index bits.\n";
      ss << "Cannot continue.  Aborting.\n";
      throw std::runtime_error(ss.str());
    }
  }

  // runtime error if hash block offset is too large
  static inline void check_hash_block_offset(uint8_t i, uint64_t offset) {
    check_source_lookup_index_bits(i);
    if (offset >= (uint64_t)1<<(64 - i)) {
      std::ostringstream ss;
      ss << "Error: The hash block offset value is too large for the current number of source lookup index bits specified, " << i << ".\n";
      ss << "No more source lookup records can be allocated at this setting.\n";
      ss << "Use a number of index bits type with a higher capacity.\n";
      ss << "Use a smaller number of index bits.\n";
      ss << "Cannot continue.  Aborting.\n";
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
                   uint8_t source_lookup_index_bits,
                   uint64_t source_lookup_index,
                   uint64_t hash_block_offset) {

    // validate request
    check_source_lookup_index_bits(source_lookup_index_bits);
    check_source_lookup_index(source_lookup_index_bits, source_lookup_index);
    check_hash_block_offset(source_lookup_index_bits, hash_block_offset);

    return (source_lookup_index >> source_lookup_index_bits | hash_block_offset);
  }

  /**
   * Get the source lookup encoding for a count value.
   */
  static uint64_t get_source_lookup_encoding(uint32_t count) {
    return ((uint64_t)count<<32 | 0x0000ffff);
  }

  /**
   * Get the count value represented by the source lookup encoding.
   */
  static uint32_t get_count(uint64_t source_lookup_encoding) {
    if ((source_lookup_encoding & 0x0000ffff) == 0x0000ffff) {
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
                   uint8_t source_lookup_index_bits,
                   uint64_t source_lookup_encoding) {

    // validate request
    check_source_lookup_index_bits(source_lookup_index_bits);

    return source_lookup_encoding >> source_lookup_index_bits;
  }

  /**
   * Get the hash block offset value given a bit size specification.
   */
  static uint64_t get_hash_block_offset(
                   uint8_t source_lookup_index_bits,
                   uint64_t source_lookup_encoding) {

    // validate request
    check_source_lookup_index_bits(source_lookup_index_bits);

    // calculate bit mask for the hash block offset bit fields
    uint64_t bit_mask = ((uint64_t)1<<(64 - source_lookup_index_bits)) - 1;

    return (source_lookup_encoding & bit_mask);
  }
};

/*
inline std::ostream& operator<<(std::ostream& os,
                         const class source_lookup_record_t& lookup) {
  os << "(source_lookup_record value=0x"
     << std::hex << lookup.composite_value_exported_for_testing() << std::dec << ")";
  return os;
}

// hash store element pair
typedef std::pair<md5_t, source_lookup_record_t> hash_store_element_t;
inline std::ostream& operator<<(std::ostream& os,
                         const hash_store_element_t& hash_store_element) {
  os << "(md5=" << hash_store_element.first.hexdigest()
     << ",source_lookup_record=" << hash_store_element.second << ")";
  return os;
}
*/


#endif

