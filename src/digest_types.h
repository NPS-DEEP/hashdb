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

#ifndef    digest_types_h
#define    digest_types_h

#include <stdint.h>
#include <cassert>
#include <string>
#include <iostream>

enum digest_type_t {
          MAP_BTREE,
          MAP_FLATSORTED_VECTOR,
          MAP_RED_BLACK_TREE,
          MAP_UNORDERED_HASH
};

inline std::string digest_type_to_string(digest_type_t type) {
  switch(type) {
    case DIGEST_MD5:    return "md5";
    case DIGEST_SHA1:   return "sha1";
    case DIGEST_SHA256: return "sha256";
    default: assert(0); return "";
  }
}

inline bool string_to_digest_type(const std::string& name, digest_type_t& type) {
  if (name == "md5")    { type = DIGEST_MD5; return true; }
  if (name == "sha1")   { type = DIGEST_SHA1; return true; }
  if (name == "sha256") { type = DIGEST_SHA256; return true; }
  type = DIGEST_MD5;
  return false;
}

inline std::ostream& operator<<(std::ostream& os, const digest_type_t& t) {
  os << digest_type_to_string(t);
  return os;
}

#endif

