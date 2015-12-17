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

//zz
#define DEBUG

class lmdb_data_codec {

  private:
#ifdef DEBUG
  // help debug
  static inline uint8_t tohex(uint8_t c) {
    switch(c) {
      case 0 : return '0'; break;
      case 1 : return '1'; break;
      case 2 : return '2'; break;
      case 3 : return '3'; break;
      case 4 : return '4'; break;
      case 5 : return '5'; break;
      case 6 : return '6'; break;
      case 7 : return '7'; break;
      case 8 : return '8'; break;
      case 9 : return '9'; break;
      case 10 : return 'a'; break;
      case 11 : return 'b'; break;
      case 12 : return 'c'; break;
      case 13 : return 'd'; break;
      case 14 : return 'e'; break;
      case 15 : return 'f'; break;
      default:
        std::cerr << "char " << (uint32_t)c << "\n";
        assert(0);
        return 0; // for mingw compiler
    }
  }
  static std::string binary_hash_to_hex(const std::string& binary_hash) {
    std::stringstream ss;
    for (size_t i=0; i<binary_hash.size(); i++) {
      uint8_t c = binary_hash.c_str()[i];
      ss << tohex(c>>4) << tohex(c&0x0f);
    }
    return ss.str();
  }
#endif

  public:
  class ddd_t {
    public:
    uint64_t d1;
    uint64_t d2;
    uint64_t d3;
    ddd_t(): d1(0), d2(0), d3(0) {
    }
    ddd_t(uint64_t p_d1, uint64_t p_d2, uint64_t p_d3) :
                 d1(p_d1), d2(p_d2), d3(p_d3) {
    }
  };

  // encode uint64_t
  static std::string encode_uint64_data(uint64_t data) {

    // allocate space for the encoding
    size_t max_size = 10;
    uint8_t encoding[max_size];
    uint8_t* p = encoding;

    // encode the field
    p = lmdb_helper::encode_uint64(data, p);

    // return encoding
    return std::string(reinterpret_cast<char*>(encoding), (p-encoding));
  }

  // decode uint64_t
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

  // encode uint64_t, uint64_t
  static std::string encode_uint64_uint64_data(uint64_t data1,
                                               uint64_t data2) {

    // allocate space for the encoding
    size_t max_size = 10 + 10;
    uint8_t encoding[max_size];
    uint8_t* p = encoding;

    // encode source ID and offset index
    p = lmdb_helper::encode_uint64(data1, p);
    p = lmdb_helper::encode_uint64(data2, p);

    // return encoding
    std::string string_encoding(reinterpret_cast<char*>(encoding), (p-encoding));
#ifdef DEBUG
    std::cout << "encoding " << data1 << ", " << data2 << "\n"
              << "      to " << binary_hash_to_hex(string_encoding)
              << "\n";
#endif

    return std::string(reinterpret_cast<char*>(encoding), (p-encoding));
  }

  // decode uint64_t, uint64_t
  static std::pair<uint64_t, uint64_t> decode_uint64_uint64_data(
                         const std::string& encoding) {
    const uint8_t* const p_start = reinterpret_cast<const uint8_t*>(encoding.c_str());
    const uint8_t* p = p_start;
    uint64_t data1;
    uint64_t data2;

    // decode source ID and offset index
    p = lmdb_helper::decode_uint64(p, &data1);
    p = lmdb_helper::decode_uint64(p, &data2);

    // validate that there is space for hash label
    if ((size_t)(p - p_start) > encoding.size()) {
      assert(0);
    }

#ifdef DEBUG
    std::cout << "decoding " << binary_hash_to_hex(encoding)
              << "      to " << data1 << ", " << data2 << "\n";
#endif

    // return decoding
    return std::pair<uint64_t, uint64_t>(data1, data2);
  }

  // encode ddd data
  static std::string encode_ddd_t_data(uint64_t d1, uint64_t d2, uint64_t d3) {

    // allocate space for the encoding
    size_t max_size = 10 + 10 + 10;
    uint8_t encoding[max_size];
    uint8_t* p = encoding;

    // encode each field
    p = lmdb_helper::encode_uint64(d1, p);
    p = lmdb_helper::encode_uint64(d2, p);
    p = lmdb_helper::encode_uint64(d3, p);

#ifdef DEBUG
    std::string encoding_string(reinterpret_cast<char*>(encoding), (p-encoding));
    std::cout << "encoding ppp data " << d1 << ", " << d2 << ", " << d3 << "\n"
              << "      to binary data "
              << binary_hash_to_hex(encoding_string)
              << " size " << encoding_string.size() << "\n";
#endif

    // return encoding
    return std::string(reinterpret_cast<char*>(encoding), (p-encoding));
  }

  // decode ddd data
  static ddd_t decode_ddd_t_data(const std::string& encoding) {
    const uint8_t* p_start = reinterpret_cast<const uint8_t*>(encoding.c_str());
    const uint8_t* p = p_start;
    lmdb_data_codec::ddd_t ddd;
    p = lmdb_helper::decode_uint64(p, &ddd.d1);
    p = lmdb_helper::decode_uint64(p, &ddd.d2);
    p = lmdb_helper::decode_uint64(p, &ddd.d3);

#ifdef DEBUG
    std::string hex_encoding = binary_hash_to_hex(encoding);
    std::cout << "decoding ddd data " << hex_encoding
              << " size " << encoding.size() << "\n"
              << " to lmdb_source_data "
              << ddd.d1 << ", " << ddd.d2 << ", " << ddd.d3 << "\n";
#endif

    // validate that the encoding was properly consumed
    if ((size_t)(p - p_start) != encoding.size()) {
      std::cerr << "decode failure: " << &p << " is not " << &p_start << "\n";
      assert(0);
    }

    return ddd;
  }


  // encode ss data
  static std::string encode_ss_t_data(const std::string& s1,
                                      const std::string& s2) {

    // allocate space for the encoding
    size_t max_size = 10 + s1.size() + 10 + s2.size();

    uint8_t encoding[max_size];
    uint8_t* p = encoding;

    // encode each field
    p = lmdb_helper::encode_sized_string(s1, p);
    p = lmdb_helper::encode_sized_string(s2, p);

#ifdef DEBUG
    std::string encoding_string(reinterpret_cast<char*>(encoding), (p-encoding));
    std::cout << "encoding ss data " << s1 << ", " << s2 << "\n"
              << "      to binary data "
              << binary_hash_to_hex(encoding_string)
              << " size " << encoding_string.size() << "\n";
#endif

    // return encoding
    return std::string(reinterpret_cast<char*>(encoding), (p-encoding));
  }

  // decode ss data
  static std::pair<std::string, std::string> decode_ss_t_data(
                                              const std::string& encoding) {
    const uint8_t* p_start = reinterpret_cast<const uint8_t*>(encoding.c_str());
    const uint8_t* p = p_start;
    std::string s1;
    std::string s2;
    p = lmdb_helper::decode_sized_string(p, &s1);
    p = lmdb_helper::decode_sized_string(p, &s2);

#ifdef DEBUG
    std::string hex_encoding = binary_hash_to_hex(encoding);
    std::cout << "decoding ss data " << hex_encoding
              << " size " << encoding.size() << "\n"
              << " to lmdb_source_data "
              << s1 << ", " << s2 << "\n";
#endif

    // validate that the encoding was properly consumed
    if ((size_t)(p - p_start) != encoding.size()) {
      std::cerr << "decode failure: " << &p << " is not " << &p_start << "\n";
      assert(0);
    }

    return std::pair<std::string, std::string>(s1, s2);
  }
};

#endif

