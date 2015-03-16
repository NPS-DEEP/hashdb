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

#ifndef LMDB_HASH_IT_DATA_HPP
#define LMDB_HASH_IT_DATA_HPP
#include <stdint.h>
#include <string>

struct lmdb_hash_it_data_t {
  std::string binary_hash;
  uint64_t source_lookup_index;
  uint64_t file_offset;
  std::string hash_label;
  bool is_valid;
  lmdb_hash_it_data_t(const std::string& p_binary_hash,
                      uint64_t p_source_lookup_index,
                      uint64_t p_file_offset,
                      const std::string& p_hash_label,
                      bool p_is_valid) :
              binary_hash(p_binary_hash),
              source_lookup_index(p_source_lookup_index),
              file_offset(p_file_offset),
              hash_label(p_hash_label),
              is_valid(p_is_valid) {
  }
  lmdb_hash_it_data_t() :
              binary_hash(),
              source_lookup_index(0),
              file_offset(0),
              hash_label(),
              is_valid(false) {
  }
  bool operator==(const lmdb_hash_it_data_t& other) const {
    if (is_valid == false && other.is_valid == false) return true;
    if (is_valid == false || other.is_valid == false) return false;
    return (binary_hash == other.binary_hash &&
            source_lookup_index == other.source_lookup_index &&
            file_offset == other.file_offset);
  }
};

inline std::ostream& operator<<(std::ostream& os,
                        const struct lmdb_hash_it_data_t& data) {
  os << "{\"lmdb_hash_it_data\":{\"hashdigest\":\""
     << lmdb_helper::binary_hash_to_hex(data.binary_hash)
     << "\",\"source_id\":" << data.source_lookup_index
     << ",\"file_offset\":" << data.file_offset
     << ",\"hash_label\":\"" << data.hash_label
     << "\",\"is_valid\":" << data.is_valid
     << "}";
  return os;
}

#endif

