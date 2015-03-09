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

  // add, true if anything changes
  // Fail if repository_name or filename integrity are broken.
  // Zero out size and file hash if existing values change, indicating
  // that bulk_extractor is importing multiple sbufs, which is expected.
  // A zeroed size and hash are size=0, hash=1 byte of 0.
  bool add(const lmdb_source_data_t& other) {

    // repository name
    bool changed = false;
    if (other.repository_name != "" && repository_name != other.repository_name) {
      // internal error
      assert(0);
    }
    if (other.repository_name != "") {
      repository_name = other.repository_name;
      changed = true;
    }

    // filename
    if (other.filename != "" && filename != other.filename) {
      // internal error
      assert(0);
    }
    if (other.filename != "") {
      filename = other.filename;
      changed = true;
    }

    // set filesize and binary_hash as required
    if (other.filesize == 0 && other.binary_hash == "") {
      // fields are not being set, so nothing to do
    } else if (filesize == 0 &&
               binary_hash == lmdb_helper::hex_to_binary_hash("00")) {
      // file size and file hash are zeroed so leave them alone

    } else if (filesize != other.filesize || binary_hash != other.binary_hash) {
      // manage the change
      if (filesize == 0 && binary_hash == "") {
        // assinging values for the first time
        filesize = other.filesize;
        binary_hash = other.binary_hash;

      } else {
        // attempt to change existing value so zero both
        filesize = 0;
        binary_hash = lmdb_helper::hex_to_binary_hash("00");
      }
      changed = true;
    }

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

