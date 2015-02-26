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
 * Manage source data.  New fields may be appended in the future.
 */

#ifndef LMDB_SOURCE_DATA_HPP
#define LMDB_SOURCE_DATA_HPP

#include <string>
#include <sstream>
#include <cstdint>
#include <iostream>
#include "lmdb_helper.h"

class lmdb_source_data_t {
  private:
  // copy string
  bool copy(const std::string& from, std::string& to) {
    // return false if empty or same, return true if change empty
    // fail if attempt to change non-empty destination
    if (from == "" || from == to) {
      return false;
    }
    if (to != "") {
      std::cerr << "string copy error, attempt to change " << from << " to " << to << "\n";
      assert(0);
    }
    to = from;
    return true;
  }
  // copy uint64
  bool copy(uint64_t from, uint64_t& to) {
    if (from == 0 || from == to) {
      return false;
    }
    if (to != 0) {
      std::cerr << "uint64_t copy error, attempt to change " << from << " to " << to << "\n";
      assert(0);
    }
    to = from;
    return true;
  }

  public:
  std::string repository_name;
  std::string filename;
  uint64_t filesize;
  std::string binary_hash;

  lmdb_source_data_t() : repository_name(), filename(),
                             filesize(), binary_hash() {
  }

  lmdb_source_data_t(const std::string& p_repository_name,
                     const std::string& p_filename,
                     uint64_t p_filesize,
                     const std::string& p_binary_hash):
               repository_name(p_repository_name), filename(p_filename),
               filesize(p_filesize), binary_hash(p_binary_hash) {
  }

  bool operator==(const lmdb_source_data_t& other) const {
    return (repository_name == other.repository_name
            && filename == other.filename
            && filesize == other.filesize
            && binary_hash == other.binary_hash);
  }

  // add, true if added, false if same, fatal if different
  bool add(const lmdb_source_data_t& other) {
    bool changed = copy(other.repository_name, repository_name);
    changed |= copy(other.filename, filename);
    changed |= copy(other.filesize, filesize);
    changed |= copy(other.binary_hash, binary_hash);
    return changed;
  }
};

inline std::ostream& operator<<(std::ostream& os,
                        const class lmdb_source_data_t& data) {
  os << "{\"lmdb_source_data\":{\"repository_name\":\"" << data.repository_name
     << "\",\"filename\":\"" << data.filename
     << "\",\"filesize\":" << data.filesize
     << ",\"hashdigest\":\"" << lmdb_helper::binary_hash_to_hex(data.binary_hash)
     << "\"}}";
  return os;
}

#endif

