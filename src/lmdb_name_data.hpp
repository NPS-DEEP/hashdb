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
 * Manage name data.  New fields may be appended in the future.
 */

#ifndef LMDB_NAME_DATA_HPP
#define LMDB_NAME_DATA_HPP

#include <string>
#include <sstream>
#include <cstdint>
#include <iostream>
#include "lmdb_helper.h"

class lmdb_name_data_t {

  public:
  std::string repository_name;
  std::string filename;

  lmdb_name_data_t() : repository_name(), filename() {
  }

  lmdb_name_data_t(const std::string& p_repository_name,
                     const std::string& p_filename):
               repository_name(p_repository_name), filename(p_filename) {
  }

  bool operator==(const lmdb_name_data_t& other) const {
    return (repository_name == other.repository_name
            && filename == other.filename);
  }

  void report_fields(std::ostream& os) const {
    os << "{\"lmdb_name_data\":{\"repository_name\":\"" << repository_name
       << "\",\"filename\":\"" << filename
       << "\"}}";
  }

  static std::string encode(const lmdb_name_data_t& data) {

    // allocate space for the encoding
    size_t max_size =
                      10
                    + data.repository_name.size()
                    + 10
                    + data.filename.size();
    uint8_t encoding[max_size];
    uint8_t* p = encoding;

    // encode each field
    p = lmdb_helper::encode_sized_string(data.repository_name, p);
    p = lmdb_helper::encode_sized_string(data.filename, p);

//#ifdef DEBUG
//zzzzzzzzzzzzzzzzzzzzz
//    std::string encoding_string(reinterpret_cast<char*>(p), (p-encoding));
//    std::cout << "encoding lmdb_source_data " << data << "\n"
//              << "      to binary data " << lmdb_helper::binary_hash_to_hex(encoding_string)
//              << " size " << encoding_string.size() << "\n";
//#endif

    // return encoding
    return std::string(reinterpret_cast<char*>(p), (p-encoding));

  }

  static lmdb_name_data_t decode(const std::string& encoding) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(encoding.c_str());
    const uint8_t* p_start = p;
    lmdb_name_data_t data;
    p = lmdb_helper::decode_sized_string(p, &data.repository_name);
    p = lmdb_helper::decode_sized_string(p, &data.filename);

//#ifdef DEBUG
//zzzzzzzzzzzzzzzzzzzzz
//    std::string hex_encoding = lmdb_helper::binary_hash_to_hex(encoding);
//    std::cout << "decoding binary data " << hex_encoding << " size " << val.mv_size << "\n"
//              << " to lmdb_source_data " << data << "\n";
//#endif

    // validate that the data was properly consumed
    if (p - p_start != encoding.size()) {
      assert(0);
    }

    return data;
  }
};

inline std::ostream& operator<<(std::ostream& os,
                        const class lmdb_name_data_t& data) {
  data.report_fields(os);
  return os;
}

#endif

