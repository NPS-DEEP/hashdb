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
 * This file defines basic types required for working with a hashdb.
 */

#ifndef    HASH_ALGORITHM_TYPES_H
#define    HASH_ALGORITHM_TYPES_H

#include <stdint.h>
#include <cassert>
#include <string>
#include <iostream>

enum hash_algorithm_type_t {
          HASH_ALGORITHM_MD5,
          HASH_ALGORITHM_SHA1,
          HASH_ALGORITHM_SHA256
};

inline std::string hash_algorithm_type_to_string(hash_algorithm_type_t type) {
  switch(type) {
    case HASH_ALGORITHM_MD5:    return "md5";
    case HASH_ALGORITHM_SHA1:   return "sha1";
    case HASH_ALGORITHM_SHA256: return "sha256";
    default: assert(0); return "";
  }
}

inline bool string_to_hash_algorithm_type(const std::string& name, hash_algorithm_type_t& type) {
  if (name == "md5")    { type = HASH_ALGORITHM_MD5; return true; }
  if (name == "sha1")   { type = HASH_ALGORITHM_SHA1; return true; }
  if (name == "sha256") { type = HASH_ALGORITHM_SHA256; return true; }
  type = HASH_ALGORITHM_MD5;
  return false;
}

inline std::ostream& operator<<(std::ostream& os, const hash_algorithm_type_t& t) {
  os << hash_algorithm_type_to_string(t);
  return os;
}

#endif

