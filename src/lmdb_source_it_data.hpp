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
 * Fields for managing hash iteration.
 */

#ifndef LMDB_SOURCE_IT_DATA_HPP
#define LMDB_SOURCE_IT_DATA_HPP
#include <cstdint>
#include <string>
#include "lmdb_source_data.hpp"

struct lmdb_source_it_data_t {
  uint64_t source_lookup_index;
  lmdb_source_data_t source_data;
  bool is_valid;
  lmdb_source_it_data_t(uint64_t p_source_lookup_index,
                        lmdb_source_data_t p_source_data,
                        bool p_is_valid) :
            source_lookup_index(p_source_lookup_index), 
            source_data(p_source_data),
            is_valid(p_is_valid) {
  }
  lmdb_source_it_data_t() :
            source_lookup_index(), 
            source_data(),
            is_valid(false) {
  }
  bool operator==(const lmdb_source_it_data_t& other) const {
    if (is_valid == false && other.is_valid == false) return true;
    if (is_valid == false || other.is_valid == false) return false;
    return (source_lookup_index == other.source_lookup_index &&
            source_data == other.source_data);
  }
};

inline std::ostream& operator<<(std::ostream& os,
                        const struct lmdb_source_it_data_t& data) {
  os << "{\"lmdb_source_it_data\":{\"source_lookup_index\":\""
     << data.source_lookup_index
     << "\",\"source_id\":" << data.source_lookup_index
     << ",\"source_data\":" << data.source_data
     << ",\"is_valid\":" << data.is_valid
     << "}";
  return os;
}

#endif

