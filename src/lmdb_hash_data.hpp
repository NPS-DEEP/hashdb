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
 * Manage hash data.  New fields may be appended in the future.
 */

#ifndef LMDB_HASH_DATA_HPP
#define LMDB_HASH_DATA_HPP

#include <string>
#include <sstream>
#include <cstdint>
#include <iostream>
#include "lmdb_helper.h"

#define DEBUG

class lmdb_hash_data_t {
  public:
  uint64_t source_id;
  uint64_t offset_index;

  lmdb_hash_data_t() : source_id(0), offset_index(0) {
  }

  lmdb_hash_data_t(uint64_t p_source_id, uint64_t p_offset_index) :
                 source_id(p_source_id), offset_index(p_offset_index) {
  }

  bool operator==(const lmdb_hash_data_t& other) const {
    return (source_id == other.source_id && offset_index == other.offset_index);
  }

  static std::string encode(const lmdb_hash_data_t& data) {

    // allocate space for the encoding
    size_t max_size = 10 + 10;
    uint8_t encoding[max_size];
    uint8_t* p = encoding;
std::cout << "encoding " << encoding << "\n";
std::cout << "p " << &p << "\n";

    // encode each field
    p = lmdb_helper::encode_uint64(data.source_id, p);
    p = lmdb_helper::encode_uint64(data.offset_index, p);

    // return encoding
    std::string string_encoding(reinterpret_cast<char*>(p), (p-encoding));
#ifdef DEBUG
    std::cout << "encoding ";
    data.report_fields(std::cout);
    std::cout << "\n"
              << "      to " << lmdb_helper::binary_hash_to_hex(string_encoding)
              << "\n";
#endif

    return std::string(reinterpret_cast<char*>(p), (p-encoding));
  }

  void report_fields(std::ostream& os) const {
    os << "{\"lmdb_hash_data\":{\"source_id\":" << source_id
       << ",\"offset_index\":" << offset_index
       << "}}";
  }

  static lmdb_hash_data_t decode(const std::string& encoding) {
    const uint8_t* const p_start = reinterpret_cast<const uint8_t*>(encoding.c_str());
    const uint8_t* p = p_start;
    lmdb_hash_data_t data;
    p = lmdb_helper::decode_uint64(p, &data.source_id);
    p = lmdb_helper::decode_uint64(p, &data.offset_index);

    // validate that the data was properly consumed
    if (p - p_start != encoding.size()) {
      assert(0);
    }

#ifdef DEBUG
    std::cout << "decoding " << lmdb_helper::binary_hash_to_hex(encoding)
              << "      to ";
    data.report_fields(std::cout);
    std::cout << "\n";
#endif

    // return decoding
    return data;
  }
};

inline std::ostream& operator<<(std::ostream& os,
                        const class lmdb_hash_data_t& data) {
  data.report_fields(os);
  return os;
}

#endif

