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
#include <stdint.h>
#include <iostream>
#include "lmdb_helper.h"

class lmdb_hash_data_t {
  public:
  uint64_t source_id;
  uint64_t offset_index;
  std::string hash_label;

  lmdb_hash_data_t() : source_id(0), offset_index(0), hash_label() {
  }

  lmdb_hash_data_t(uint64_t p_source_id,
                   uint64_t p_offset_index,
                   const std::string& p_hash_label) :
                          source_id(p_source_id),
                          offset_index(p_offset_index),
                          hash_label(p_hash_label) {
  }

  bool operator==(const lmdb_hash_data_t& other) const {
    return (source_id == other.source_id && offset_index == other.offset_index);
  }
};

inline std::ostream& operator<<(std::ostream& os,
                        const class lmdb_hash_data_t& data) {
  os << "{\"lmdb_hash_data\":{\"source_id\":" << data.source_id
     << ",\"offset_index\":" << data.offset_index
     << ",\"hash_label\":\"" << data.hash_label << "\""
     << "}}";
  return os;
}

#endif

