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
 * Provide support for LMDB operations.
 *
 * Note: it would be nice if MDB_val had a const type and a non-const type
 * to handle reading vs. writing.  Instead, we hope the callee works right.
 */

#ifndef LMDB_SOURCE_DATA_ENCODING_H
#define LMDB_SOURCE_DATA_ENCODING_H

#include "sys/stat.h"
#include "lmdb.h"
#include "lmdb_helper.h"
#include "lmdb_source_data.hpp"
#include <cstdint>
#include <sstream>

class lmdb_source_data_encoding {
  private:
  class private_string_reader {
    const char* ptr;
    size_t max;

    // do not allow copy or assignment
    private_string_reader(const private_string_reader&);
    private_string_reader& operator=(const private_string_reader&);

    public:
    private_string_reader(const char* p_ptr, size_t p_max) :
                 ptr(p_ptr), max(p_max) {
    }
    std::string get() {
      const char* p2 = (char*)memchr(ptr, 0, max);
      std::string s(ptr, (p2==NULL ? max : p2 - ptr));
      if (p2 == NULL) {
        max = 0;
      } else {
        size_t count = p2 - ptr + 1;
        max -= count;
        ptr += count;
      }
      return s;
    }
    ~private_string_reader() {
      if (max != 0) {
        std::cout << "Data error: unread data.\n";
      }
    }
  };

  public:
  static std::string lmdb_source_data_to_encoding(const lmdb_source_data_t data) {
    std::string filesize_string = lmdb_helper::uint64_to_encoding(data.filesize);
    size_t l1 = data.repository_name.size();
    size_t l2 = data.filename.size();
    size_t l3 = filesize_string.size();
    size_t l4 = data.binary_hash.size();
    
    // fill ordered null-delimited record from fields
    char chars[l1+1+l2+1+l3+1+l4];
    std::memcpy(chars, data.repository_name.c_str(), l1);
    size_t start = l1;
    chars[start++] = 0;
    std::memcpy(chars+start, data.filename.c_str(), l2);
    start += l2;
    chars[start++] = 0;
    std::memcpy(chars+start, filesize_string.c_str(), l3);
    start += l3;
    chars[start++] = 0;
    std::memcpy(chars+start, data.binary_hash.c_str(), l4);

    // return the encoding
    return std::string(chars, l1+1+l2+1+l3+1+l4);
  }

  static lmdb_source_data_t encoding_to_lmdb_source_data(const MDB_val& val) {
    private_string_reader reader(static_cast<char*>(val.mv_data), val.mv_size);
    lmdb_source_data_t data;
    data.repository_name = reader.get();
    data.filename = reader.get();
    std::string filesize_string = reader.get();
    MDB_val filesize_val;
    filesize_val.mv_data =
              static_cast<void*>(const_cast<char*>(filesize_string.c_str()));
    filesize_val.mv_size = filesize_string.size();
    data.filesize = lmdb_helper::encoding_to_uint64(filesize_val);
    data.binary_hash = reader.get();
    return data;
  }
};

#endif

