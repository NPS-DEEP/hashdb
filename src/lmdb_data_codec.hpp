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

#ifndef LMDB_DATA_CODEC_HPP
#define LMDB_DATA_CODEC_HPP

#include <string>
#include <sstream>
#include <stdint.h>
#include <iostream>
#include "lmdb_helper.h"
#include "db_typedefs.h"
//#include "lmdb_hash_data.hpp"
//#include "lmdb_source_data.hpp"

// #define DEBUG

// note offset_index is calculated from file_offset / sector_size
class lmdb_data_codec {

  public:
  static std::string encode_hash_data(uint64_t source_id,
                                      uint64_t offset_index) {

    // allocate space for the encoding
    size_t max_size = 10 + 10;
    uint8_t encoding[max_size];
    uint8_t* p = encoding;

    // encode source ID and offset index
    p = lmdb_helper::encode_uint64(source_id, p);
    p = lmdb_helper::encode_uint64(offset_index, p);

    // return encoding
    std::string string_encoding(reinterpret_cast<char*>(encoding), (p-encoding));
#ifdef DEBUG
    std::cout << "encoding " << source_id << ", " offset_index << "\n"
              << "      to " << lmdb_helper::binary_hash_to_hex(string_encoding)
              << "\n";
#endif

    return std::string(reinterpret_cast<char*>(encoding), (p-encoding));
  }

  // decode hash data
  static std::pair<uint64_t, uint64_t> decode_hash_data(
                                           const std::string& encoding) {
    const uint8_t* const p_start = reinterpret_cast<const uint8_t*>(encoding.c_str());
    const uint8_t* p = p_start;
    uint64_t source_id;
    uint64_t offset_index;

    // decode source ID and offset index
    p = lmdb_helper::decode_uint64(p, &source_id);
    p = lmdb_helper::decode_uint64(p, &offset_index);

    // validate that there is space for hash label
    if ((size_t)(p - p_start) > encoding.size()) {
      assert(0);
    }

#ifdef DEBUG
    std::cout << "decoding " << lmdb_helper::binary_hash_to_hex(encoding)
              << "      to " << source_id << ", " offset_index << "\n";
#endif

    // return decoding
    return std::pair<uint64_t, uint64_t>();
  }

/*
  // encode source data
  static std::string encode_source_data(const lmdb_source_data_t& data) {

    // allocate space for the encoding
    size_t max_size =
                      10
                    + data.repository_name.size()
                    + 10
                    + data.filename.size()
                    + 10
                    + 10
                    + data.binary_hash.size();
    uint8_t encoding[max_size];
    uint8_t* p = encoding;

    // encode each field
    p = lmdb_helper::encode_sized_string(data.repository_name, p);
    p = lmdb_helper::encode_sized_string(data.filename, p);
    p = lmdb_helper::encode_uint64(data.filesize, p);
    p = lmdb_helper::encode_sized_string(data.binary_hash, p);

#ifdef DEBUG
    std::string encoding_string(reinterpret_cast<char*>(encoding), (p-encoding));
    std::cout << "encoding lmdb_source_data " << data << "\n"
              << "      to binary data "
              << lmdb_helper::binary_hash_to_hex(encoding_string)
              << " size " << encoding_string.size() << "\n";
#endif

    // return encoding
    return std::string(reinterpret_cast<char*>(encoding), (p-encoding));

  }

  // decode source data
  static lmdb_source_data_t decode_source_data(const std::string& encoding) {
    const uint8_t* p_start = reinterpret_cast<const uint8_t*>(encoding.c_str());
    const uint8_t* p = p_start;
    lmdb_source_data_t data;
    p = lmdb_helper::decode_sized_string(p, &data.repository_name);
    p = lmdb_helper::decode_sized_string(p, &data.filename);
    p = lmdb_helper::decode_uint64(p, &data.filesize);
    p = lmdb_helper::decode_sized_string(p, &data.binary_hash);

#ifdef DEBUG
    std::string hex_encoding = lmdb_helper::binary_hash_to_hex(encoding);
    std::cout << "decoding binary data " << hex_encoding
              << " size " << encoding.size() << "\n"
              << " to lmdb_source_data " << data << "\n";
#endif

    // validate that the encoding was properly consumed
    if ((size_t)(p - p_start) != encoding.size()) {
      assert(0);
    }

    return data;
  }

  // encode name data
  static std::string encode_name_data(const std::string& repository_name,
                                      const std::string& filename) {
    // build cstr
    size_t l1 = repository_name.length();
    size_t l2 = filename.length();
    size_t l3 = l1 + 1 + l2;
    char cstr[l3];  // space for strings separated by \0
    std::strcpy(cstr, repository_name.c_str()); // copy first plus \0
    std::memcpy(cstr+l1+1, filename.c_str(), l2); // copy second without \0

#ifdef DEBUG
    std::string binary_encoding(cstr, l3);
    std::cout << "encoding lmdb_name_data {\"repository_name\":\""
              << repository_name << "\",\"filename\":\"" << filename << "\"}\n"
              << "      to binary data "
              << lmdb_helper::binary_hash_to_hex(binary_encoding)
              << " size " << binary_encoding.size() << "\n";
#endif

    return std::string(cstr, l3);
  }
*/

  static std::string encode_uint64_data(const uint64_t data) {

    // allocate space for the encoding
    size_t max_size = 10;
    uint8_t encoding[max_size];
    uint8_t* p = encoding;

    // encode the field
    p = lmdb_helper::encode_uint64(data, p);

    // return encoding
    return std::string(reinterpret_cast<char*>(encoding), (p-encoding));
  }

  static uint64_t decode_uint64_data(const std::string& encoding) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(encoding.c_str());
    const uint8_t* p_start = p;
    uint64_t data = 0;
    p = lmdb_helper::decode_uint64(p, &data);

    // validate that the encoding was properly consumed
    if ((size_t)(p - p_start) != encoding.size()) {
      assert(0);
    }

    return data;
  }
};

#endif

